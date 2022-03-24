#include "CommonDefs.h"
#include <ntstatus.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <Psapi.h>

HANDLE drvDeviceHandle = NULL;

HANDLE 
OpenDriverHandle()
{
    if (!drvDeviceHandle)
    {
        drvDeviceHandle = CreateFile(UM_DEVICE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }
    return drvDeviceHandle;
}

NTSTATUS
AddPidToProtection(
    DWORD Pid
)
{
    DWORD bytesReturned;
    DWORD pid;
    BOOL status;
    HANDLE hDevice;

    if (!Pid)
        return STATUS_INVALID_PARAMETER_1;
    
    pid = Pid;

    hDevice = OpenDriverHandle();
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("Create file 2 error %d\n", GetLastError());
        return STATUS_INVALID_HANDLE;
    }

    status = DeviceIoControl(
        hDevice,
        (DWORD)PROTECT_PID,
        &pid,
        sizeof(pid),
        NULL,
        0,
        &bytesReturned,
        NULL
    );
    if (!status)
    {
        printf("Error: DeviceIoControl Failed : %d\n", GetLastError());
    }
    return STATUS_SUCCESS;
}

NTSTATUS
RemovePidFromProtection(
    DWORD Pid
)
{
    DWORD bytesReturned;
    DWORD pid;
    BOOL status;
    HANDLE hDevice;

    if (!Pid)
        return STATUS_INVALID_PARAMETER_1;

    pid = Pid;

    hDevice = OpenDriverHandle();
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("Create file 2 error %d\n", GetLastError());
        return STATUS_INVALID_HANDLE;
    }
    else
    {
        printf("Driver opened\n");
    }

    status = DeviceIoControl(
        hDevice,
        (DWORD)UNPROTECT_PID,
        &pid,
        sizeof(pid),
        NULL,
        0,
        &bytesReturned,
        NULL
    );
    if (!status)
    {
        printf("Error: DeviceIoControl Failed : %d\n", GetLastError());
    }

    return STATUS_SUCCESS;
}

NTSTATUS 
StartProcess(
    const char * Cmdline,
    PROCESS_INFORMATION *ProcessInformation
)
{
    STARTUPINFOA startupInfo = { .cb = sizeof(startupInfo) };
    PROCESS_INFORMATION processInformation = { 0 };

    BOOL status;

    status = CreateProcessA(
        NULL,
        Cmdline,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &startupInfo,
        &processInformation
    );
    if (!status)
    {
        printf("Could not start notepad.exe! CreateProcessA exitted with status %x\n", GetLastError());
        return STATUS_ABANDONED;
    }
    
    memcpy_s(ProcessInformation, sizeof(PROCESS_INFORMATION), &processInformation, sizeof(processInformation));

    return STATUS_SUCCESS;
}

#define MAX_PIDS 2048

BOOL
IsPidAvailable(
    DWORD Pid
)
{
    DWORD pidList[MAX_PIDS];
    DWORD pidsRead;
    EnumProcesses(pidList, sizeof(pidList), &pidsRead);
    for (DWORD i = 0; i < pidsRead; i++)
        if (pidList[i] == Pid)
            return TRUE;
    return FALSE;
}

void SanityTest()
{
    DWORD notepadPid;
    char pidString[20];
    char cmdline[100] = "taskkill /pid ";
    PROCESS_INFORMATION notepadProcessInfo, taskkillProcessInfo;

    StartProcess(
        "c:\\Windows\\System32\\notepad.exe",
        &notepadProcessInfo
    );

    notepadPid = notepadProcessInfo.dwProcessId;

    AddPidToProtection(notepadPid);

    printf("Notepad pid %d\n", notepadPid);

    _itoa_s(notepadPid, pidString, 20, 10);

    strcat_s(cmdline, sizeof(cmdline), pidString);
    printf("cmdline %s\n", cmdline);

    StartProcess(
        cmdline,
        &taskkillProcessInfo
    );

    WaitForSingleObject(
        taskkillProcessInfo.hProcess,
        INFINITE
    );

    if (IsPidAvailable(notepadPid))
        printf("Notepad is still open\n");
    else
        printf("Notepad was succesfully closed\n");

    CloseHandle(taskkillProcessInfo.hProcess);
    CloseHandle(taskkillProcessInfo.hThread);

    RemovePidFromProtection(notepadPid);

    StartProcess(
        cmdline,
        &taskkillProcessInfo
    );

    WaitForSingleObject(
        taskkillProcessInfo.hProcess,
        INFINITE
    );

    if (IsPidAvailable(notepadPid))
        printf("Notepad is still open\n");
    else
        printf("Notepad was succesfully closed\n");

    CloseHandle(taskkillProcessInfo.hProcess);
    CloseHandle(taskkillProcessInfo.hThread);

    CloseHandle(notepadProcessInfo.hProcess);
    CloseHandle(notepadProcessInfo.hThread);

}

void PrintHelp()
{
    printf("protect <PID> - protects PID from being terminated\nunprotect <PID> - removes PID from protecton\ntest - performs sanity test\n");
}

int main(int argc, char* argv[])
{
    if (argc < 2)
        PrintHelp();

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "test"))
        {
            SanityTest();
        }
        else if (strcmp(argv[i], "protect"))
        {
            NTSTATUS status;

            i++;
            int pid = atoi(argv[i]);

            status = AddPidToProtection(
                pid
            );
            if (status != STATUS_SUCCESS)
            {
                printf("AddPidToProtection returned status %x\n", status);
            }
        }
        else if (strcmp(argv[i], "unprotect"))
        {
            NTSTATUS status;

            i++;
            int pid = atoi(argv[i]);

            status = RemovePidFromProtection(
                pid
            );
            if (status != STATUS_SUCCESS)
            {
                printf("RemovePidFromProtection returned status %x\n", status);
            }
        }
        else if (strcmp(argv[i], "help"))
        {
            PrintHelp();
        }
        else
        {
            printf("Unknown command!\n");
            PrintHelp();
        }
    }
}