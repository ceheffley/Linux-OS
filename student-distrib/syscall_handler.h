#ifndef _SYSCALL_HANDLER_H_
#define _SYSCALL_HANDLER_H_

#include "system_call.h"

#ifndef ASM
    extern void syscall_handler_asm();
#endif

#endif
