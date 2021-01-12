#include "Paging.h"
#include "physical_memory_manager.h"
#include "logging.h"

OS_STATUS
MapPageRange(
    __int64 VaAddress, 
    __int64 PaAddress, 
    __int32 NoPages,
	PAGING_STRUCTURE_4_LEVELS *Cr3
)
{
	__int64 vaAddressShadow = VaAddress;
	__int64 paAddressShadow = PaAddress;
	OS_STATUS status;

	for (__int32 crtPage = 0; crtPage < NoPages; crtPage++)
	{
		status = MapPage(
			vaAddressShadow,
			paAddressShadow,
			Cr3
		);
		if (!OS_STATUS_IS_SUCCESS(status))
		{
			Log("MapPage returned status error!\n");
			return OS_STATUS_ERROR;
		}

		vaAddressShadow += PAGE_SIZE;
		paAddressShadow += PAGE_SIZE;
	}

    return OS_STATUS_SUCCESS;
}

OS_STATUS 
MapPage(
    __int64 VaAddress, 
    __int64 PaAddress,
	PAGING_STRUCTURE_4_LEVELS *Cr3
)
{
	PAGING_STRUCTURE_4_LEVELS *crtPagingStructure = Cr3;
	__int64 crtPagingIdxShadow = VaAddress;
	crtPagingIdxShadow <<= 16;
	
	for (__int8 level = 1; level <= 4; level++)
	{
		__int64 crtPagingIdx = (crtPagingIdxShadow >> 55) & 0b0111111111;

		PAGING_STRUCTURE_4_LEVELS *nextPagingStructure;

		__int64 nextPagingStructureFrame = crtPagingStructure[crtPagingIdx].PageFrameNumber;
		if (!nextPagingStructureFrame)
		{
			__int64 nextPageAddress;

			if (level < 4)
			{
				nextPageAddress = (__int64)AllocNewPage();
				if (nextPageAddress == 0)
				{
					Log("AllocNewPage returned error!\n");
					return OS_STATUS_ERROR;
				}
			}
			else
			{
				nextPageAddress = PaAddress;
			}

			crtPagingStructure[crtPagingIdx].PageFrameNumber = PAGE_ADDRESS_TO_PAGE_FRAME(nextPageAddress);
			crtPagingStructure[crtPagingIdx].Present = TRUE;
			crtPagingStructure[crtPagingIdx].ReadWrite = TRUE;
			crtPagingStructure[crtPagingIdx].UserSupervisor = TRUE;

			nextPagingStructure = (PAGING_STRUCTURE_4_LEVELS*)nextPageAddress;

		}
		else
		{
			nextPagingStructure = (PAGING_STRUCTURE_4_LEVELS *)(PAGE_FRAME_TO_PAGE_ADDRESS(nextPagingStructureFrame));
		}

		crtPagingStructure = nextPagingStructure;
		crtPagingIdxShadow = crtPagingIdxShadow << 9;
	}

    return OS_STATUS_SUCCESS;
}

OS_STATUS CreateCr3Structure(PAGING_STRUCTURE_4_LEVELS* FirstPageAddress, CR3_STRUCTURE* Cr3)
{

	if(!FirstPageAddress)
	{ 
		Log("Invalid address for CR3!\n");
		return OS_STATUS_ERROR;
	}
	
	Cr3->Raw = 0;
	Cr3->PhysicalAddress = PAGE_ADDRESS_TO_PAGE_FRAME((size_t)FirstPageAddress);
	return OS_STATUS_SUCCESS;
}
