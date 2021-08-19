/* rtc.h
 */

#ifndef _RTC_H
#define _RTC_H

#include "types.h"
#include "system_call.h"


// IO ports, see wiki.osdev.org/RTC
#define RTC_PORT 	0x70
#define CMOS_PORT   0x71

// Offset of A-C in CMOS RAM, plus 0x80 bit set to disable non-maskable-interrupt
#define STATUS_REG_A   0x8A
#define STATUS_REG_B   0x8B
#define STATUS_REG_C   0x8C

// RTC sits on irq port 8
#define RTC_IRQ_NUM   0x08

// The code for 1024 Hz, the max amount allowed, is 0110, see in data sheet
#define MAX_HZ 0x06
// The rest of the rtc codes for valid Hz values, see data sheet
#define RTC_2_HZ 0x0F
#define RTC_4_HZ 0x0E
#define RTC_8_HZ 0x0D
#define RTC_16_HZ 0x0C
#define RTC_32_HZ 0x0B
#define RTC_64_HZ 0x0A
#define RTC_128_HZ 0x09
#define RTC_256_HZ 0x08
#define RTC_512_HZ 0x07
#define RTC_1024_HZ 0x06

extern void rtc_init(void);
extern void rtc_handler(void);
int32_t rtc_write(int32_t* fd, uint32_t* i, char* buf, uint32_t nbytes);
int32_t rtc_read(int32_t* fd, uint32_t* i, char* buf, uint32_t nbytes);
int32_t rtc_open(int32_t* i, char* filename);
int32_t rtc_close(int32_t* fd);
uint32_t rtc_set_frequency(uint32_t new_frequency);

extern fops_table_t rtc_fops;

#endif /* _RTC_H */
