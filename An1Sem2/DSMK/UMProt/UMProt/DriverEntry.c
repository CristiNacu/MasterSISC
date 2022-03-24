#include <ntddk.h>
#include <windef.h>
#include "CommonDefs.h"
#include <stdlib.h>

#define MAX_NO_PIDS 1024
KMUTEX gListMutex;
DWORD gProtectedPids[MAX_NO_PIDS];
DWORD gLastIndex = 0;

VOID
ProcessProtectionRoutine(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(ProcessId);

    if (CreateInfo && 
        CreateInfo->CommandLine && 
        CreateInfo->CommandLine->Buffer && 
        wcsstr(CreateInfo->CommandLine->Buffer, L"taskkill"))
    {
        PWCH pidAddress = wcsstr(CreateInfo->CommandLine->Buffer, L"/pid");
        DWORD pid;
        BOOLEAN found = FALSE;
        __debugbreak();

        pidAddress += sizeof("/pid ") - 1;
        pid = _wtoi(pidAddress);

        KeWaitForSingleObject(&gListMutex, Executive, KernelMode, FALSE, NULL);


        for (DWORD i = 0; i < gLastIndex && !found; i++)
            if (gProtectedPids[i] == pid)
                found = TRUE;

        if (found)
        {
            CreateInfo->CreationStatus = STATUS_ACCESS_DENIED;
        }

        KeReleaseMutex(&gListMutex, FALSE);
    }
    

}

NTSTATUS
MyCreateClose(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

void
MyUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    PsSetCreateProcessNotifyRoutineEx(
        ProcessProtectionRoutine,
        TRUE
    );

    if (DriverObject->DeviceObject != NULL)
        IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS
DeviceControl(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    //NTSTATUS status;
    IO_STACK_LOCATION* stackPointer;
    ULONG inBufLength;
    ULONG command;
    DWORD processId;

    stackPointer = IoGetCurrentIrpStackLocation(Irp);
    inBufLength = stackPointer->Parameters.DeviceIoControl.InputBufferLength;
    command = stackPointer->Parameters.DeviceIoControl.IoControlCode;

    if (!inBufLength || inBufLength != sizeof(DWORD))
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Protect process
    if (command == PROTECT_PID)
    {
        BOOLEAN found = FALSE;
        __debugbreak();

        if (gLastIndex >= MAX_NO_PIDS)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        processId = *(DWORD*)Irp->AssociatedIrp.SystemBuffer;

        KeWaitForSingleObject(&gListMutex, Executive, KernelMode, FALSE, NULL);
        for (DWORD i = 0; i < gLastIndex && !found; i++)
            if (gProtectedPids[i] == processId)
                found = TRUE;

        if (!found)
        {
            gProtectedPids[gLastIndex] = processId;
            gLastIndex++;
        }
        KeReleaseMutex(&gListMutex, FALSE);

    }
    // Unprotect process
    else if (command == UNPROTECT_PID)
    {
        BOOLEAN found = FALSE;
        __debugbreak();

        if (gLastIndex >= MAX_NO_PIDS)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        processId = *(DWORD*)Irp->AssociatedIrp.SystemBuffer;

        KeWaitForSingleObject(&gListMutex, Executive, KernelMode, FALSE, NULL);

        for (DWORD i = 0; i < gLastIndex; i++)
        {
            if (gProtectedPids[i] == processId)
                found = TRUE;
            if (found)
                gProtectedPids[i] = gProtectedPids[i + 1];
        }
        if (found)
            gLastIndex--;

        KeReleaseMutex(&gListMutex, FALSE);
        
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS ntstatus;
    UNICODE_STRING dosDeviceName;
    UNICODE_STRING deviceName;
    PDEVICE_OBJECT device;

    __debugbreak();

    KeInitializeMutex(&gListMutex, 0);
    RtlInitUnicodeString(&deviceName, KERNEL_DEVICE_NAME);
    RtlInitUnicodeString(&dosDeviceName, DOS_DEVICE_NAME);

    ntstatus = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &device
    );

    if (!NT_SUCCESS(ntstatus))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IoCreateDevice returned status %d\n", ntstatus);
        return ntstatus;
    }

    ntstatus = IoCreateSymbolicLink(&dosDeviceName, &deviceName);
    if (!NT_SUCCESS(ntstatus))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IoCreateSymbolicLink returned status %d\n", ntstatus);
        return ntstatus;
    }

    ntstatus = PsSetCreateProcessNotifyRoutineEx(
        ProcessProtectionRoutine,
        FALSE
    );

    DriverObject->DriverUnload = MyUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = MyCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MyCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
    return STATUS_SUCCESS;
}