#include <ntddk.h>
#include <windef.h>
#include "CommonDefs.h"
#include <ntddk.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD MyUnload;

void MyUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING uniWin32NameString;
    __debugbreak();

    RtlInitUnicodeString(&uniWin32NameString, DOS_DEVICE_NAME);


    IoDeleteSymbolicLink(&uniWin32NameString);

    if (deviceObject != NULL)
    {
        IoDeleteDevice(deviceObject);
    }

    UNREFERENCED_PARAMETER(DriverObject);
}

NTSTATUS
MyCreate(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    __debugbreak();

    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
DeviceControl(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    __debugbreak();

    //NTSTATUS status;
    IO_STACK_LOCATION* stackPointer;
    //ULONG inBufLength;
    //ULONG outBufLength;
    ULONG command;

    stackPointer = IoGetCurrentIrpStackLocation(Irp);
    //inBufLength = stackPointer->Parameters.DeviceIoControl.InputBufferLength;
    //outBufLength = stackPointer->Parameters.DeviceIoControl.OutputBufferLength;
    command = stackPointer->Parameters.DeviceIoControl.IoControlCode;

    //if (!inBufLength || !outBufLength)
    //{
    //    status = STATUS_INVALID_PARAMETER;
    //    goto cleanup;
    //}

    if (command == IOCTL_COMMAND_1)
    {
        __debugbreak();
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "First IOCTL was called\n");
    }
    else if (command == IOCTL_COMMAND_2)
    {
        __debugbreak();
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Second IOCTL was called\n");
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

//cleanup:
//    return status;
}


NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS ntstatus;
    UNICODE_STRING deviceName;
    UNICODE_STRING dosDeviceName;
    PDEVICE_OBJECT device;

    RtlInitUnicodeString(&deviceName, KERNEL_DEVICE_NAME);
    RtlInitUnicodeString(&dosDeviceName, DOS_DEVICE_NAME);

    __debugbreak();
    ntstatus = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &device);
    if (!NT_SUCCESS(ntstatus))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IoCreateDevice returned status %d\n", ntstatus);
        return ntstatus;
    }
    __debugbreak();

    ntstatus = IoCreateSymbolicLink(&dosDeviceName, &deviceName);
    if (!NT_SUCCESS(ntstatus))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IoCreateSymbolicLink returned status %d\n", ntstatus);
        return ntstatus;
    }

    DriverObject->DriverUnload = MyUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = MyCreate;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;

    return STATUS_SUCCESS;
}

