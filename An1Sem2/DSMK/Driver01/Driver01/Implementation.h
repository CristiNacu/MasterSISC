#ifndef _IMPLEMENTATION_H_
#define _IMPLEMENTATION_H_

#include "Includes.h"

typedef struct _PROCESS_INFO {
    DWORD ProcessId;
    NTSTATUS ErrorStatus;
    USHORT CmdlineSize;
    PWSTR ProcessCmdline;
} PROCESS_INFO;


typedef struct _PROCESS_LIST{

    DWORD listSize;
    PROCESS_INFO* processes;

} PROCESS_LIST;



PROCESS_LIST* ListAllProcesses();
void PrintProcessList(PROCESS_LIST* ProcessList);
void FreeProcessList(PROCESS_LIST** ProcessList);

#endif