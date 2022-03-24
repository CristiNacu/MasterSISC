#include "Implementation.h"
#include <winternl.h>
#include <Psapi.h>

#define MAX_NO_OF_PROCESSES 1024

typedef __kernel_entry NTSTATUS (*NtQueryInformationProcessPtr)(
    HANDLE           ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID            ProcessInformation,
    ULONG            ProcessInformationLength,
    PULONG           ReturnLength
);

NtQueryInformationProcessPtr NtQueryAddress = NULL;

NTSTATUS _GentNtDll() {
    HINSTANCE hDLL;
    hDLL = LoadLibraryA("Ntdll.dll");
    if (hDLL == NULL)
    {
        printf_s("LoadLibrary returned error %X\n", GetLastError());
        return STATUS_ABANDONED;
    }

    NtQueryInformationProcessPtr lpfnDllFunc1 = (NtQueryInformationProcessPtr)GetProcAddress(
        hDLL, 
        "NtQueryInformationProcess"
    );
    if (lpfnDllFunc1 == NULL)
    {
        printf_s("GetProcAddress returned error %X\n", GetLastError());
        return STATUS_ABANDONED;
    }
    
    NtQueryAddress = lpfnDllFunc1;
    return STATUS_SUCCESS;
}

NTSTATUS _GetProcessCmd(DWORD ProcessId, PROCESS_INFO* processInfo)
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION pebDescriptor;
    ULONG ReturnedLength;
    BOOL statusReadProcessMemory;
    PEB peb;
    SIZE_T bytesRead;

    processInfo->ProcessCmdline = NULL;
    
    if (ProcessId == 0)
    {
        /// If the specified process is the System Idle Process(0x00000000), the function failsand the last error code is ERROR_INVALID_PARAMETER.
        return STATUS_SUCCESS;
    }

    if (NtQueryAddress == NULL)
    {
        status = _GentNtDll();
        if (!NT_SUCCESS(status))
        {
            printf("_GentNtDll returned status %X\n", status);
            return status;
        }
    }

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ,
        FALSE, ProcessId);
    if (hProcess == NULL)
    {
        status = STATUS_ABANDONED;
        printf_s("OpenProcess returned status %X\n", GetLastError());
        goto cleanup;
    }

    if (NtQueryAddress == NULL)
    {
        status = STATUS_ABANDONED;
        printf_s("[ERROR] NtQueryAddress is NULL\n");
        goto cleanup;
    }

    status = NtQueryAddress(
        hProcess,
        ProcessBasicInformation,
        &pebDescriptor,
        sizeof(pebDescriptor),
        &ReturnedLength
    );
    if (!NT_SUCCESS(status))
    {
        printf_s("NtQueryInformationProcess returned status %X\n", GetLastError());
        goto cleanup;
    }


    statusReadProcessMemory = ReadProcessMemory(
        hProcess,
        pebDescriptor.PebBaseAddress,
        &peb,
        sizeof(peb),
        &bytesRead
    );
    if (!statusReadProcessMemory)
    {
        printf_s("ReadProcessMemory returned status %X\n", GetLastError());
        status = STATUS_ABANDONED;
        goto cleanup;
    }
    if (bytesRead != sizeof(peb))
    {
        printf_s("[WARNING] Read %d bytes when sizeof(PEB) is %d\n", (int)bytesRead, (int)sizeof(peb));
    }

    RTL_USER_PROCESS_PARAMETERS processParams;
    statusReadProcessMemory = ReadProcessMemory(
        hProcess,
        peb.ProcessParameters,
        &processParams,
        sizeof(processParams),
        &bytesRead
    );
    if (!statusReadProcessMemory)
    {
        printf_s("ReadProcessMemory returned status %X\n", GetLastError());
        status = STATUS_ABANDONED;
        goto cleanup;
    }

    processInfo->CmdlineSize = processParams.CommandLine.Length;
    processInfo->ProcessCmdline = malloc(sizeof(WCHAR) * processParams.CommandLine.Length);

    statusReadProcessMemory = ReadProcessMemory(
        hProcess,
        processParams.CommandLine.Buffer,
        processInfo->ProcessCmdline,
        sizeof(WCHAR) * processParams.CommandLine.Length,
        &bytesRead
    );
    if (!statusReadProcessMemory)
    {
        printf_s("ReadProcessMemory returned status %X\n", GetLastError());
        status = STATUS_ABANDONED;
        goto cleanup;
    }

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (processInfo->ProcessCmdline != NULL)
        {
            free(processInfo->ProcessCmdline);
        }
    }

    if (hProcess && !CloseHandle(hProcess))
    {
        printf_s("Could not close handle!\n");
    }

    return status;
}

PROCESS_LIST* ListAllProcesses()
{
    PROCESS_LIST* resultedProcessList;
    DWORD processIds[MAX_NO_OF_PROCESSES];
    DWORD writtenBytes;
    BOOL status;

    resultedProcessList = (PROCESS_LIST*)malloc(sizeof(PROCESS_LIST));

    status = EnumProcesses(
        processIds,
        sizeof(processIds),
        &writtenBytes
    );
    if (!status)
    {
        printf_s("EnumProcesses returned error %X", GetLastError());
    }

    DWORD noProcessesIds = (writtenBytes / sizeof(DWORD));

    resultedProcessList->listSize = 0;
    resultedProcessList->processes = malloc(sizeof(PROCESS_INFO) * noProcessesIds);

    for (DWORD i = 0; i < noProcessesIds; i++)
    {
        resultedProcessList->processes[i].ProcessId = processIds[i];
        NTSTATUS status = _GetProcessCmd(
            processIds[i], 
            &resultedProcessList->processes[i]
        );
        resultedProcessList->processes[i].ErrorStatus = status;
        resultedProcessList->listSize++;
    }

    return resultedProcessList;
}

void PrintProcessList(PROCESS_LIST *ProcessList)
{
    for (DWORD i = 0; i < ProcessList->listSize; i++)
    {
        printf_s("Process %d\nPID: %d\n", i, ProcessList->processes[i].ProcessId);
        if (!NT_SUCCESS(ProcessList->processes[i].ErrorStatus))
        {
            //if (ProcessList->processes[i].ErrorStatus == ERROR_ACCESS_DENIED)
            //{
            //    printf_s("Process could not be oppened, as per msdn docs:\n\
            //        If the process is the System process or one of the Client Server Run-Time Subsystem (CSRSS) processes\n\
            //        this function fails and the last error code is ERROR_ACCESS_DENIED because their access restrictions prevent user-level code from opening them.\n");
            //}
        }
        else if (ProcessList->processes[i].ProcessCmdline != NULL)
        {
            printf_s("Cmdline: %S\n", ProcessList->processes[i].ProcessCmdline);
        }
        printf_s("\n");
    }
}

void FreeProcessList(PROCESS_LIST **ProcessList)
{
    PROCESS_LIST* procList = *ProcessList;
    for (DWORD i = 0; i < procList->listSize; i++)
    {
        if (NT_SUCCESS(procList->processes[i].ErrorStatus) &&
            (procList->processes[i].ProcessCmdline != NULL))
        {
            free(procList->processes[i].ProcessCmdline);
        }
    }
    free(procList->processes);
    free(procList);
    *ProcessList = NULL;
}

