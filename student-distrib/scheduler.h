#include "system_call.h"
#include "pit.h"
#include "paging.h"
#include "fs_module.h"
#include "x86_desc.h"
#include "rtc.h"
#include "keyboard.h"

extern void scheduler(uint32_t t_num);
extern int get_active_terminal();
