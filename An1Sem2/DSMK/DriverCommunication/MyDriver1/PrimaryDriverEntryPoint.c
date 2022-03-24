#include <ntddk.h>
#include <windef.h>
#include "CommonDefs.h"
#include <ntddk.h>
#include <wdm.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD MyUnload;
IO_COMPLETION_ROUTINE CompletionRoutine;
PFILE_OBJECT gSecondaryDriverFileObject = NULL;
PDEVICE_OBJECT gSecondaryDriverDeviceObject = NULL;

void MyUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING uniWin32NameString;

    __debugbreak();

    RtlInitUnicodeString(&uniWin32NameString, PRIMARY_DOS_DEVICE_NAME);
    IoDeleteSymbolicLink(&uniWin32NameString);

    if (deviceObject != NULL)
    {
        IoDeleteDevice(deviceObject);
    }

    if (!gSecondaryDriverDeviceObject)
    {
        goto cleanup;
    }

    ObReferenceObjectByPointer(
        gSecondaryDriverDeviceObject,
        0,
        NULL,
        KernelMode
    );

    if (gSecondaryDriverFileObject)
    {
        ObDereferenceObject(gSecondaryDriverFileObject);
        gSecondaryDriverFileObject = NULL;
    }
    if (gSecondaryDriverDeviceObject)
    {
        ObDereferenceObject(gSecondaryDriverDeviceObject);
        gSecondaryDriverDeviceObject = NULL;
    }

cleanup:

}
NTSTATUS
CompletionRoutine(
    PDEVICE_OBJECT Device,
    PIRP Irp,
    PVOID Context
)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Context);
    __debugbreak();

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

    if (command == PRIMARY_IOCTL_COMMAND_1)
    {
        __debugbreak();
        if (!gSecondaryDriverDeviceObject)
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Secondary driver was not opened\n");
            return STATUS_ABANDONED;
        }

        IoCopyCurrentIrpStackLocationToNext(
            Irp
        );

        IoSetCompletionRoutine(
            Irp,
            &CompletionRoutine,
            NULL,
            TRUE,
            TRUE,
            TRUE
        );

    }
    else if (command == PRIMARY_IOCTL_COMMAND_2)
    {
        __debugbreak();
        if (!gSecondaryDriverDeviceObject)
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Secondary driver was not opened\n");
            return STATUS_ABANDONED;
        }
    }

    return IoCallDriver(gSecondaryDriverDeviceObject, Irp);


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
    UNICODE_STRING secondaryDriverName;
    
    PDEVICE_OBJECT device;

    RtlInitUnicodeString(&deviceName, PRIMARY_KERNEL_DEVICE_NAME);
    RtlInitUnicodeString(&dosDeviceName, PRIMARY_DOS_DEVICE_NAME);
    RtlInitUnicodeString(&secondaryDriverName, SECONDARY_KERNEL_DEVICE_NAME);

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

    ntstatus = IoCreateSymbolicLink(&dosDeviceName, &deviceName);
    if (!NT_SUCCESS(ntstatus))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IoCreateSymbolicLink returned status %d\n", ntstatus);
        return ntstatus;
    }

    ntstatus = IoGetDeviceObjectPointer(
        &secondaryDriverName,
        FILE_READ_ATTRIBUTES,
        &gSecondaryDriverFileObject,
        &gSecondaryDriverDeviceObject
    );
    if (!NT_SUCCESS(ntstatus))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IoGetDeviceObjectPointer returned status %d\n", ntstatus);
        gSecondaryDriverDeviceObject = NULL;
        return ntstatus;
    }

    device->StackSize = 2;



    DriverObject->DriverUnload = MyUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = MyCreate;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;

    return STATUS_SUCCESS;
}