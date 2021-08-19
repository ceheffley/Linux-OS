/* pit.h
 */

#ifndef _PIT_H
#define _PIT_H

#include "types.h"
#include "system_call.h"

// Values from OSdev
#define PIT_IRQ_NUM  0x00
#define PIT_CHANNEL0 0x40
#define PIT_CMD_REG  0x43
#define PIT_COMMAND  0x36 //channel 0, lobyte/hibyte enabled, use mode 3 as recommended in OSdev

#define TERMINAL_COUNT 3

// reload_val = 1193182/frequency, we are using frequency of 50 Hz
#define RELOAD_VAL   23863
// Need a mask for first byte for sending reload val, since only takes one byte at a time
#define PIT_MASK     0xFF
// Also need to shift by a byte for high byte
#define PIT_SHIFT	 8


void pit_init();
void pit_handler();

#endif /* _PIT_H */
