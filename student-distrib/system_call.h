/* system_call.h
 */

#ifndef _SYSTEM_CALL_H
#define _SYSTEM_CALL_H

#include "types.h"

#define SPACE_CHAR 0x20
#define NULL_CHAR 0x00


// The four bytes returned if file is ELF
#define ELF_CHECK0 0x7f
#define ELF_CHECK1 0x45
#define ELF_CHECK2 0x4C
#define ELF_CHECK3 0x46

#define TASKNAME_SIZE 33
#define ARGUMENT_SIZE 129

#define oneK 1024
#define fourK 4096
#define eightK 8192

#define program_addr 0x08048000
#define eip_offset 6
#define twoM 0x200000

typedef struct fops_table_t {

	int32_t (*open)(int32_t*, char*);
	int32_t (*close)(int32_t*);
	int32_t (*read)(int32_t*, uint32_t*, char*, uint32_t);
	int32_t (*write)(int32_t*, uint32_t*, char*, uint32_t);

}fops_table_t;

typedef struct file_descriptor_t {

    fops_table_t* fops_table;          //pointer to file operations table
    int32_t inode;
    uint32_t file_position;
    int32_t flags;

}file_descriptor_t;

typedef struct pcb_t {

    //doubly linked list for parent/child tasks -- currently unused
    /*struct pcb_t* child;       //pointer to child task
    struct pcb_t* parent;*/      //pointer to parent task

    //file descriptor array
    file_descriptor_t fd [8];                 //8 is max number of files per task
    uint8_t file_name [TASKNAME_SIZE];                 //human readable name for tasks

    //task id of current task and parent/child task if there is one
    uint32_t task_id;
    uint32_t task_id_child;
    uint32_t task_id_parent;

    //useful flags -- currently unused
    /*uint32_t has_child;
    uint32_t has_parent;*/

    //return pointers
    uint32_t old_ebp;
    uint32_t old_esp;
    uint32_t old_eip;

    //buffer for arguments of this task
    uint8_t argument_buf [ARGUMENT_SIZE];              //128 is max argument length, +1 for newline char



}pcb_t;


int32_t syscall_halt(uint8_t status);
int32_t syscall_execute(const uint8_t* command);
int32_t syscall_read (uint32_t fd, void* buf, int32_t nbytes);
int32_t syscall_write(uint32_t fd, void* buf, int32_t nbytes);
int32_t syscall_open(const uint8_t* filename);
int32_t syscall_close(int32_t fd);
int32_t syscall_getargs(uint8_t* buf, int32_t nbytes);
int32_t syscall_vidmap(uint8_t ** screen_start); 
pcb_t* get_pcb(uint32_t grab_task_id);
int32_t fda_init();
int32_t find_open_task();
int32_t find_bottom_task(uint32_t task_num);

extern uint32_t get_tasks_running();
extern void set_exception_death();
extern int32_t switch_running_task(uint32_t task_num);

#endif /* _SYSTEM_CALL_H */
