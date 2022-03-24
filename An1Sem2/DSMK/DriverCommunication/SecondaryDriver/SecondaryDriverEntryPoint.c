#include <ntddk.h>
#include <windef.h>
#include "SecondaryDriverDefs.h"
#include <ntddk.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD MyUnload;

void MyUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;

    if (deviceObject != NULL)
    {
        IoDeleteDevice(deviceObject);
    }

    UNREFERENCED_PARAMETER(DriverObject);
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
    ULONG command;

    stackPointer = IoGetCurrentIrpStackLocation(Irp);

    command = stackPointer->Parameters.DeviceIoControl.IoControlCode;

    //if (!inBufLength || !outBufLength)
    //{
    //    status = STATUS_INVALID_PARAMETER;
    //    goto cleanup;
    //}

    if (command == SECONDARY_IOCTL_COMMAND_1)
    {
        __debugbreak();
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "First IOCTL was called\n");
    }
    else if (command == SECONDARY_IOCTL_COMMAND_2)
    {
        __debugbreak();
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Second IOCTL was called\n");
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
MyCreate(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();
    __debugbreak();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
MyClose(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();
    __debugbreak();

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
    UNICODE_STRING deviceName;
    PDEVICE_OBJECT device;

    RtlInitUnicodeString(&deviceName, SECONDARY_KERNEL_DEVICE_NAME);

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

    DriverObject->DriverUnload = MyUnload;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = MyCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MyClose;

    return STATUS_SUCCESS;
}

