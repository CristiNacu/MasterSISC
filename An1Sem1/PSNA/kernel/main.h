#ifndef _MAIN_H_
#define _MAIN_H_

#include <intrin.h>

//
// default types
//
typedef unsigned __int8     BYTE, *PBYTE;
typedef unsigned __int16    WORD, *PWORD;
typedef unsigned __int32    DWORD, *PDWORD;
typedef unsigned __int64    QWORD, *PQWORD;
typedef signed __int8       INT8;
typedef signed __int16      INT16;
typedef signed __int32      INT32;
typedef signed __int64      INT64;

//
// exported functions from __init.asm
//
void __cli(void);
void __sti(void);
void __magic(void);         // MAGIC breakpoint into BOCHS (XCHG BX,BX)
void __assignEax(__int32);         // MAGIC breakpoint into BOCHS (XCHG BX,BX)
void __enableSSE(void);
void __setRegs(__int32 code, __int32 data);
void __setRegs64(__int32 code, __int32 data);
void __assignRipToEax();
void __assignRax(__int64 val);

#define PVOID   void *
#define BIT(bit_address) (1<<(bit_address))

#define TRUE    (1)
#define FALSE   (0)

#define KILO    (1024)
#define MEGA    (1024 * (KILO))
#define GIGA    (1024 * (MEGA))

#define PAGE_SIZE                                   (1 << 12)
#define PAGE_MASK                                   ((PAGE_SIZE) - 1)
#define PAGE_COUNT(size)                            ((size)/PAGE_SIZE)
#define PAGE_ADDRESS_TO_PAGE_FRAME(page_address)    ((page_address) >> 12)
#define PAGE_FRAME_TO_PAGE_ADDRESS(page_frame)      ((page_frame) << 12)

#define OS_STATUS __int8
#define OS_STATUS_SUCCESS   (OS_STATUS)0
#define OS_STATUS_ERROR     (OS_STATUS)1
#define OS_STATUS_IS_SUCCESS(status) ((status) == (OS_STATUS_SUCCESS))

#define MSR_EFER        0xC0000080

#define MSR_EFER_LME    BIT(8)
#define CR4_PAE         BIT(5)
#define CR0_PG          BIT(31)

#define MIN(a,b)        ((a) < (b) ? (a) : (b))

#endif // _MAIN_H_