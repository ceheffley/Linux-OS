//interrupt_handler.h - added by mlee148
//header file for interrupt_handler.S

#ifndef _INTERRUPT_HANDLER_H_
#define _INTERRUPT_HANDLER_H_

#ifndef ASM
	extern void pit_handler_asm();
    extern void keyboard_handler_asm();
    extern void rtc_handler_asm();
#endif

#endif
