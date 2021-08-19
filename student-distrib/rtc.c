/* rtc.c
 */

#include "rtc.h"
#include "i8259.h"
#include "lib.h"
#include "system_call.h"

// Virtualized frequency of RTC, RTC running at 1024 Hz
uint32_t virt_freq;
// Count of interrupts, used to see if reached virt_freq's period
uint32_t rtc_inter_count; 

fops_table_t rtc_fops = {
	.open = rtc_open,
	.close = rtc_close,
	.read = rtc_read,
	.write = rtc_write
};

/* rtc_init() (heffley)
 * Inputs: None
 * Returns: None
 * Effects: Initializes the RTC and enables its irq line
 */
void rtc_init(void) {
	// Disable interrupts
	cli();
	
	//Select register B, and disable NMI
	outb(STATUS_REG_B, RTC_PORT);
	// Read the current value of register B
	char prev = inb(CMOS_PORT);
	// Set the index again
	outb(STATUS_REG_B, RTC_PORT);
	// Write the previous value ORed with 0x40 to turn on bit 6 of reg_b
	outb(prev | 0x40, CMOS_PORT);
	
	// Set frequency to max (1024) Hz
	rtc_set_frequency(1024);
	
	// Set virtualized frequency to be 2 Hz by default
	virt_freq = 2;
	
	// Set rtc_int_count to 0, as no interrupts have happened
	rtc_inter_count = 0;
	
	// Enable rtc irq
	enable_irq(RTC_IRQ_NUM);
	
	// Enable interrupts
	sti();
}

/* rtc_handler() (heffley)
 * Inputs: none
 * Returns: none
 * Effects: Called when rtc generates interrupts, sets up for next interrupt
 */
void rtc_handler(void) {
	// Increase interrupt count
	rtc_inter_count++;


	// Need to read register C for future interrupts to occur
	outb(STATUS_REG_C, RTC_PORT);
	inb(CMOS_PORT);

	// Interrupt ended, so tell PIC
	send_eoi(RTC_IRQ_NUM);	
}


/* rtc_write() (heffley)
 * Inputs: fd unused, buf ptr containing new frequency, nbytes to check if 4 bytes
 * Returns: 0 on success, -1 on failure
 * Effects: Changes the frequency at which the real time clock generates interrupts
 */
int32_t rtc_write(int32_t* fd, uint32_t* i, char* buf, uint32_t nbytes) {
	// Check if nbytes = 4, if not, invalid
	if(nbytes != 4) {
		return -1;
	}
	
	//return rtc_set_frequency(*(uint32_t*)buf);
	// Check if input is a valid power of 2, otherwise return failure
	if((*(uint32_t*)buf <= 1024) && (*(uint32_t*)buf >= 2) && !(*(uint32_t*)buf & (*(uint32_t*)buf - 1))) {
		// Valid power of 2, reset interrupt count and set frequency
		rtc_inter_count = 0;
		virt_freq = *(uint32_t*)buf;
		return 0;
	}
	// Invald input, so return failure
	return -1;
}

/* rtc_read() (heffley)
 * Inputs: fd, buf ptr, nbytes, all unused
 * Returns: 0 on completed interrupt
 * Effects: returns when interrupt is completed
 */
int32_t rtc_read(int32_t* fd, uint32_t* i, char* buf, uint32_t nbytes) {
	
	// Set interrupt count to 0
	rtc_inter_count = 0;

	// Spin until 1024/frequency interrupts have occured
	while(rtc_inter_count < (1024 / virt_freq)) {}
	
	return 0;
}


/* rtc_open() (heffley)
 * Inputs: filename ptr, unused
 * Returns: 0 on success (always)
 * Effects: Sets the frequency to 2 Hz
 */
int32_t rtc_open(int32_t* i, char* filename) {
	virt_freq = 2; 
	return 0;
}

/* rtc_close() (heffley)
 * Inputs: fd, unused
 * Returns: 0 on success (always)
 * Effects: 
 */
int32_t rtc_close(int32_t* fd) {
	return 0;
}


/* rtc_set_frequency() (heffley)
 * Inputs: new_frequency
 * Returns: 0 on successful new frequency, -1 if invalid frequency
 * Effects: Updates the frequency of the RTC
 */
uint32_t rtc_set_frequency(uint32_t new_frequency) {
	uint8_t send_rtc_freq;
	
	// Fill in value to send to RTC with new_frequency, if invalid frequency, return error
	switch(new_frequency) {
		case 2: 
			send_rtc_freq = RTC_2_HZ;
			break;
		case 4: 
			send_rtc_freq = RTC_4_HZ;
			break;
		case 8:
			send_rtc_freq = RTC_8_HZ;
			break;
		case 16: 
			send_rtc_freq = RTC_16_HZ;
			break;
		case 32:
			send_rtc_freq = RTC_32_HZ;
			break;
		case 64:
			send_rtc_freq = RTC_64_HZ;
			break;
		case 128: 
			send_rtc_freq = RTC_128_HZ;
			break;
		case 256:
			send_rtc_freq = RTC_256_HZ;
			break;
		case 512:
			send_rtc_freq = RTC_512_HZ;
			break;
		case 1024:
			send_rtc_freq = RTC_1024_HZ;
			break;
		default: return -1;
	}
	cli();
	// Grab REG_A and change last 4 bits to 1s to make new rate
	outb(STATUS_REG_A, RTC_PORT);
	uint8_t prev = inb(CMOS_PORT);
	outb(STATUS_REG_A, RTC_PORT);
	// Send old REG_A, with last 4 bits set to MAX_HZ, back to REG_A
	outb((prev & 0xF0) | send_rtc_freq, CMOS_PORT);
	sti();
	return 0;
}



