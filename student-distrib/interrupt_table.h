//interrupt_table.h - added by mlee148, task mod by heffley
//header file for interrupt_table.c

#include "x86_desc.h"
#include "interrupt_handler.h"

#ifndef _INTERRUPT_TABLE_H_
#define _INTERRUPT_TABLE_H_


#ifndef ASM

	//initialize interrupt table
	void init_idt();
	
	//handle exception macro: prints error message and holds while loop
	//also checks if a task is running, if it is, squash it
    #define HANDLE_EXCEPTION(exception_name, message) \
        void exception_name(){                        \
            printf(message);                		  \
			if(get_tasks_running()){putc('\n'); set_exception_death(); syscall_halt(0);}\
            while(1) {}                    			  \
        }											  \

#endif


#endif
