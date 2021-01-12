#ifndef  __PHYSICAL_MEMORY_MANAGER_H__
#define  __PHYSICAL_MEMORY_MANAGER_H__

#include "main.h"

void
InitPageAllocator(QWORD LastAllocatedPage);

PVOID
AllocNewPage();

#endif // ! __PHYSICAL_MEMORY_MANAGER_H__
