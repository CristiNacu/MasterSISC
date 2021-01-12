#include "physical_memory_manager.h"
#include "logging.h"

#define FREE_MEMORY_LIMIT (0x80000)

QWORD gCrtPageAllocationAddress = 0x50000;

void InitPageAllocator(QWORD LastAllocatedPage)
{
    Log("Set last allocated page");
    gCrtPageAllocationAddress = LastAllocatedPage;
}

PVOID
AllocNewPage()
{
    if (gCrtPageAllocationAddress >= FREE_MEMORY_LIMIT)
    {
        Log("Reached the limit of conventional low memory, too many allocations!!!\n");
        return 0;
    }

    __int8 *retAddr = gCrtPageAllocationAddress;
    gCrtPageAllocationAddress += PAGE_SIZE;

    for (__int32 crtByte = 0; crtByte < PAGE_SIZE; crtByte++)
        retAddr[crtByte] = 0;

    return (PVOID)retAddr;
}
