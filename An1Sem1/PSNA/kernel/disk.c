#include "disk.h"
#include "physical_memory_manager.h"

#define SECTOR_DISK_DATA        0x1F0
#define SECTOR_COUNT_PORT       0x1F2
#define SECTOR_NUMBER_PORT      0x1F3
#define SECTOR_CYL_LOW_PORT     0x1F4
#define SECTOR_CYL_HIGH_PORT    0x1F5
#define HEAD_PORT               0x1F6
#define DISK_COMMAND_PORT       0x1F7

#define READ_WITH_RETRY_COMMAND 0x20

// Luat de aici https://wiki.osdev.org/ATA_read/write_sectors
// si de aici https://wiki.osdev.org/ATA_PIO_Mode#Hardware
QWORD ReadSector(BYTE Cylinder, BYTE Head, BYTE Sector)
{
    QWORD address = AllocNewPage();

    BYTE headPortValue = Head & 0xF | 0xA0;
    __out(HEAD_PORT, headPortValue);

    __out(SECTOR_COUNT_PORT, 1);
    __out(SECTOR_NUMBER_PORT, Sector);
    __out(SECTOR_CYL_LOW_PORT, Cylinder);
    __out(SECTOR_CYL_HIGH_PORT, 0);
    __out(DISK_COMMAND_PORT, READ_WITH_RETRY_COMMAND);

    while (__in(DISK_COMMAND_PORT) == 0);

    WORD* buffer = address;

    for (int i = 0; i < 256; i++)
    {
        WORD sectorWord = __inword(SECTOR_DISK_DATA);
        buffer[i] = sectorWord;
    }

    return address;
    
}
