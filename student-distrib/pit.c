/* pit.c
 */

#include "pit.h"
#include "i8259.h"
#include "lib.h"
#include "scheduler.h"

int i = 0;

/* pit_init()
 * Input: none
 * Return: none
 * Effect: Initializes and enables the PIT for scheduling
 */
void pit_init() {
	// Disable interrupts
	cli();
	
	//Initialize PIT into proper mode
	outb(PIT_COMMAND, PIT_CMD_REG);
	//Set frequency, set low byte first, then high byte
	outb(RELOAD_VAL & PIT_MASK, PIT_CHANNEL0);
	outb(RELOAD_VAL >> PIT_SHIFT, PIT_CHANNEL0);
	
	// Enable pit irq
	enable_irq(PIT_IRQ_NUM);
	
	// Enable interrupts
	sti();
	return;
}

/* pit_handler()
 * Input: none
 * Return: none
 * Effect: Called when PIT signals interrupt, sends eoi to PIC
 */
void pit_handler() {
	cli();
	//round robin through three terminals
	scheduler(i);
	i++;
	if(i > 2)
		i = 0;
	sti();
	// Signal interrupt ended when finished handling
	send_eoi(PIT_IRQ_NUM);
	return;
}


