#include "paging.h"

uint32_t page_directory [oneK] __attribute__((aligned (fourK)));
uint32_t page_table [oneK] __attribute__((aligned (fourK)));
uint32_t video_page [oneK] __attribute__((aligned (fourK)));

/*
function: paging_init()
effects: initialize paging for kernel and Userspace
*/
void paging_init(){
  int i;
  for (i = 0; i < oneK; i++){
    page_directory[i] = 0x2; //RW enabled, not present

    if (i == video_mem_offset)
      page_table[i] = i * fourK | 0x3;  // RW enabled, present, supervisor
    else
      page_table[i] = i* fourK | 0x2; // RW enabled, not present
  }

  // RW enabled, present, 4kb page, page table start addr
  page_directory[0] = (uint32_t) page_table | 0x3;
  //4M start addr, RW enabled, present, 4mb page
  page_directory[1] = oneK* fourK | 0x83;

  /* initialize required regs for paging */
  //paging startup
  asm volatile(
    "movl %%cr4, %%eax \n"
    "orl $0x00000010, %%eax \n"
    "movl %%eax, %%cr4 \n"
    "movl %0, %%eax \n"
    "movl %%eax, %%cr3  \n"
    "movl %%cr0, %%eax \n"
    "orl $0x80000000, %%eax \n"
    "movl %%eax, %%cr0 \n "
    :
    :"r" (page_directory)
    :"memory", "cc", "eax"

  );

}
/*
function: syscall_paging_setup
input: physical address to map to virtual address
output: None
effect: setup paging for execute and halt
*/
void syscall_paging_setup(uint32_t addr){
  uint32_t i;
  i = virtual_mem / fourM;
  page_directory[i] = addr | 0x87 ; // user, R/w, present, 4MB page;
  flush_TLB();
}
/* videomem_map
 * Input: uint32_t virtual, uint32_t physical
 * Returns: void
 * Effect: allocate a virtual user page to point to physical
    addr of videomem
 */
void videomem_map(uint32_t virtual, uint32_t physical){
  uint32_t i = virtual / fourM;
  page_directory[i] = (uint32_t) video_page |0x7;  // user_level, R/W, present
  uint32_t j;
  for (j = 0; j < oneK; j++){
    if (j == 0)
      video_page[0] = physical | 0x7; // user_level, R/W, present
    else
      video_page[j] = 0x2; // not present
  }
  flush_TLB();

}
/* videomem_unmap
 * Input: uint32_t virtual, uint32_t physical
 * Returns: void
 * Effect: unmap the virtual user video page
 */
void videomem_unmap(uint32_t virtual, uint32_t physical){
  uint32_t i = virtual / fourM;
  video_page[0] = 0x02; // not present
  page_directory[i] = 0x02; // not present
  flush_TLB();
}
/*
function: flush_TLB
effects: refresh paging, for new settings to work
*/
void flush_TLB (void) {
  asm volatile (
    "movl %%cr3, %%eax \n"
    "movl %%eax, %%cr3 \n"
    :         /* no outputs */
    :         /* no inputs */
    : "eax"
  );
}
