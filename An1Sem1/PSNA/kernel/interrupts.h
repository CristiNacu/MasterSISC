#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

#include "main.h"

#pragma pack(1)
typedef struct _COMMON_REGISTER_STRUCTURE
{
    QWORD Cr3;
    QWORD Cr2;
    QWORD Cr0;
    QWORD Gs;
    QWORD Fs;
    QWORD Es;
    QWORD Ds;
    QWORD R15;
    QWORD R14;
    QWORD R13;
    QWORD R12;
    QWORD R11;
    QWORD R10;
    QWORD R9;
    QWORD R8;
    QWORD Rdi;
    QWORD Rsi;
    QWORD Rdx;
    QWORD Rcx;
    QWORD Rbx;
    QWORD Rbp;
    QWORD ExceptionVector;
    QWORD Rax;
} COMMON_REGISTER_STRUCTURE;


typedef struct _EXCEPTION_REGISTER_STUCTURE
{
    COMMON_REGISTER_STRUCTURE COMMON_REGS;

    QWORD ErrCode;
    QWORD Rip;
    QWORD Cs;
    QWORD Rflags;
    QWORD Rsp;
    QWORD Ss;
} EXCEPTION_REGISTER_STUCTURE;

typedef struct _INTERRUPT_REGISTER_STUCTURE
{
    COMMON_REGISTER_STRUCTURE COMMON_REGS;

    QWORD Rip;
    QWORD Cs;
    QWORD Rflags;
    QWORD Rsp;
    QWORD Ss;
} INTERRUPT_REGISTER_STUCTURE;


#pragma pack()


void InitInterrupts();

void GeneralExceptionHandler();

#endif