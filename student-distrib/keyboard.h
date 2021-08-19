/* keyboard.h
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"
#include "system_call.h"

// See PS/2 Controller page on osdev
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_REG  0x64
#define KEYBOARD_CMD_REG  0x64
#define press_release_offset  0x80
#define leftshift 0x2A
#define rightshift 0x36
#define leftCtrl 0x1D
#define leftAlt 0x38
#define CapsLock 0x3A
#define function1_key 0x3B
#define function2_key 0x3C
#define function3_key 0x3D
#define TERMINAL_COUNT 3
#define TERMINAL_SIZE 4000

// keyboard sits on irq port 1
#define KEYBOARD_IRQ_NUM   0x1

#define KEYBOARD_MAX_BUFFER  128   //maximum number of characters allowed in buffer

extern void keyboard_init(void);
extern void keyboard_handler(void);
void terminal_buf_add(uint8_t char_to_print);
void clear_screen();
void terminal_backspace();


//terminal struct
typedef struct terminal_t{



 uint8_t keyboard_buffer[KEYBOARD_MAX_BUFFER];
 uint32_t cursor_pos_x;                   //current cursor position 0-79
 uint32_t cursor_pos_y;
 int keyboard_buffer_size;              //current buffer size 0-127
 int allow_terminal_read;               //1 to enable keyboard input, 0 to disable
 char terminal_buffer[TERMINAL_SIZE];            //row*col*2

 uint32_t ebp, esp;

}terminal_t;

terminal_t terminal[TERMINAL_COUNT];
int32_t display_terminal_id;


int32_t terminal_open(int32_t* ignore, char* filename);
int32_t terminal_close(int32_t* fd);
int32_t terminal_read(int32_t* fd, uint32_t* ignore, char* buf, uint32_t nbytes);
int32_t terminal_write(int32_t* fd, uint32_t* ignore, char* buf, uint32_t nbytes);
void switch_display_terminal(uint32_t tid);


//void add_to_buffer(uint32_t press_code, uint32_t index);

extern fops_table_t stdin_fops;
extern fops_table_t stdout_fops;

#endif /* _KEYBOARD_H */
