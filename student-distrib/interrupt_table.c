/*
	interrupt_table.c - added by mlee148
	
	This initializes the IDT with vectors and descriptions for each interrupt.
	Then it loads the IDT into the kernel.
	Also invokes handlers for each exception
*/

#include "interrupt_table.h"
#include "lib.h"
#include "i8259.h"

#include "syscall_handler.h"

#define VECTOR_PIT			0x20
#define VECTOR_KEYBOARD		0x21
#define VECTOR_RTC			0x28
#define VECTOR_SYSCALL		0x80

//Invoke handlers for each exception; for CP1 simply display exception and hold while loop
HANDLE_EXCEPTION(exception_divide_error, "DIVIDE ERROR");
HANDLE_EXCEPTION(debug_intel_only, "DEBUG INTEL ONLY");	//debug only
HANDLE_EXCEPTION(exception_nmi_interrupt, "NMI INTERRUPT");
HANDLE_EXCEPTION(exception_breakpoint, "BREAKPOINT");
HANDLE_EXCEPTION(exception_overflow, "OVERFLOW");
HANDLE_EXCEPTION(exception_bound_range_exceeded, "BOUND RANGE EXCEEDED");
HANDLE_EXCEPTION(exception_invalid_opcode, "INVALID OPCODE");
HANDLE_EXCEPTION(exception_device_not_available, "DEVICE NOT AVAILABLE");
HANDLE_EXCEPTION(exception_double_fault, "DOUBLE FAULT");
HANDLE_EXCEPTION(exception_coprocessor_segment_overrun, "COPROCESSOR SEGMENT OVERRUN");
HANDLE_EXCEPTION(exception_invalid_tss, "INVALID TSS");
HANDLE_EXCEPTION(exception_segment_not_present, "SEGMENT NOT PRESENT");
HANDLE_EXCEPTION(exception_stack_segment_fault, "STACK SEGMENT FAULT");
HANDLE_EXCEPTION(exception_general_protection, "GENERAL PROTECTION");
HANDLE_EXCEPTION(exception_page_fault, "PAGE FAULT");
HANDLE_EXCEPTION(exception_x87_fpu_error, "X87 FPU FLOATING POINT ERROR");
HANDLE_EXCEPTION(exception_alignment_check, "ALIGNMENT CHECK");
HANDLE_EXCEPTION(exception_machine_check, "MACHINE CHECK");
HANDLE_EXCEPTION(exception_simd_floating_point, "SIMD FLOATING POINT EXCEPTION");

/*
 * init_idt
 * 		DESCRIPTION: initializes IDT and sets IDT entries for first 18 exceptions
 * 		INPUTS: N/A
		OUTPUT: N/A
		SIDE EFFECTS: IDT initialized and loaded via lidt
 */
void init_idt(){
	
    int i;
    for(i = 0; i < NUM_VEC; i++){	//loop through all vectors in idt table and initialize
        idt[i].seg_selector = KERNEL_CS;
        idt[i].reserved4 = 0;
        idt[i].reserved3 = 1;
        idt[i].reserved2 = 1;
        idt[i].reserved1 = 1;
        idt[i].reserved0 = 0;
		idt[i].size = 1;
        idt[i].dpl = 0;
        idt[i].present = 1;
		
		if(i > 31)
            idt[i].reserved3 = 0;    //IDT entries beyond 32 are interrupt gates
		if(i == VECTOR_SYSCALL)	//if system call is called at 0x80, set priority to "user"
			idt[i].dpl = 3;
    }

	//Set IDT entry for each exception
	//Entries referenced from INTEL IA 32 MANUAL VOL3
    SET_IDT_ENTRY(idt[0x00], exception_divide_error);
    // idt[0x01] for INTEL use only
	SET_IDT_ENTRY(idt[0x01], debug_intel_only);	//debug only
    SET_IDT_ENTRY(idt[0x02], exception_nmi_interrupt);
    SET_IDT_ENTRY(idt[0x03], exception_breakpoint);
    SET_IDT_ENTRY(idt[0x04], exception_overflow);
    SET_IDT_ENTRY(idt[0x05], exception_bound_range_exceeded);
    SET_IDT_ENTRY(idt[0x06], exception_invalid_opcode);
    SET_IDT_ENTRY(idt[0x07], exception_device_not_available);
    SET_IDT_ENTRY(idt[0x08], exception_double_fault);
    SET_IDT_ENTRY(idt[0x09], exception_coprocessor_segment_overrun);
    SET_IDT_ENTRY(idt[0x0A], exception_invalid_tss);
    SET_IDT_ENTRY(idt[0x0B], exception_segment_not_present);
    SET_IDT_ENTRY(idt[0x0C], exception_stack_segment_fault);
    SET_IDT_ENTRY(idt[0x0D], exception_general_protection);
    SET_IDT_ENTRY(idt[0x0E], exception_page_fault);
    // idt[0x0F] for INTEL use only
    SET_IDT_ENTRY(idt[0x10], exception_x87_fpu_error);
    SET_IDT_ENTRY(idt[0x11], exception_alignment_check);
    SET_IDT_ENTRY(idt[0x12], exception_machine_check);
    SET_IDT_ENTRY(idt[0x13], exception_simd_floating_point);
	
	//Set IDT entry for pit interrupts
	SET_IDT_ENTRY(idt[VECTOR_PIT], pit_handler_asm);
	
	//Set IDT entry for keyboard interrupts
	SET_IDT_ENTRY(idt[VECTOR_KEYBOARD], keyboard_handler_asm);
	
	//Set IDT entry for RTC interrupts
	SET_IDT_ENTRY(idt[VECTOR_RTC], rtc_handler_asm);

	//Set IDT entry for system calls
	SET_IDT_ENTRY(idt[VECTOR_SYSCALL], syscall_handler_asm);
	
	//set pointer to IDT
	lidt(idt_desc_ptr);	
}
