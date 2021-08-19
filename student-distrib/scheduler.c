#include "scheduler.h"
#include "i8259.h"

uint32_t bootup_flag = 0;
int active_process;

/* scheduler()
 * Input: the terminal ID that will be "scheduled"
 * Return: none
 * Effect: Originally meant to switch execution between programs, but wasn't functional
 *			currently only spawns in the 3 different shells when they do not exist
 */
void scheduler(uint32_t t_num){
	
	//check for invalid terminal number
	if(t_num >= TERMINAL_COUNT || t_num < 0)
		return;
	
	//After the first 3 shells have been made, switch back to terminal 1
	if(bootup_flag) {
		t_num = 0;
		switch_display_terminal(t_num);
		bootup_flag = 0;
	}
	
	//reference to the current active process of given terminal
	//active_process = switch_running_task(t_num);
	active_process = find_bottom_task(t_num);
	
	//invalid task, run shell
	if(active_process == -1) {
		send_eoi(0);
		if(t_num == 2)
			bootup_flag = 1;
		switch_display_terminal(t_num);
		syscall_execute((const uint8_t*)"shell");
	}
	
	//get pcb of active task of this terminal
	/*pcb_t* active_pcb = get_pcb(active_process);
	
	//Save esp and ebp from current process
	asm volatile("movl %%esp, %0":"=r" (active_pcb->old_esp));
    asm volatile("movl %%ebp, %0":"=r" (active_pcb->old_ebp));
	
	//remap paging to new process
	syscall_paging_setup(eightM + (fourM * active_process));
	
	//save kernel stack, esp pointer
	tss.ss0 = KERNEL_DS;
	tss.esp0 = eightM - (eightK * active_process) - 4;

	
	//restore
	asm volatile ("       	  \n\
            movl %0, %%esp    \n\
            movl %1, %%ebp    \n\
            "
            :
            : "r" (active_pcb->old_esp), "r" (active_pcb->old_ebp)
            : "memory"
        );*/
		
}

/* get_active_terminal()
 * Input: none
 * Return: returns the terminal currently being processed by the scheduler
 * Effect: returns the terminal currently being processed by the scheduler
 */
int get_active_terminal() {
	return active_process;
}
