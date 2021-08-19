#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc.h"
#include "keyboard.h"
#include "fs_module.h"
#include "system_call.h"
#include "paging.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) &&
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* pagefault exception test
 *
 * trigger a page fault by accessing bad memory
 * Inputs: None
 * Outputs: None
 * Side Effects: trigger a pagefault
 * Files: x86_desc.h/S, interrupt_tble.c/h
 */
int pagefault_test(){
	uint32_t * ptr = (uint32_t*) (0x00000008);
	int x;
	x = *ptr;
	return 0;
}
/* IDT divide by zero test - added by mlee148
 *
 * compute a divide by zero
 * Inputs: None
 * Outputs: None
 * Side Effects: trigger a divide by zero exception
 * Files: x86_desc.h/S, interrupt_tble.c/h
 */
void idt_test_divide_zero(){
	TEST_HEADER;
	printf("Testing divide by zero exception:\n");
	int a = 5;
	int b = 0;
	int c;
	c = a/b;

}

// add more tests here
/* paging setup Test
 *
 * Access allocated page memory, see if pagefault
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: paging_init
 * Files: paging.c/h
 */
int paging_test(){
	TEST_HEADER;
	int i;
	uint32_t *ptr;
	int x;
	int result = PASS;
	for (i = 0; i <128; ++i) {
		ptr = (uint32_t*) (0xb8000+i*32);
		x = *ptr;
	}


	ptr = (uint32_t*) (0x400000+4);
	x = *ptr;
	ptr = (uint32_t*) (0x800000-4);
	x = *ptr;

	return result;

}
/* dereferencing a null ptr
 *
 * see if pagefault
 * Inputs: None
 * Outputs: none
 * Side Effects: pagefault
 *
 */
void dereferencing_null(){
	TEST_HEADER;
	int *ptr = NULL;
	int x;
	x = *ptr;
}

/* keyboard test
 * Spins and lets keyboard write to screen on interrupt
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Writes characters to the screen
 * Coverage: keyboard
 * Files: keyboard.c/h
 */
// int keyboard_test(){
	// clear();
	// TEST_HEADER;
	// int result = PASS;
	// keyboard_init();
	// terminal_open(NULL);
	// uint8_t buf[128];
	// int retval;
	// while(1){
	// retval=terminal_read(0, buf, 128);
    // printf("bytes read: %d \n", retval);
	// }

	// return result;
// }

/* rtc test
 * Spins and lets rtc interrupt, which calls test_interrupts()
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Writes characters to the screen
 * Coverage: rtc
 * Files: rtc.c/h
 */
int rtc_test(){
	TEST_HEADER;
	int result = PASS;
	rtc_init();
	clear();
	while(1){
	}

	return result;
}

/* Checkpoint 2 tests */

/* File system read small file test - added by mlee148
 *
 * Reads and prints frame0.txt and/or frame1.txt
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Print small text file
 * Files: fs_module.c/h
 */
int fs_test_read_small(){

	//TEST_HEADER;

    dentry_t file0;
	//dentry_t file1;

    read_dentry_by_name("frame0.txt", &file0);
	//read_dentry_by_name("frame1.txt", &file1);
		//return FAIL;

	char buf[200];
	read_data(file0.inode, 0, buf, 200);
		//return FAIL;
	//printf("read_date returned: %d\n", result);
	int i = 0;
	for(i = 0; i < 187; i++)
		putc(buf[i]);

	//result = read_data(file1.inode, 0, buf, 174);

	return PASS;
}

/* File system read large file test - added by mlee148
 *
 * Reads and prints large files
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Print section of large text file
 * Files: fs_module.c/h
 */
int fs_test_read_large(){
	dentry_t file0;
	if(read_dentry_by_name("verylargetextwithverylongname.tx", &file0) == -1)
		return -1;

	char buf[5277];
	read_data(file0.inode, 0, buf, 5277);

	int i = 0;
	for(i = 0; i < 200; i++)
		putc(buf[i]);

	return PASS;
}

/* File system read executable file test - added by mlee148
 *
 * Reads and prints executable files
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Print section of executable file
 * Files: fs_module.c/h
 */
int fs_test_read_exe(){
	dentry_t file0;
	if(read_dentry_by_name("ls", &file0) == -1)
		return -1;

	char buf[5349];
	read_data(file0.inode, 0, buf, 5349);

	int i = 0;
	for(i = 0; i < 50; i++)
		putc(buf[5298+i]);

	return PASS;
}

/* File directory list test - added by mlee148
 *
 * reads and prints all valid files
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Print list of files
 * Files: fs_module.c/h
 */
int fs_test_list_dir(){

	char buf[MAX_FILENAME + 1];

	printf("File Directory\n\n");

	int i = 0;
	for(i = 0; i < MAX_FILENAME; i++){

		int result = directory_read(i, buf, MAX_FILENAME);
		if(result == 0)
			break;

		buf[result] = '\0';
		printf("file name: ");
		printf(buf);
		printf("\n");

	}
	return PASS;
}

/* rtc read write test
 * Inputs: None
 * Outputs: PASS
 * Side Effects: Writes characters to the screen at rate of every power of 2, 2->1024
 * Coverage: rtc read write functions
 * Files: rtc.c/h
 */
int rtc_read_write_test() {
	TEST_HEADER;
	int result = PASS;
	int chars_in_line;
	uint32_t i, j;
	char z = 2;
	for(i = 0; i < 10; i++) {
		chars_in_line = 0;
		clear();
		reset_position();
		rtc_write(0, 0, &z, 4);
		for(j = 0; j <= z; j++) {
			if (chars_in_line >= 80) {
				putc('\n');
				chars_in_line = 0;
			}
			rtc_read(0, 0, 0, 0);
			putc(i + 65);
			chars_in_line++;
		}
		z *= 2;
	}
	return result;
}

/* rtc open close test
 * Inputs: None
 * Outputs: PASS if close returns 0, FAIL if doesnt
 * Side Effects: Writes characters to the screen at rate of 2 -> 4 -> 2, using rtc_open() to set to 2
 * Coverage: rtc read write open close functions
 * Files: rtc.c/h
 */
int rtc_open_close_test() {
	clear();
	reset_position();
	TEST_HEADER;
	int i;
	char write_val = 4;
	rtc_open(0, 0);
	for(i = 0; i < 10; i++) {
		rtc_read(0, 0, 0, 0);
		putc('1');
	}
	rtc_write(0, 0, &write_val, 4);
	for(i = 0; i < 10; i++) {
		rtc_read(0, 0, 0, 0);
		putc('2');
	}
	rtc_open(0, 0);
	for(i = 0; i < 10; i++) {
		rtc_read(0, 0, 0, 0);
		putc('3');
	}
	if(!rtc_close(0)) {
		return PASS;
	} else {
		return FAIL;
	}

}

// int terminal_driver_test()
// {

	// int i;
	// int32_t write_return;
	// uint8_t buf[128];
	// for (i=0; i<128;i++){
		// buf[i]=0;
	// }
	// buf[0]='3';
	// buf[1]='9';
	// buf[2]='1';
	// buf[3]='\n';
	// write_return=terminal_write(NULL, buf, 128);
	// printf("wrote %d bytes",write_return);

	// return PASS;
// }


/* Checkpoint 3 tests */

int system_execute_test()
{
	uint8_t program[] = "ls filler.txt";
	syscall_execute(program);

	return PASS;
}

int rwoc_test() {

	syscall_execute((const uint8_t*)"shell");
	char buf[200];
	int ret = 0;
	int fdi = syscall_open((const uint8_t*)"frame1.txt");
	//pcb_t* pcb = get_pcb(syscall_execute((const uint8_t*)"shell"));
	syscall_read(fdi, buf, 200);
	//read_data(file0.inode, 0, buf, 200);
		//return FAIL;
	//printf("read_date returned: %d\n", result);
	int i = 0;
	for(i = 0; i < 187; i++)
		putc(buf[i]);

	syscall_close(fdi);
	ret = syscall_close(fdi);
	printf("second close: %d", ret);

	return 0;
}
/* Checkpoint 4 tests */
/*
vidmap test
check validity of vidmap allocated memory
*/
int vidmap_test(){
	TEST_HEADER;
	int ret = 1;
	uint8_t * addr;
	syscall_vidmap(&addr);
	if ((uint32_t) addr != _148MB)
		ret = 0;
	uint8_t *ptr = addr +4;
	uint8_t x;
	x = *ptr;

	return ret;

}
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	//EST_OUTPUT("idt_test", idt_test());
	//TEST_OUTPUT("paging_test", paging_test());

	//TEST_OUTPUT("keyboard_test", keyboard_test());
	//TEST_OUTPUT("rtc_test", rtc_test());
 	//idt_test_divide_zero();

	//dereferencing_null();

	// TEST_OUTPUT("pagefault_test", pagefault_test());


	/* CP2 TESTS */

	/* RTC TESTS */
	//TEST_OUTPUT("rtc_read_write_test", rtc_read_write_test());
	//TEST_OUTPUT("rtc_open_close_test", rtc_open_close_test());

	//CP2

	//fs_test_read_small();
	//fs_test_read_large();
	//fs_test_read_exe();
	//fs_test_list_dir();
 //TEST_OUTPUT("terminal_driver_test",terminal_driver_test());
	// launch your tests here
	//system_execute_test();
	TEST_OUTPUT("vidmap_test", vidmap_test());
	// rwoc_test();
}
