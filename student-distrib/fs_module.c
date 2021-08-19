/*
	fs_module.c - added by mlee148

	This implements the file system for MP3.
	It has functions that initialize the file system as well as read/write/open/close files and directories.
*/


#include "fs_module.h"
#include "system_call.h"

//global boot block; only one
boot_block_t* boot_block = NULL;

fops_table_t dir_fops = {
	.open = dir_open,
	.close = dir_close,
	.read = dir_read,
	.write = dir_write
};

fops_table_t file_fops = {
	.open = file_open,
	.close = file_close,
	.read = file_read,
	.write = file_write
};

/*
   fs_init
   		DESCRIPTION: initialize file system from module start location
   		INPUTS: N/A
		OUTPUT: N/A
		SIDE EFFECTS: boot block pointer set to module start
 */
void fs_init(uint32_t module_start){

	boot_block = (boot_block_t*) module_start;
}

/*
   read_dentry_by_name
   		DESCRIPTION: Finds dir. entry from given file name
   		INPUTS: fname - name of file to be read
				dentry - pointer to dentery object
		OUTPUT: 0 on success, -1 on failure
		SIDE EFFECTS: dentry object populated with file
 */
int32_t read_dentry_by_name(const char* fname, dentry_t* dentry){

	int fname_length = strlen(fname);
	//printf("file name length: %d", fname_length);
	if(fname_length > MAX_FILENAME)	//check that file name does not exceed limit
		return -1;

	if(dentry == NULL)						//Make sure pointer is valid
		return -1;

	int i = 0;
	for(i = 0; i < MAX_FILECOUNT; i++){		//loop through all dir. entries and find matching file

		//load file entry at [i]
		dentry_t* f = &(boot_block->entries[i]);

		//if all char in string match, file is found
		//fname_length + 1 to account for terminating 0x0
		if(fname_length < MAX_FILENAME)
			fname_length++;

		//compare strings and copy file info if match
		if(strncmp(fname, f->name, fname_length) == 0){

			*dentry = *f;
			return 0;
		}
	}

	//file not found
	return -1;
}

/*
   read_dentry_by_index
   		DESCRIPTION: Finds dir. entry from given index node
   		INPUTS: index - index node of file
				dentry - pointer to dentery object
		OUTPUT: 0 on success, -1 on failure
		SIDE EFFECTS: dentry object populated with file
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry){

	if(dentry == NULL)						//Make sure pointer is valid
		return -1;

	if(index >= boot_block->dir_entries)	//Make sure inode index is valid
		return -1;

	//load file entry at index if index is valid
	*dentry = boot_block->entries[index];

	return 0;
}

/*
   read_data
   		DESCRIPTION: Reads file data onto buffer
   		INPUTS: inode - index node of file
				offset - starting position of read
				buf - buffer to be filled
				length - length of data to be read
		OUTPUT: 0 on success, -1 on failure
		SIDE EFFECTS: file data is copied to buffer
 */
int32_t read_data(uint32_t inode, uint32_t offset, char* buf, uint32_t length){

	uint32_t first_block = offset / 4096;			//position of first data block is offset/size of each block which is 4KB(4096B)
	uint32_t last_block = (offset + length) / 4096;	//position of last data block is (offset + length) / size of each block
	uint32_t data = 0;
	uint32_t bytes_copied = 0;

	if(buf == NULL)					//if buffer pointer is invalid, failure
		return -1;

	if(inode >= boot_block->inode)	//if inode index exceeds max, failure
		return -1;

	//location of inode is inode from boot_block location; + 1 is to account for the boot block itself
	inode_t* inode_temp = (inode_t*) boot_block + (inode + 1);

	//if length puts it over the end of file, reduce back to file size
	if(offset + length > inode_temp->length)
		length = inode_temp->length - offset;

	//if offset reaches beyond end of file, return 0
	if(offset >= inode_temp->length)
		return 0;

	int i = 0;
	for(i = first_block; i <= last_block; i++){		//loop through all blocks within inode

		//Get position of data block, then compute position of given data block
		data = inode_temp->data_block[i];
		data_block_t* data_location = (data_block_t*) boot_block + (boot_block->inode + data + 1);

		//default "offsets" if whole block is to be copied
		int start_offset = 0;
		int end_offset = 4096;

		/* for reference

		   offset  length
			 |----------->|
		|	||  |	|	| | |	|
			 |			  |
		start_offset  end_offset

		each block size 4096B
		*/

		//if first block, you copy from block location + start_offset to end of block
		if(i == first_block){
			start_offset = (offset%4096);
		}
		//if last block, you copy from start of block to end_offset
		if(i == last_block){
			//if invalid last block, quit out before copying
			if(last_block*4096 > inode_temp->length)
				break;
			end_offset = (offset + length)%4096;
		}

		//length of data to be copied is from start_offset to end_offset.
		//This way, all edge cases are handled, including when start offset and end offset are in the same block

		// int len = end_offset - start_offset;
		// memcpy(buf + bytes_copied, data_location + start_offset, len);

		int len = end_offset - start_offset;
		uint8_t * addr = (uint8_t*)data_location;
		addr = (uint8_t*) data_location + start_offset;
		memcpy(buf + bytes_copied, (uint8_t*)addr, len);

		//add to total bytes copied
		bytes_copied += len;
	}
	return bytes_copied;
}

/*
   file_open
   		DESCRIPTION: open file from file name
   		INPUTS: inode - index pointer
				filename - name of file
		OUTPUT: 0 on success, -1 on failure
		SIDE EFFECTS: file inode read
 */
int32_t file_open(int32_t* inode, char* filename){

	dentry_t f;

	//Error out if can't read file
	if(read_dentry_by_name(filename, &f) == -1)
		return -1;

	//copy index node
	*inode = f.inode;
	return 0;
}

/*
   file_close
   		DESCRIPTION: undo file open
   		INPUTS: inode - index pointer
		OUTPUT: 0 on success, -1 on failure
		SIDE EFFECTS: index deleted
 */
int32_t file_close(int32_t* inode){

    *inode = 0;
    return 0;
}

/*
   file_write
   		DESCRIPTION: unused function for MP3
   		INPUTS: inode - index pointer
				offset - starting point
				buf - data to be written
				len - length
		OUTPUT: 0 on success, -1 on failure
		SIDE EFFECTS:
 */
int32_t file_write(int32_t* inode, uint32_t* offset, char* buf, uint32_t len){

	return -1;
}

/*
   file_read
   		DESCRIPTION: read file from index, offset, and length
   		INPUTS: inode - index pointer
				offset - starting point
				buf - buffer to contain data
				len - length of data to be copied
		OUTPUT: 0 on success, -1 on failure
		SIDE EFFECTS: data read to buffer
 */
int32_t file_read(int32_t* inode, uint32_t* offset, char* buf, uint32_t len){

	int32_t result = read_data(*inode, *offset, buf, len);
	//printf("result = %d", result);
	if(result > 0)
		*offset += result;
	return result;
}

/*
   dir_open
   		DESCRIPTION: open directory from file name
   		INPUTS: inode - index pointer
				filename - name of file to open
		OUTPUT: 0 on success, -1 on failure
		SIDE EFFECTS: index copied
 */
int32_t dir_open(int32_t* inode, char* filename){

	dentry_t f;

	//Error out if can't read file
	if(read_dentry_by_name(filename, &f) == -1)
		return -1;

	//copy index
	*inode = f.inode;
	return 0;
}

/*
   dir_close
   		DESCRIPTION: undo dir_open
   		INPUTS: inode - index pointer
		OUTPUT: 0 on success, -1 on failure
		SIDE EFFECTS: index deleted
 */
int32_t dir_close(int32_t* inode){

	*inode = 0;
    return 0;
}

/*
   dir_write
   		DESCRIPTION: unused function for MP3
   		INPUTS: inode - index pointer
				offset - starting point
				buf - data to be written
				len - length
		OUTPUT: 0 on success, -1 on failure
		SIDE EFFECTS:
 */
int32_t dir_write(int32_t* inode, uint32_t* offset, char* buf, uint32_t len){

    return -1;
}

/*
   dir_read
   		DESCRIPTION: unused function for MP3
   		INPUTS: inode - index pointer
				offset - starting point
				buf - data to be written
				len - length
		OUTPUT: 0 on success, -1 on failure
		SIDE EFFECTS:
 */
int32_t dir_read(int32_t* inode, uint32_t* offset, char* buf, uint32_t len){

    int result= directory_read(*offset, buf, len);
	if(result>0)
		*offset+=1;				//go to next file
    return result;
}

/*
   directory_read
   		DESCRIPTION: reads file directory
   		INPUTS: offset - index for files
				buf - buffer for data
				len - size of buffer
		OUTPUT: 0 on success, -1 on failure
		SIDE EFFECTS: file name is read onto buffer
 */
int32_t directory_read(uint32_t offset, char* buf, uint32_t len){

	//Check for offset overflow
	if(offset >= MAX_FILECOUNT)
		return -1;

	//Check for invalid buffer
	if(buf == NULL)
		return -1;

	//Limit length to max file name length
	if(len > MAX_FILENAME)
		len = MAX_FILENAME;

	//find dir. entry at offset
	dentry_t* f = &(boot_block->entries[offset]);

	//Limit length to file name length
	if(len > strlen(f->name))
		len = strlen(f->name);

	//place file name into buffer
	memcpy((char*)buf, (char*)f->name, len);

	return len;
}
