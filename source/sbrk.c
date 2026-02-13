#include <_ansi.h>
#include <_syslist.h>
#include <stdlib.h>
#include <unistd.h>
#include <reent.h>
#include <errno.h>

#include <ogc/machine/asm.h>
#include <ogc/machine/processor.h>
#include <ogc/system.h>
#include <stdio.h>

#if defined(HW_RVL)
u32 MALLOC_MEM2 __attribute__((weak)) = 1;
#endif

int s_log_count = 0;
char s_log_buffer[1024];

void* _sbrk_r( struct _reent *ptr, ptrdiff_t incr)
{
	u32 level;
	char *heap_end = 0;
	char *prev_heap = 0;
#if defined(HW_RVL)
	static char *mem2_start = NULL;
#endif

    s_log_count += snprintf(s_log_buffer + s_log_count, sizeof(s_log_buffer) - s_log_count,
                            "%s: incr %d, MEM1: %p-%p, MEM2: %p-%p\n",
                            __func__, incr, SYS_GetArenaLo(), SYS_GetArenaHi(), SYS_GetArena2Lo(), SYS_GetArena2Hi());
	_CPU_ISR_Disable(level);
#if defined(HW_RVL)
	if(MALLOC_MEM2) {
		// use MEM2 aswell for malloc
		if(mem2_start==NULL)
			heap_end = (char*)SYS_GetArenaLo();
		else
			heap_end = (char*)SYS_GetArena2Lo();

		if(mem2_start) {
			// we're in MEM2
			if((heap_end+incr)>(char*)SYS_GetArena2Hi()) {
				// out of MEM2 case
				ptr->_errno = ENOMEM;
				prev_heap = (char *)-1;
			} else if ((heap_end+incr) < mem2_start) {
				// trying to sbrk() back below the MEM2 start barrier
				ptr->_errno = EINVAL;
				prev_heap = (char *)-1;
			} else {
				// success case
				prev_heap = heap_end;
				SYS_SetArena2Lo((void*)(heap_end+incr));
			}
			// if MEM2 area is exactly at the barrier, transition back to MEM1 again
			if(SYS_GetArena2Lo() == mem2_start) mem2_start = NULL;
		} else {
			// we're in MEM1
			if((heap_end+incr)>(char*)SYS_GetArenaHi()) {
				// out of MEM1, transition into MEM2
				if(((char*)SYS_GetArena2Lo() + incr) > (char*)SYS_GetArena2Hi()) {
					// this increment doesn't fit in available MEM2
					ptr->_errno = ENOMEM;
					prev_heap = (char *)-1;
				} else {
					// MEM2 is available, move into it
					mem2_start = heap_end = prev_heap = SYS_GetArena2Lo();
					SYS_SetArena2Lo((void*)(heap_end+incr));
				}
			} else {
				// MEM1 is available (or we're freeing memory)
				prev_heap = heap_end;
				SYS_SetArenaLo((void*)(heap_end+incr));
			}
		}
	} else {
#endif
		heap_end = (char*)SYS_GetArenaLo();

		if((heap_end+incr)>(char*)SYS_GetArenaHi()) {

			ptr->_errno = ENOMEM;
			prev_heap = (char *)-1;

		} else {

			prev_heap = heap_end;
			SYS_SetArenaLo((void*)(heap_end+incr));
		}
#if defined(HW_RVL)
	}
#endif
	_CPU_ISR_Restore(level);

	return (void*)prev_heap;
}
