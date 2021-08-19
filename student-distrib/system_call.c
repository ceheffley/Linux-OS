/* system_call.c
 */

#include "system_call.h"
#include "fs_module.h"
#include "x86_desc.h"
#include "rtc.h"
#include "keyboard.h"
#include "paging.h"
#include "lib.h"

// Keep track of currently running task
int32_t tasks_running = -1;

// Array that tracks if task is running in the slot
uint32_t task_slots[6] = {0,0,0,0,0,0};

// 1 if program died by exception, 0 if didn't
uint32_t exception_death = 0;

/* system_halt()
 * Input: status
 * Returns: 32 bit int expanded from 8 bit argument for execute's return val
 * Effect: Terminates a process, returns specified value to its parent process
 */
int32_t syscall_halt(uint8_t status) {

	uint32_t i, return_status;

	//************RESTORE PARENT DATA************//
	pcb_t* process_control_block = get_pcb(tasks_running);

	// Make sure it isnt the final shell
	if(process_control_block->task_id_parent == -1) {
		// Don't want final shell to exit, so rerun shell program

		// Message to indicate they tried to halt last shell
		printf("Halting final shell is not allowed\n");

		tss.ss0 = KERNEL_DS; // Kernel's stack segment
		tss.esp0 = (eightK * oneK - (eightK * (tasks_running)) - 4); //process' kernel-mode stack

		// Grab the EIP from pcb
		uint32_t EIP_bytes;
		EIP_bytes = process_control_block->old_eip;

		// Do iret, explanation of stack at bottom of syscall_execute
		asm volatile(
		"movl $0x2B, %%eax \n"
		"movw %%ax, %%ds \n"
		"movw %%ax, %%es \n"
		"movw %%ax, %%fs \n"
		"movw %%ax, %%gs \n"
		"pushl %%eax \n"
		"pushl $0x83FFFFC \n"
		"pushf \n"
		"pushl $0x23 \n"
		"pushl %0 \n"
		"iret \n"
		:
		:"r" (EIP_bytes)
		:"%eax"
		);
	}

	pcb_t* parent_control_block = get_pcb(process_control_block->task_id_parent);
	parent_control_block->task_id_child = -1;

	//************RESTORE PARENT PAGING************//

	syscall_paging_setup(eightM + (process_control_block->task_id_parent) * fourM);
	uint32_t k = _148MB / fourM;
	uint32_t j = page_directory[k] & 0x01; // check whether the page is allocated
	if ( j ){
		videomem_unmap(_148MB, video_mem_addr);
	}

	//************CLOSE RELEVANT FDS************//

	// Check if fd 2-7 are in use, close if they are
	for(i = 0; i < 8; i++) {
		if(process_control_block->fd[i].flags) {
			syscall_close(i);
		}
	}

	//Clear the allocated task slot
	task_slots[tasks_running] = 0;

	//Change tasks_running to the parent's id
	tasks_running = process_control_block->task_id_parent;

	//************JUMP TO EXECUTE'S RETURN************//

	tss.esp0 = process_control_block->old_esp;

	// If died by exception, return 256, otherwise return status
	if(exception_death) {
		return_status = 256;
		exception_death = 0;
	}
	else
		return_status = (uint32_t)status;

	// Store status in eax, change stack ptrs to previous prgm
	asm volatile(
		"movl %0, %%eax \n"
		"movl %1, %%esp \n"
		"movl %2, %%ebp \n"
		:
		:"r"(return_status), "r"(process_control_block->old_esp), "r"(process_control_block->old_ebp)
		:"eax"
	);

	// Set up to return with execute's stack
	asm volatile(
		"leave \n"
		"ret \n"
	);

	return 0;
}





/* system_execute()
 * Input: command pointer for what to be executed
 * Returns: -1 if cannot be execute, 256 if program dies by exception, 0-255 otherwise if halt happens
 * Effect: Attempts to load and execute a new program, hands off processor until new program terminates
 */
int32_t syscall_execute(const uint8_t* command) {

	// Local variables
	uint8_t task_name[TASKNAME_SIZE]; // max size of fname max size + 1 for null char
	uint8_t argument[ARGUMENT_SIZE]; // max size of keyboard buffer max size + 1 for null char
	uint32_t i, j;		  // loop counts
	char buf[4];		  // char buf for file_read() to check if elf, need 4 bytes to check
	dentry_t file_check;
	uint32_t EIP_bytes = 0;
	uint32_t pcb_esp, pcb_ebp;
	uint32_t blank_cmd_flag = 0;
	uint32_t new_slot, old_slot;

	//************PARSE ARGS************//
	i = 0;
	j = 0;

	// Get rid of leading spaces
	while(command[i] == SPACE_CHAR) {
		i++;
	}

	// Grab task_name from command, max size of 32 (longest fname size)
	while((command[i] != SPACE_CHAR) && (command[i] != '\n') && (command[i] != NULL_CHAR)&& (j < (TASKNAME_SIZE - 1))) {
		task_name[j] = command[i];
		j++;
		i++;
	}

	// Make sure command was fully recorded, if went over char limit, return invalid
	if((j == (TASKNAME_SIZE - 1)) && ((command[i] != '\n') && (command[i] != SPACE_CHAR) && (command[i] != NULL_CHAR))) {
		return -1;
	}

	// Add null to end of task_name
	task_name[j] = NULL_CHAR;

	// Go through all spaces
	while((command[i] != '\n') && (command[i] == SPACE_CHAR) && (command[i] != NULL_CHAR)) {
		i++;
	}

	// Grab argument from command, max size of 128 (max keyboard buf size), piazza @914 says dont care about spaces
	j = 0;
	while((command[i] != '\n') && (j < (ARGUMENT_SIZE - 1)) && (command[i] != NULL_CHAR)) {
		argument[j] = command[i];
		j++;
		i++;
	}

	// Add null to end of argument
	argument[j] = NULL_CHAR;
		

	//Check if task_name is blank
	for(i = 0; i < strlen((const char*)task_name); i++) {
		if((task_name[i] != ' ') || (task_name[i] == '\n'))
			blank_cmd_flag = 1;
	}
	//If task_name was blank, don't display anything in shell
	if(blank_cmd_flag == 0)
		return 0;

	//************CHECK FILE VALIDITY************//

	// Check if task_name is a valid file, return invalid if not
	read_dentry_by_name((const char*)task_name, &file_check);
	if(read_data(file_check.inode, 0, buf, 4) != 4) {
		return -1;
	}

	// Check buf to make sure file is elf, return invalid if not
	if((buf[0] != ELF_CHECK0) || (buf[1] != ELF_CHECK1) || (buf[2] != ELF_CHECK2) || (buf[3] != ELF_CHECK3)) {
		return -1;
	}

	// Make sure there aren't 6 tasks running already, else, go next task
	new_slot = find_open_task();
	if(new_slot == -1) {
		printf("Max programs reached, only valid command: exit\n");
		return -1;
	} else {
		old_slot = tasks_running;
		tasks_running = new_slot;
		task_slots[tasks_running] = 1;
	}

	//************SET UP PAGING************//
	syscall_paging_setup(eightM + (tasks_running) * fourM);

	//************LOAD FILE INTO MEMORY************//
	read_data(file_check.inode, 0, (char*)program_addr, twoM);

	// Grab bytes 24-27 for the EIP
	EIP_bytes = *((uint32_t*)program_addr + eip_offset);

	//************CREATE PCB/OPEN FDs************//

	// Assign the pcb to a portion in kernel space

	pcb_t* process_control_block = get_pcb(tasks_running);
	// Fill in pcb task values
	process_control_block->task_id = tasks_running;

	// If not one of three base shells, fill in parent info
	if(tasks_running >= 3) { 
		process_control_block->task_id_parent = old_slot;
		// Also tell parent block that it has a child
		pcb_t* parent_control_block = get_pcb(old_slot);
		parent_control_block->task_id_child = tasks_running;
	} else {
		process_control_block->task_id_parent = -1;
	}

	// Get current esp and ebp for pcb
	asm volatile (
	"movl %%esp, %0 \n"
	"movl %%ebp, %1 \n"
	:"=r" (pcb_esp), "=r" (pcb_ebp)
	:
	);
	process_control_block->task_id_child = -1;
	process_control_block->old_esp = pcb_esp;
	process_control_block->old_ebp = pcb_ebp;
	process_control_block->old_eip = EIP_bytes;

	//fill in file name and argument buf
	for(i = 0; i < (TASKNAME_SIZE); i++) {
		process_control_block->file_name[i] = task_name[i];
		process_control_block->argument_buf[i] = argument[i];
	}
	for(; i < (ARGUMENT_SIZE); i++) {
		process_control_block->argument_buf[i] = argument[i];
	}

	// init fd
	fda_init();

	//************PREPARE FOR CONTEXT SWITCH************//

	tss.ss0 = KERNEL_DS; // Kernel's stack segment
	tss.esp0 = (eightK * oneK - (eightK * (tasks_running)) - 4); //process' kernel-mode stack



	//************IRET STACK************//

	/* Interrupted Procedure's Stack	Handler's Stack
	 * [        ] <-ESP	before			[	SS		] (User DS = 0x2B)
	 * [        ]						[	ESP		] (bottom of page holding executable, 132 MB - 4 = 0x8400000 - 4 = 0x83FFFFC)
	 * [        ]						[	EFLAGS	]
	 * [        ]		=>				[	CS		] (User CS = 0x23)
	 * [        ]						[	EIP		] (bytes 24-27 of executable)
	 * [        ]						[Error code	] <-ESP after
	 */

	// All values explained above
	asm volatile(
		"movl $0x2B, %%eax \n"
		"movw %%ax, %%ds \n"
		"movw %%ax, %%es \n"
		"movw %%ax, %%fs \n"
		"movw %%ax, %%gs \n"
		"pushl %%eax \n"
		"pushl $0x83FFFFC \n"
		"pushf \n"
		"pushl $0x23 \n"
		"pushl %0 \n"
		"iret \n"
		:
		:"r" (EIP_bytes)
		:"%eax"
	);

	// Returns from end of halt, not here, if here, it's an error
	return -1;
}

/* system_read
 * Input: fd - file descriptor id
		  buf - buffer to written with contents from file
		  nbytes - number of bytes to be read from file
 * Returns: -1 if cannot be execute
 * Effect: buf is written with file contents
 */
int32_t syscall_read (uint32_t fd, void* buf, int32_t nbytes){

	//check for invalid input
	if((fd > 7) || (fd == 1))
		return -1;

	pcb_t* pcb = get_pcb(tasks_running);
	file_descriptor_t* fd_array = pcb->fd;

	//error out if fd table is invalid
	if(fd_array == NULL)
		return -1;

	//if file is unopened, return invalid
	if(fd_array[fd].flags == 0)
		return -1;

	//if read is unused, return -1
	if(fd_array[fd].fops_table->read == NULL)
		return -1;

	//if the called read function returns -1, return -1
	/*if((*fd_array[fd].fops_table->read)(&fd_array[fd].inode, &fd_array[fd].file_position, (char*) buf, nbytes) == -1)
		return -1;*/

	return (*fd_array[fd].fops_table->read)(&fd_array[fd].inode, &fd_array[fd].file_position, (char*) buf, nbytes);

}


/* system_write
 * Input: fd - file descriptor id
		  buf - buffer to be read from
		  nbytes - number of bytes to write to file
 * Returns: -1 if cannot be execute
 * Effect: file is written with contents from buf
 */
int32_t syscall_write(uint32_t fd, void* buf, int32_t nbytes){
	//check for invalid input
	if((fd > 7) || (fd < 1))
		return -1;

	pcb_t* pcb = get_pcb(tasks_running);
	file_descriptor_t* fd_array = pcb->fd;

	//error out if fd table is invalid
	if(fd_array == NULL)
		return -1;

	//if file is unopened, return invalid
	if(fd_array[fd].flags == 0)
		return -1;

	//if write is unused, return -1
	if(fd_array[fd].fops_table->write == NULL)
		return -1;

	//if the called write function returns -1, return -1
	/*if((*fd_array[fd].fops_table->write)(&fd_array[fd].inode, &fd_array[fd].file_position, (char*) buf, nbytes) == -1)
		return -1;*/

	return (*fd_array[fd].fops_table->write)(&fd_array[fd].inode, &fd_array[fd].file_position, (char*) buf, nbytes);

}


/* system_open() - added by mlee148
 * Input: file name to be opened
 * Returns: -1 if cannot be execute
 * Effect:
 */
int32_t syscall_open(const uint8_t* filename) {

	//null check
	if((filename == NULL) || (*filename == NULL))
		return -1;

	//get current file descriptor array from pcb
	pcb_t* pcb = get_pcb(tasks_running);
	file_descriptor_t* fd_array = pcb->fd;

	//error out if fd table is invalid
	if(fd_array == NULL)
		return -1;

	//find an empty position in fda to open
	int fd_index = 2;
	while(fd_index < 8){

		//flags=0 means open spot
		if(fd_array[fd_index].flags == 0)
			break;

		fd_index++;
	}

	//if no free space in fda, error out
	if(fd_index > 7)
		return -1;

	//char* f = filename;
	//detect if stdin or stdout is called and set correct fops table
	if(strncmp("stdin", (const char*)filename, 6) == 0)
		fd_array[fd_index].fops_table = &stdin_fops;
	else if(strncmp("stdout", (const char*)filename, 7) == 0)
		fd_array[fd_index].fops_table = &stdout_fops;

	dentry_t dentry;

	//check if file exists
	if(read_dentry_by_name((char*) filename, &dentry) == 0){

		//set correct fops table for each file type
		if(dentry.type == 0)	//rtc
			fd_array[fd_index].fops_table = &rtc_fops;
		else if(dentry.type == 1)	//folder
			fd_array[fd_index].fops_table = &dir_fops;
		else if(dentry.type == 2)	//file
			fd_array[fd_index].fops_table = &file_fops;
	}
	else
		return -1;	//return error if file is non-existent

	//if open fails
	if((*fd_array[fd_index].fops_table->open)(&fd_array[fd_index].inode, (char*) filename) == -1)
		return -1;

	//set flag to 1 signifying index is filled
	fd_array[fd_index].flags = 1;
	//return the fd index where file was initialized
	return fd_index;
}

/* system_close() - added by mlee148
 * Input: file name to be opened
 * Returns: -1 if cannot be execute
 * Effect:
 */
int32_t syscall_close(int32_t fd) {

	//check for invalid input
	if((fd > 7) || (fd < 2))
		return -1;



	//get current file descriptor array from pcb
	pcb_t* pcb = get_pcb(tasks_running);
	file_descriptor_t* fd_array = pcb->fd;

	//already closed
	if(fd_array[fd].flags == 0)
		return -1;

	//error out if fd table is invalid
	if(fd_array == NULL)
		return -1;

	//if close is unused, return -1
	if(fd_array[fd].fops_table->close == NULL)
		return -1;

	//if the called close function returns -1, return -1
	if((*fd_array[fd].fops_table->close)(&fd_array[fd].inode) == -1)
		return -1;

	//reset entry's file_position
	fd_array[fd].file_position = 0;

	//free the fda entry and return 0
	fd_array[fd].flags = 0;
	return 0;
}

/* syscall_getargs() - added by mlee148
 * Input: file name to be opened
 * Returns: -1 if cannot be execute
 * Effect:
 */
int32_t syscall_getargs(uint8_t* buf, int32_t nbytes) {

	//null check
	if(buf == NULL)
		return -1;

	//get current pcb
	pcb_t* pcb = get_pcb(tasks_running);

	//error out if argument buffer is too big
	if(strlen((const char*)pcb->argument_buf) > nbytes)
		return -1;
		
	//if argument is blank, return -1
	if(pcb->argument_buf[0] == NULL_CHAR)
		return -1;

	//copy argument to user space
	memset(buf, 0, nbytes);

	//if nbytes is too big, only copy maximum argument amount
	if(nbytes > ARGUMENT_SIZE)
		memcpy(buf, pcb->argument_buf, ARGUMENT_SIZE);
	else
		memcpy(buf, pcb->argument_buf, nbytes);

	return 0;
}
/* syscall_vidmap
 * Input: user ptr to a ptr to be assigned vidmem
 * Returns: -1 if cannot be execute
 * Effect: allocate user_level video_mem
 */
int syscall_vidmap(uint8_t ** screen_start){
	if ( screen_start == NULL || screen_start == (uint8_t **)fourM)
		return -1;
	//148MB is the user video page location 
	videomem_map(_148MB, video_mem_addr);
	*screen_start = (uint8_t*) _148MB ;

	 return 0;

}
/* get_pcb()
 * Input: task id of pcb to grab
 * Return: pointer to pcb specified
 * Effect: helper function to grab a pcb pointer
 */
pcb_t* get_pcb(uint32_t grab_task_id) {
	return (pcb_t*)(eightK * oneK - (eightK * (grab_task_id + 1)));
}

/* fda_init()
 * Input: none
 * Return: success or failure
 * Effect: fda initialization function, stdin and stdout is allocated
 */
int32_t fda_init() {

	//get current file descriptor array from pcb
	pcb_t* pcb = get_pcb(tasks_running);
	file_descriptor_t* fd_array = pcb->fd;

	//initialize all 8 "files"
	int i = 0;
	for(i = 0; i < 8; i++){
		fd_array[i].fops_table = NULL;
		fd_array[i].inode = 0;
		fd_array[i].file_position = 0;
		fd_array[i].flags = 0;
	}

	//set fops table for stdin and stdout which are fd_array[0] and [1]
	fd_array[0].fops_table = &stdin_fops;
	fd_array[1].fops_table = &stdout_fops;
	fd_array[0].flags = 1;
	fd_array[1].flags = 1;

	return 0;
}

/* get_tasks_running()
 * Input: none
 * Return: current tasks_running value
 * Effects: none, just used for exception handling
 */
uint32_t get_tasks_running() {
	return tasks_running;
}

/* set_exception_death()
 * Input: none
 * Return: none
 * Effects: used in exception handlers to set exception_death flag to true
 */
void set_exception_death() {
	exception_death = 1;
	return;
}

/* find_open_task()
 * Input: none
 * Return: open task index, -1 if fully
 * Effects: finds an open task slot for the new program to occupy
 */
int32_t find_open_task() {
	int i;
	for(i = 0; i < 6; i++) {
		// Check if empty slot, if so, return it
		if(!task_slots[i]) {
			return i;
		}
	}
	// No open slots
	return -1;
}

/* find_bottom_task()
 * Input: entry to check for a child
 * Return: entry number for bottom of task chain, -1 for error
 * Effect: find the bottom running task for a chain of tasks
 */
int32_t find_bottom_task(uint32_t task_num) {
	pcb_t* process_control_block;
	
	// If invalid task_num, error
	if((task_num < 0) || (task_num > 2))
		return -1;
		
	// If given task_num isn't running, error
	if(task_slots[task_num] == 0)
		return -1;
	
	process_control_block = get_pcb(task_num);
	// Go until no child task
	while(process_control_block->task_id_child != -1) {
		process_control_block = get_pcb(process_control_block->task_id_child);
	}
	
	return process_control_block->task_id;
}

/* switch_running_task()
 * Input: shell to switch to the bottom task of
 * Return: 0 on success, -1 on failure
 * Effect: Changes the tasks_running var to the bottom task of the shell specified
 */
int32_t switch_running_task(uint32_t task_num) {
	uint32_t new_task_num = find_bottom_task(task_num);
	
	if(new_task_num == -1) {
		return -1;
	} else {
		tasks_running = new_task_num;
		return 0;
	}
}



