/* rtc.c
 */

#include "keyboard.h"
#include "i8259.h"
#include "lib.h"
#include "system_call.h"
#include "scheduler.h"

fops_table_t stdin_fops = {
	.open = terminal_open,
	.close = NULL,
	.read = terminal_read,
	.write = NULL,
};

fops_table_t stdout_fops = {
	.open = terminal_open,
	.close = NULL,
	.read = NULL,
	.write = terminal_write,
};

// Last letter is 0x3A = 59, so need size 51
// Size will change for when need to handle more than just letters + numbers
// Added a few extras other than required for checkpoint 12 some missing though
//"lowercase, Shift, uppercase, key being pressed "
uint8_t scan_codes[61][4] = {
	{0, 0,0,0},
	{0, 0,0,0},
	{'1', '!', '1',0},
	{'2', '@', '2',0},
	{'3', '#', '3',0},
	{'4', '$', '4',0},
	{'5', '%', '5',0},
	{'6', '^', '6',0},
	{'7', '&', '7',0},
	{'8', '*', '8',0},
	{'9', '(', '9',0},
	{'0', ')', '0',0},
	{'-', '_', '-',0},
	{'=', '+','=',0},
	{0, 0,0,0},
	{'\t', '\t', '\t',0},
	{'q', 'Q','Q',0},
	{'w', 'W','W',0},
	{'e', 'E', 'E',0},
	{'r', 'R', 'R',0},
	{'t', 'T', 'T',0},
	{'y', 'Y', 'Y',0},
	{'u', 'U','U',0},
	{'i', 'I','I',0},
	{'o', 'O','O',0},
	{'p', 'P','P',0},
	{'[', '{','[',0},
	{']', '}',']',0},
	{'\n', '\n','\n',0},
	{0, 0,0,0},
	{'a', 'A','A',0},
	{'s', 'S','S',0},
	{'d', 'D','D',0},
	{'f', 'F','F',0},
	{'g', 'G','G',0},
	{'h', 'H','H',0},
	{'j', 'J','J',0},
	{'k', 'K','K',0},
	{'l', 'L','L',0},
	{';', ':',';',0},
	{'\'', '"', '\'',0},
	{'`', '~','`',0},
	{0, 0,0,0},
	{'\\', '|', '\\' ,0},
	{'z', 'Z','Z',0},
	{'x', 'X','X',0},
	{'c', 'C','C',0},
	{'v', 'V','V',0},
	{'b', 'B','B',0},
	{'n', 'N','N',0},
	{'m', 'M','M',0},
	{',','<', ',',0},
	{'.', '>', '.',0},
	{'/', '?', '/',0},
	{'\0', '\0', '\0',0},
	{'\0', '\0', '\0',0},
	{'\0', '\0', '\0',0},
	{0x20, 0x20, 0x20, 0},
	{'\0', '\0', '\0',0},
	{'\0', '\0', '\0',0},
	{'\0', '\0', '\0',0}
};


/* keyboard_init() (heffley)
 * Inputs: none
 * Returns: none
 * Effects: Enables keyboard irq
 */
void keyboard_init(void) {
	// Enable keyboard irq on PIC
	enable_irq(KEYBOARD_IRQ_NUM);
}

/* keyboard_handler() (heffley) (xunli3)
 * Inputs: none
 * Outputs: none
 * Effects: updated for checkpoint 2, will handle uppercase switching and function keys, also checks for invalid inputs
 */
 void keyboard_handler(void) {
 	// Recieve key pressed
 	uint32_t press_code = inb(KEYBOARD_DATA_PORT);
 	uint8_t char_to_print;
 	char_to_print = '\0';

	// check whether the press_code is within bonds
 	if ((press_code <0 ) || (press_code > 0xBD)){
		send_eoi(KEYBOARD_IRQ_NUM);
		return;
	}
 	if ((press_code >0x3D) && (press_code < 0x81)) {
		send_eoi(KEYBOARD_IRQ_NUM);
		return;
	}

	// record key press
 	if (press_code <= function3_key){
 		scan_codes[press_code][3] = 1;
 	}
 	else{
	// record key release
 		if (press_code != 0xBA)
 			scan_codes[press_code-press_release_offset][3] = 0;
 	}

	// after key press check whether capslock is on
 	if ( press_code <= CapsLock){
 	if (press_code == CapsLock){
 		if (scan_codes[press_code][3] ==1)
 			scan_codes[press_code][3] = 0;
 		else
 			scan_codes[press_code][3] = 1;
 	}

	// when capslock is off
 	if (scan_codes[CapsLock][3] ==0) {
		//shift is on
 		if (scan_codes[leftshift][3]== 1 || scan_codes[rightshift][3] ==1){
 				char_to_print = scan_codes[press_code][1];
 		}
 		else
 				char_to_print = scan_codes[press_code][0];
 	}
	// when capslock is on
 	if (scan_codes[CapsLock][3] ==1){
		// when shift is also on
 		if (scan_codes[leftshift][3]== 1 || scan_codes[rightshift][3] ==1){
			//when the char to print is letters
 				if (scan_codes[press_code][1] == scan_codes[press_code][2])
 					char_to_print = scan_codes[press_code][0];
 				else
				// print lowercase
 					char_to_print = scan_codes[press_code][1];
 		}
 		else
 			char_to_print = scan_codes[press_code][2];
 	}
 	}
 	//////////////////////////////////////////////////////////
 	//check for special circumstances, ctrl+i and ctrl+l
 	if ((scan_codes[leftCtrl][3] == 1) && (scan_codes[0x26][3] ==1) ){
 		clear_screen();
		 
 	}
 	else if (press_code == 0x0E ){
 		terminal_backspace(); 
 	}
	
	else if ((scan_codes[leftAlt][3] == 1) && (scan_codes[function1_key][3] ==1)){
		switch_display_terminal(0);
	}

	else if ((scan_codes[leftAlt][3] == 1) && (scan_codes[function2_key][3] ==1)){
		switch_display_terminal(1);
	}

	else if ((scan_codes[leftAlt][3] == 1) && (scan_codes[function3_key][3] ==1)){
		switch_display_terminal(2);
	}


	//print the char
 	else if ( char_to_print != '\0')
 		terminal_buf_add (char_to_print);
 	// Send EOI to PIC
 	send_eoi(KEYBOARD_IRQ_NUM);
 }



 /* clear_screen()
  * Inputs: none
  * Outputs: none
  * Effects: clear video memory
  */
 void clear_screen(){
 	clear();
	reset_position();

 }
 /* terminal_backspace()
  * Inputs: none
  * Outputs: none
  * Effects: creaate backspace effect
  */
void terminal_backspace(){
	 char space = ' ';
	if (terminal[display_terminal_id].keyboard_buffer_size == 0)
		return;
	// int scr_x;
	// int scr_y;
	// getScreenPos(scr_x, scr_y);
	// printf("%d, %d", scr_x, scr_y );
	// scr_x --;
	// setScreenPos(scr_x, scr_y);
	back_space();
	terminal[display_terminal_id].keyboard_buffer[terminal[display_terminal_id].keyboard_buffer_size] = '\0';
	terminal[display_terminal_id].keyboard_buffer_size --;
	printf("%c", space );
	back_space();

}

/* terminal_open() (xunli3)
 * Inputs: none
 * Outputs: none
 * Effects: initializes stuff
 */
int terminal_open(int32_t* ignore, char* filename){
	int i,j;   //looping variable

	display_terminal_id=0;
	for (j=0; j<TERMINAL_COUNT; j++){
	terminal[display_terminal_id].allow_terminal_read=0;
	terminal[display_terminal_id].cursor_pos_x=0;
	terminal[display_terminal_id].cursor_pos_y=0;
	terminal[display_terminal_id].keyboard_buffer_size = 0;
	
	

	for (i=0; i<KEYBOARD_MAX_BUFFER; i++){
		terminal[display_terminal_id].keyboard_buffer[i]= '\0';    //initializes keyboard buffer to null string
	}

	}

	return 0;
}

/* terminal_close() (xunli3)
 * Inputs: none
 * Outputs: none
 * Effects: Closes the terminal, but this will never be called
 */
int terminal_close(int32_t* fd){

return 0;
}



/* terminal_read() (xunli3)
 * Inputs: none
 * Outputs: number of bytes read from keyboard buffer
 * Effects: resets keyboard buffer
 */
int terminal_read(int32_t* fd, uint32_t* ignore, char* buf, uint32_t nbytes){
	int i;
	int numbytes=0;

	//spins until terminal is allowed to read the keyboard buffer after the user presses enter
	while (!terminal[display_terminal_id].allow_terminal_read);


    												/*cli and sti so that no inputs are allowed
														when user terminal reads from keyboard buffer*/
	cli();
	for (i=0; i<nbytes; i++){
		buf[i]=terminal[display_terminal_id].keyboard_buffer[i];
		terminal[display_terminal_id].keyboard_buffer[i]='\0';
		if (buf[i]!='\0'){
        numbytes++;
        } else 
			break;
	}

							
	terminal[display_terminal_id].keyboard_buffer_size=0;
	terminal[display_terminal_id].allow_terminal_read=0;				//stops terminal_read when user is ready to input next command

	sti();
	return numbytes;

}

/* terminal_write() (xunli3)
 * Inputs: none
 * Outputs: number of bytes written
 * Effects: print whatever passes through buffer on the screen
 */
int terminal_write(int32_t* fd, uint32_t* ignore, char* buf, uint32_t nbytes){
	int i;
	int numbytes=0;


	for (i=0; i<nbytes; i++){
		putc(buf[i]);

		if (buf[i]!='\n'){
        numbytes++;
        }

	}

	return numbytes;

}

/* terminal_buf_add(uint8_t char_to_print)
 * Inputs: char to be printed
 * Outputs: none
 * Effects: print the char from keyboard in terminal.
 */
void terminal_buf_add(uint8_t char_to_print){
	// see if keyboard_size is over
	if (terminal[display_terminal_id].keyboard_buffer_size >= KEYBOARD_MAX_BUFFER -2) {
		return;
	}

	
	// signals terminal read
	if (char_to_print == '\n'){
	 //while(get_active_terminal() != find_bottom_task(display_terminal_id)){}
	 putc(char_to_print);
	 terminal[display_terminal_id].keyboard_buffer[terminal[display_terminal_id].keyboard_buffer_size] = char_to_print;
	 terminal[display_terminal_id].keyboard_buffer_size++;
	 terminal[display_terminal_id].allow_terminal_read = 1;
	}

	else{
	 putc(char_to_print);
	 terminal[display_terminal_id].keyboard_buffer[terminal[display_terminal_id].keyboard_buffer_size] = char_to_print;
	 terminal[display_terminal_id].keyboard_buffer_size++;
	}
	// int y = getYpos();
	//
	// if (y >= NUM_ROWS -1){
	// 	scroll_up();
	// }
	
	// if (terminal[display_terminal_id].keyboard_buffer_size == 80){
	// 	printf("%c", '\n');
	// }


}

/* switch_display_terminal()
 * Input: terminal id to switch to
 * Return: none
 * Effect: Switches the task that is being displayed on the screen from one terminal to a task from
 *			a different terminal. 
 */
void switch_display_terminal(uint32_t tid){
	if (display_terminal_id==tid || tid <0 || tid>2){
		return;
	}

	//remap paging to new process
	syscall_paging_setup(eightM + (fourM * find_bottom_task(tid)));

	pcb_t* prev_pcb = get_pcb(display_terminal_id);
	pcb_t* next_pcb = get_pcb(tid);

	// Save current terminal's buffer
	cpy_screen_pos();
	memcpy(terminal[display_terminal_id].terminal_buffer, (char*)VIDEO, TERMINAL_SIZE);

	//Prep for task switch
	tss.ss0 = KERNEL_DS;
	tss.esp0 = eightM - (eightK * find_bottom_task(tid)) - 4;

	//Update the currently displayed terminal id variable
	display_terminal_id= tid;

	//Move the new terminal's buffer into video mem
	memcpy((char*)VIDEO, terminal[display_terminal_id].terminal_buffer, TERMINAL_SIZE);

	//Update cursor position
	setScreenPos(terminal[display_terminal_id].cursor_pos_x, terminal[display_terminal_id].cursor_pos_y);

	//Switch the task to a different terminal's
	switch_running_task(tid);
	
	asm volatile (
	"movl %%esp, %0 \n"
	"movl %%ebp, %1 \n"
	:"=r" (prev_pcb->old_esp), "=r" (prev_pcb->old_ebp)
	:
	);
	
	asm volatile (
	"movl %0, %%esp \n"
	"movl %1, %%ebp \n"
	:"=r" (next_pcb->old_esp), "=r" (next_pcb->old_ebp)
	:
	);
}


