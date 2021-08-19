#include "types.h"

#define oneK 1024
#define fourK 4096
#define fourM 0x400000
#define eightM 0x800000
#define video_mem_offset 0xb8
#define video_mem_addr 0xb8000 
#define virtual_mem 0x8000000
#define _148MB 0x9400000



extern uint32_t page_directory [oneK] __attribute__((aligned (fourK)));
extern uint32_t page_table [oneK] __attribute__((aligned (fourK)));
extern uint32_t video_page [oneK] __attribute__((aligned (fourK)));


/*functions*/
void paging_init();
void syscall_paging_setup(uint32_t addr);
void flush_TLB (void) ;
void videomem_map(uint32_t virtual, uint32_t physical);
void videomem_unmap(uint32_t virtual, uint32_t physical);
