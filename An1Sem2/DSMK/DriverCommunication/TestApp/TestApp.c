#include "CommonDefs.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

HANDLE drvDeviceHandle = NULL;

HANDLE OpenDriverHandle()
{
    if (!drvDeviceHandle)
    {
        drvDeviceHandle = CreateFile(PRIMARY_UM_DEVICE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }
    return drvDeviceHandle;
}

int main()
{
    BYTE command[30];
    while (strcmp(command, "exit"))
    {
        scanf_s("%s", command, sizeof(command) - 1);
        if (!strcmp(command, "command1"))
        {
            DWORD bytesReturned;
            BOOL status;
            HANDLE hDevice;

            hDevice = OpenDriverHandle();
            if (hDevice == INVALID_HANDLE_VALUE)
            {
                printf("Create file 2 error %d\n", GetLastError());
                continue;
            }

            status = DeviceIoControl(
                hDevice,
                (DWORD)PRIMARY_IOCTL_COMMAND_1,
                NULL,
                0,
                NULL,
                0,
                &bytesReturned,
                NULL
            );
            if (!status)
            {
                printf("Error: DeviceIoControl Failed : %d\n", GetLastError());
            }
        }
        else if (!strcmp(command, "command2"))
        {
            DWORD bytesReturned;
            BOOL status;
            HANDLE hDevice;

            if (OpenDriverHandle() == INVALID_HANDLE_VALUE &&
                GetLastError() != ERROR_FILE_NOT_FOUND)
            {
                printf("Create file error %d\n", GetLastError());
                continue;
            }

            hDevice = OpenDriverHandle();
            if (hDevice == INVALID_HANDLE_VALUE)
            {
                printf("Create file 2 error %d\n", GetLastError());
                continue;
            }

            status = DeviceIoControl(
                hDevice,
                (DWORD)PRIMARY_IOCTL_COMMAND_2,
                NULL,
                0,
                NULL,
                0,
                &bytesReturned,
                NULL
            );
            if (!status)
            {
                printf("Error: DeviceIoControl Failed : %d\n", GetLastError());
            }
        }
        else
        {
            printf("Error: Unknown command\n");
        }
    }
    if (drvDeviceHandle)
        CloseHandle(drvDeviceHandle);
}