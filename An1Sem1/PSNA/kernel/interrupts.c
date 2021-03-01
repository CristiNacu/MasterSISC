#include "interrupts.h"
#include "logging.h"
#include "physical_memory_manager.h"
#include "timer.h"
#include "keyboard.h"

QWORD gInterruptDescriptorTableAddress = 0;

#define LEAST_SIGNIFICANT_WORD(value) (value & 0x0ffff)
#define MOST_SIGNIFICANT_WORD(value) ((value >> 16) & 0x0ffff)   
#define LEAST_SIGNIFICANT_DWORD(value) (value & 0x0ffffffff)   
#define MOST_SIGNIFICANT_DWORD(value) ((value >> 32) & 0x0ffffffff)  

extern void _Handle0();
extern void _Handle1();

#define HANDLE_SIZE (((QWORD)&_Handle1) - ((QWORD)&_Handle0))
QWORD handle0Address = (QWORD)&_Handle0;

char* ExceptionNames[] = {
    "DIVIDE ERROR",
    "DEBUG EXCEPTION",
    "NON MASKABLE INTERRUPT",
    "BREAKPOINT",
    "OVERFOW",
    "BOUND RANGE EXCEPTION",
    "INVALID OPCODE #UD",
    "NO MATH COPROCESSOR",
    "DOUBLE FAULT",
    "COPROCESSOR SEGMENT OVERRUN",
    "INVALID TSS",
    "SEGMENT NOT PRESENT",
    "STACK SEGMENT FAULT",
    "GENERAL PROTECTION",
    "PAGE FAULT",
    "",
    "MATH FAULT",
    "ALIGNMENT CHECK",
    "MACHINE CHECK",
    "SIMD FPU EXCEPTION",
    "VIRTUALIZATION EXCEPTION"
};

typedef enum _EXCEPTION_VECTOR
{
    VECTOR_NR_DE = 0,
    VECTOR_NR_DB = 1,
    VECTOR_NR_NMI = 2,
    VECTOR_NR_BP = 3,
    VECTOR_NR_OF = 4,
    VECTOR_NR_UD = 6,
    VECTOR_NR_NM = 7,
    VECTOR_NR_DF = 8,
    VECTOR_NR_TSS = 10,
    VECTOR_NR_NP = 11,
    VECTOR_NR_SS = 12,
    VECTOR_NR_GP = 13,
    VECTOR_NR_PF = 14,
    VECTOR_NR_MF = 16,
    VECTOR_NR_AC = 17,
    VECTOR_NR_MC = 18,
    VECTOR_NR_XM = 19,
    VECTOR_NR_VE = 20,
    RESERVED_START = 21,
    RESERVED_END = 31,
    USER_DEFINED_START = 32,
    USER_DEFINED_END = 255
} EXCEPTION_VECTOR;


#pragma pack(1)
typedef struct _IDT_ATTRIBUTES {
    union
    {
        struct
        {
            BYTE Type : 4;
            BYTE Zero : 1;
            BYTE Dpl : 2;
            BYTE P : 1;
        } Explicit;

        BYTE Raw;
    };
} IDT_ATTRIBUTES;
#pragma pack()

#pragma pack(1)
typedef struct _IDT_DESCRIPTOR
{
    WORD Offset1;
    WORD Selector;
    BYTE Ist;
    IDT_ATTRIBUTES Attributes;
    WORD Offset2;
    DWORD Offset3;
    DWORD Zero;
} IDT_DESCRIPTOR;
#pragma pack()

#pragma pack(1)
typedef struct _IDT
{
    IDT_DESCRIPTOR Table[256];
} IDT;
#pragma pack()

#pragma pack(1)
typedef struct _IDT_ADDRESS_STRUCTURE
{
    WORD Limit;
    QWORD IdtAddress;
} IDT_ADDRESS_STRUCTURE;
#pragma pack()

#define DEBUG_INTERRUPTS FALSE
//  PCI1 and PCI2 I/O Ports Base
#define PIC1            0x20
#define PIC2            0xA0

//  PCI1 and PCI2 Control Registers
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

//  Eoi value
#define PIC_EOI         0x20

//  Configuration values
#define ICW3_MASK 0x7
#define ICW1_INIT	                0x10	    // Initiate configuration
#define ICW1_ICW4	                0x01        // Determines the configuration will be MASTER SLAVE next ICW MASTER SLAVE next ICW
#define ICW3_MASTER_IRQ2            0x4         // Communicate with slave PIC via IRQ2
#define ICW3_SLAVE_CASCADE          0x2         // The master will cascade-communicate via IRQ2
#define ICW4_INTEL_ARCHITECTURE	    0x01		// Bit 0 must be set in order to indicate the controoler are operating in INTEL AChitecture

BYTE    gRemappedBaseForPic1;
BYTE    gRemappedBaseForPic2;


void 
RemapPicInterrupts(
    DWORD OffsetInt_0_7,
    DWORD OffsetInt_8_15
)
{
    BYTE pic1InterruptMask;
    BYTE pic2InterruptMask;

    if (OffsetInt_0_7 + 7 >= OffsetInt_8_15)
    {
        Log("[WARNING] Overlapping IRQs\n");
    }

    gRemappedBaseForPic1 = OffsetInt_0_7;
    gRemappedBaseForPic2 = OffsetInt_8_15;


    pic1InterruptMask = __inbyte(PIC1_DATA);
    pic2InterruptMask = __inbyte(PIC2_DATA);

    // Send ICW1 in COMMAND register
    __outbyte(PIC1_COMMAND, (ICW1_INIT | ICW1_ICW4));
    __outbyte(PIC2_COMMAND, (ICW1_INIT | ICW1_ICW4));

    // Send ICW2 in DATA register
    __outbyte(PIC1_DATA, OffsetInt_0_7);
    __outbyte(PIC2_DATA, OffsetInt_8_15);

    // Send ICW3 in DATA register
    __outbyte(PIC1_DATA, ICW3_MASTER_IRQ2);
    __outbyte(PIC2_DATA, ICW3_SLAVE_CASCADE);

    // Send ICW4 in DATA register
    __outbyte(PIC1_DATA, ICW4_INTEL_ARCHITECTURE);
    __outbyte(PIC2_DATA, ICW4_INTEL_ARCHITECTURE);

    // Reload masks
    __outbyte(PIC1_DATA, pic1InterruptMask);
    __outbyte(PIC2_DATA, pic2InterruptMask);
    return;
}

void
SendEoiForPic(
    BYTE Irq
)
{
    __int32 validIrq = FALSE;
    if (Irq < 8)
    {
        __outbyte(PIC1_COMMAND, PIC_EOI);
        validIrq = TRUE;
    }
    if (Irq >= 8)
    {
        __outbyte(PIC1_COMMAND, PIC_EOI);
        __outbyte(PIC2_COMMAND, PIC_EOI);
        validIrq = TRUE;
    }

    return;
}

void InitInterrupts()
{
    gInterruptDescriptorTableAddress = AllocNewPage();

    sizeof(IDT);
    IDT* idt = (IDT*)gInterruptDescriptorTableAddress;

    for (DWORD vector = 0; vector <= 256; vector++)
    {
        QWORD isrStartAddr = 0;
        idt->Table[vector].Selector = 0x28; // Trust me 
        idt->Table[vector].Ist = 0; // Don't use IST mechanism right now (https://wiki.osdev.org/Task_State_Segment)
        idt->Table[vector].Zero = 0;
        idt->Table[vector].Attributes.Explicit.Type = 14;
        idt->Table[vector].Attributes.Explicit.Zero = 0;
        idt->Table[vector].Attributes.Explicit.P = 1; // Present
        idt->Table[vector].Attributes.Explicit.Dpl = 0;/*vector == 33 ? 3 : 0;*/ // Ring 0 ISR

        isrStartAddr = (handle0Address + (vector * HANDLE_SIZE)); // Compute the offset of the ISR handle
        idt->Table[vector].Offset1 = LEAST_SIGNIFICANT_WORD(isrStartAddr);
        idt->Table[vector].Offset2 = MOST_SIGNIFICANT_WORD(isrStartAddr);
        idt->Table[vector].Offset3 = MOST_SIGNIFICANT_DWORD(isrStartAddr);
    }

    IDT_ADDRESS_STRUCTURE idtStructure;

    idtStructure.Limit = sizeof(IDT);
    idtStructure.IdtAddress = (QWORD)idt;

    __lidt(&idtStructure);
    Log("Init interrupts ok\n");

    RemapPicInterrupts(0x28, 0x30);
    return;
}

void GeneralExceptionHandler(COMMON_REGISTER_STRUCTURE* Registers)
{
    //__assignRax(Registers->ExceptionVector);
    //__magic();

    if (Registers->ExceptionVector >= gRemappedBaseForPic1)
    {

        if (Registers->ExceptionVector - gRemappedBaseForPic1 == 0)
        {
            TimerDriver();
        }
        if (Registers->ExceptionVector - gRemappedBaseForPic1 == 1)
        {
            KeyboardDriver();
        }

        SendEoiForPic(Registers->ExceptionVector - gRemappedBaseForPic1);
        return;
    }
    else
    {
        Log("Caught Error!");


        if (Registers->ExceptionVector == VECTOR_NR_PF)
        {
            Log("Page Fault");
        }

        if (Registers->ExceptionVector == VECTOR_NR_DE)
        {
            Log("Division by zero");
        }

        if (Registers->ExceptionVector == VECTOR_NR_DF)
        {
            Log("Double fault");
        }

        if (Registers->ExceptionVector == VECTOR_NR_TSS)
        {
            Log("Tss exception");
        }

        __assignRax(Registers->ExceptionVector);

        __magic();
        __cli();
        __halt();
    }
    return;

}

