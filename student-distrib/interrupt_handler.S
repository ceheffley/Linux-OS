# interrupt_handler.S - added by mlee148
# This is an assembly wraper for handler functions for various interrupts.
# For each interrupt it calls the appropriate handler function

#define ASM 1

#include "interrupt_handler.h"
#include "x86_desc.h"


#define INTERRUPT_HANDLER(handler_name, handler) \
    .globl handler_name                 	  ;\
    handler_name:                         	  ;\
        pushal                  			  ;\
        pushfl                  			  ;\
        call handler               			  ;\
        popfl                    			  ;\
        popal                    			  ;\
        iret

#interrupt handlers for pit, keyboard and rtc; calls handler functions in pit.c, keyboard.c and rtc.c
INTERRUPT_HANDLER(pit_handler_asm, pit_handler);
INTERRUPT_HANDLER(keyboard_handler_asm, keyboard_handler);
INTERRUPT_HANDLER(rtc_handler_asm, rtc_handler);
#INTERRUPT_HANDLER(syscall_handler, syscall_interrupt);
