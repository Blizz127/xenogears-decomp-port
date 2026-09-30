#ifndef _XENO_DEBUG_H
#define _XENO_DEBUG_H

#define POSSIBLE_DEBUG_CODE if(0){while(1);}

#ifdef WANT_BREAKPOINTS
#define ASM_BREAKPOINT "break"
#else
#define ASM_BREAKPOINT ""
#endif

#define SYSTEM_PC_HARDDRIVE 0

#endif
