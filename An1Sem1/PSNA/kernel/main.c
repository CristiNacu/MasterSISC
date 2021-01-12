#include "main.h"
#include "screen.h"
#include "logging.h"
#include "Paging.h"
#include "physical_memory_manager.h"
#include "console.h"
#include "keyboard.h"
#include "interrupts.h"

#pragma pack(1)
typedef struct _GDT_INFO_32 {
    WORD GdtLimit;
    DWORD TableStart;
}GDT_INFO_32;
#pragma pack()

#pragma pack(1)
typedef struct _GDT_INFO_64 {
    WORD GdtLimit;
    QWORD TableStart;
}GDT_INFO_64;

QWORD gGdtrDescArr[] = {
    0x00000000000000,
    // Code 16 ring 0
    0x8F9A000000FFFF,
    // Data 16 ring 0
    0x8F92000000FFFF,
    // Code 32 ring 0
    0xCF9A000000FFFF,
    // Data 32 ring 0
    0xCF92000000FFFF,
    // Code 64 ring 0
    0xAF9A000000FFFF,
    // Data 64 ring 0
    0xAF92000000FFFF
};
#pragma pack()

#define CODE_16_R0_SELCTOR 0x8
#define DATA_16_R0_SELCTOR 0x10
#define CODE_32_R0_SELCTOR 0x18
#define DATA_32_R0_SELCTOR 0x20
#define CODE_64_R0_SELCTOR 0x28
#define DATA_64_R0_SELCTOR 0x30

typedef struct _BOOT_STRUCTURE {

    DWORD LastAllocatedPage;
    DWORD Cr3;

} BOOT_STRUCTURE;


void KernelMain(BOOT_STRUCTURE *BootStructure)
{
    ClearScreen();

    InitLogging();

    InitPageAllocator(BootStructure->LastAllocatedPage);

    KeyboardDriverInit(HandleConsoleInput);

    InitInterrupts();


    __sti();
    while (1 == 1);

    // TODO!!! PIC programming; see http://www.osdever.net/tutorials/view/programming-the-pic
    // TODO!!! define interrupt routines and dump trap frame
    
    // TODO!!! Timer programming

    // TODO!!! Keyboard programming

    // TODO!!! Implement a simple console

    // TODO!!! read disk sectors using PIO mode ATA

    // TODO!!! Memory management: virtual, physical and heap memory allocators
}
