#ifndef __PAGING_H_
#define __PAGING_H_

#include "main.h"


#pragma pack(push)
#pragma pack(1)
typedef struct PAGING_STRUCTURE_4_LEVELS
{
	__int64 Present				: 1;	// Must be 1, region invalid if 0.
	__int64 ReadWrite			: 1;	// If 0, writes not allowed.
	__int64 UserSupervisor		: 1;	// If 0, user-mode accesses not allowed.
	__int64 PageWriteThrough	: 1;	// Determines the memory type used to access PDPT.
	__int64 PageCacheDisable	: 1;	// Determines the memory type used to access PDPT.
	__int64 Accessed			: 1;	// If 0, this entry has not been used for translation.
	__int64 Ignored1			: 1;
	__int64 PageSize			: 1;	// Must be 0 for PML4E.
	__int64 Ignored2			: 4;
	__int64 PageFrameNumber		: 36;	// The page frame number of the PDPT of this PML4E.
	__int64 Reserved			: 4;
	__int64 Ignored3			: 11;
	__int64 ExecuteDisable		: 1;	// If 1, instruction fetches not allowed.
} PAGING_STRUCTURE_4_LEVELS;

typedef union CR3_STRUCTURE
{
	struct{
		__int64 Ignored				: 3;
		__int64 PWT					: 1;
		__int64 PCD					: 1;
		__int64 Ignored2			: 7;
		__int64 PhysicalAddress		: 36;
		__int64 Reserved			: 16;
	};
	__int64 Raw;
} CR3_STRUCTURE;

#pragma pack(pop)

OS_STATUS
MapPageRange(
	__int64 VaAddress,
	__int64 PaAddress,
	__int32 NoPages,
	PAGING_STRUCTURE_4_LEVELS* Cr3
);

OS_STATUS
MapPage(
	__int64 VaAddress,
	__int64 PaAddress,
	PAGING_STRUCTURE_4_LEVELS* Cr3
);

OS_STATUS
CreateCr3Structure(
	PAGING_STRUCTURE_4_LEVELS* FirstPageAddress,
	CR3_STRUCTURE* Cr3
);

#endif