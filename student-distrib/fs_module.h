/*	
	fs_module.h - added by mlee148
	
	header file for fs_module.c
*/

#ifndef _fs_module_H_
#define _fs_module_H_

#include "lib.h"
#include "system_call.h"

#define MAX_FILENAME 32
#define MAX_FILECOUNT 63
#define MAX_DATA_BLOCK 1023		//Max number of data blocks is (4096/4 - 1) Because first block is reserved and each block is 4B

	
typedef struct {	//dir. entry struct
	char name[MAX_FILENAME];
	uint32_t type;
	uint32_t inode;
	uint8_t reserved[24];
} dentry_t;

typedef struct {	//boot block struct
	uint32_t dir_entries;
	uint32_t inode;
	uint32_t data_blocks;
	uint8_t reserved[52];
	dentry_t entries[MAX_FILECOUNT];
} boot_block_t;

typedef struct {	//inode struct
	uint32_t length;
	uint32_t data_block[MAX_DATA_BLOCK];
} inode_t;

typedef struct {	//data block struct
	uint32_t data[1024];
} data_block_t;

//file system initialization function
void fs_init(uint32_t module_start);

//read dir entry given a file name
int32_t read_dentry_by_name(const char* fname, dentry_t* dentry);

//read dir entry given an index node
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

//read data from file given inode, offset, and length
int32_t read_data(uint32_t inode, uint32_t offset, char* buf, uint32_t length);

//open/close/write/read functions for files and directories
int32_t file_open(int32_t* inode, char* filename);
int32_t file_close(int32_t* inode);
int32_t file_write(int32_t* inode, uint32_t* offset, char* buf, uint32_t len);
int32_t file_read(int32_t* inode, uint32_t* offset, char* buf, uint32_t len);
int32_t dir_open(int32_t* inode, char* filename);
int32_t dir_close(int32_t* inode);
int32_t dir_write(int32_t* inode, uint32_t* offset, char* buf, uint32_t len);
int32_t dir_read(int32_t* inode, uint32_t* offset, char* buf, uint32_t len);
int32_t directory_read(uint32_t offset, char* buf, uint32_t len);

//fops table for file and directory
extern fops_table_t file_fops;
extern fops_table_t dir_fops;

#endif
