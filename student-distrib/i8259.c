/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled
 * master_mask and slave_mask are updated by enable_irq and disable_irq */
uint8_t master_mask = 0xFB; /* IRQs 0-7 Slave port (2) starts unmasked */
uint8_t slave_mask  = 0xFF;  /* IRQs 8-15 */
uint8_t all_mask = 0xFF; /* Doesn't change, masks all IRQs for a PIC */

/* i8259_init(): Initialize the 8259 PIC  (heffley)
 * Inputs: none
 * Returns: none
 * Effects: Initializes the master and slave PICs
 */
void i8259_init(void) {
	// Mask the master and slave interrupts
	outb(all_mask, MASTER_8259_DATA);
	outb(all_mask, SLAVE_8259_DATA);

	// Send initialization commands to master and slave
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW2_MASTER, MASTER_8259_DATA);
	outb(ICW3_MASTER, MASTER_8259_DATA);
	outb(ICW4, MASTER_8259_DATA);
	
	outb(ICW1, SLAVE_8259_PORT);
	outb(ICW2_SLAVE, SLAVE_8259_DATA);
	outb(ICW3_SLAVE, SLAVE_8259_DATA);
	outb(ICW4, SLAVE_8259_DATA);
	
	// Unmask master and slave interrupts
	outb(master_mask, MASTER_8259_DATA);
	outb(slave_mask, SLAVE_8259_DATA);
}

/* enable_irq(): Enable (unmask) the specified IRQ (heffley)
 * Inputs: irq_num, valid 0-15, that tells which irq to enable
 * Returns: none
 * Effects: Enables the given irq 
 */
void enable_irq(uint32_t irq_num) {
	
	// Mask that singles out irq to be enabled
	uint8_t mask = 0xFF;
	
	// If irq_num 0-7, use master PIC
	if((irq_num >= 0) && (irq_num < 8)) {
		// Turn bit being unmasked into a 0
		mask -= (0x1 << irq_num);
		
		// & with master_mask to update and tell PIC
		master_mask &= mask;
		outb(master_mask, MASTER_8259_DATA);
	
	} else if ((irq_num >= 8) && (irq_num < 16)) {
		// If irq_num 8-15, use slave PIC
		
		// Turn bit being unmasked into a 0, subtract by 8 to make 8-15 -> 0-7 for mask of size[8]
		mask -= (0x1 << (irq_num - 8));
		
		// & with master_mask to update and tell PIC
		slave_mask &= mask;
		outb(slave_mask, SLAVE_8259_DATA);
		
	} else // Invalid irq, so just return
		return;
}

/* disable_irq() : Disable (mask) the specified IRQ  (heffley)
 * Inputs: irq_num, valid 0-15, that tells which irq to disable
 * Returns: none
 * Effects: Disables the given irq 
 */
void disable_irq(uint32_t irq_num) {
		
	// Mask that singles out irq to be disabled
	uint8_t mask = 0x00;
	
	// If irq_num 0-7, use master PIC
	if((irq_num >= 0) && (irq_num < 8)) {
		// Turn bit being masked into a 1
		mask += (0x1 << irq_num);
		
		// OR with master_mask to update and tell PIC
		master_mask |= mask;
		outb(master_mask, MASTER_8259_DATA);
	
	} else if ((irq_num >= 8) && (irq_num < 16)) {
		// If irq_num 8-15, use slave PIC
		
		// Turn bit being masked into a 1, subtract by 8 to make 8-15 -> 0-7 for mask of size[8]
		mask += (0x1 << (irq_num - 8));
		
		// OR with master_mask to update and tell PIC
		slave_mask |= mask;
		outb(slave_mask, SLAVE_8259_DATA);
		
	} else // Invalid irq, so just return
		return;
}

/* send_eoi(): Send end-of-interrupt signal for the specified IRQ (heffley)
 * Inputs: irq_num, valid 0-15, that tells which irq to signal EOI to
 * Returns: none
 * Effects: Signals EOI for given irq, if on slave, signals to the master irq which is connected to the slave
 */
void send_eoi(uint32_t irq_num) {
	/* End-of-interrupt byte gets OR'd with
	* the interrupt number and sent out to the PIC
	* to declare the interrupt finished */
	
	// If irq_num 0-7, use master PIC
	if((irq_num >= 0) && (irq_num < 8)) {
		outb((EOI | irq_num), MASTER_8259_PORT);
	} else if ((irq_num >= 8) && (irq_num < 16)) {
		// If irq_num 8-15, use slave PIC
		// irq_num - 8 as slave sees irqs 8-15 as 0-7
		outb((EOI | (irq_num - 8)), SLAVE_8259_PORT);
		
		// Need to also send EOI to master at port slave is attached to, which is irq2 on master, hence the 0x2
		outb((EOI | 0x2), MASTER_8259_PORT);
	} else // Invalid irq, so just return
		return;
}
