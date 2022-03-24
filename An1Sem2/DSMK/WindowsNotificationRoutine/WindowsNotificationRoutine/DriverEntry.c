#include <ntddk.h>
#include <windef.h>

#define PROCESS_CREATE_EVENT    1
#define PROCESS_EXIT_EVENT      2

#define DEVICE_NAME L"Interceptor"
#define KERNEL_NAME L"\\Device\\" ## DEVICE_NAME


LARGE_INTEGER gCookie;

void OnProcessNotify(
    PEPROCESS Process,
    HANDLE ProcessId,
    PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    UNREFERENCED_PARAMETER(Process);
    if (CreateInfo) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Process created:\nPID: %X, image file path: %S, command line %S\n",
            ProcessId, (wchar_t *)CreateInfo->ImageFileName->Buffer, (wchar_t*)CreateInfo->CommandLine->Buffer);
    }
    //else {
    //    // process exit
    //}
}

void OnImageLoad(
    _In_opt_ PUNICODE_STRING FullImageName,
    _In_ HANDLE ProcessId,
    _In_ PIMAGE_INFO ImageInfo
)
{
    if (!ProcessId && ImageInfo->SystemModeImage)  //For drivers (kernel images), this value is zero.
    {
        if(FullImageName)
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Loading driver:\nimage file path: %S\n",
                FullImageName->Buffer);
    }
}

void MyUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    PsSetCreateProcessNotifyRoutineEx(
        OnProcessNotify,
        TRUE
    );

    PsRemoveLoadImageNotifyRoutine(
        OnImageLoad
    );

    CmUnRegisterCallback(gCookie);

    if (DriverObject->DeviceObject != NULL)
    {
        IoDeleteDevice(DriverObject->DeviceObject);
    }

    __debugbreak();

}


NTSTATUS
RegistryCallback(
    _In_ PVOID CallbackContext,
    _In_opt_ PVOID Argument1,
    _In_opt_ PVOID Argument2
)
{
    UNREFERENCED_PARAMETER(CallbackContext);

    if ((REG_NOTIFY_CLASS)(size_t)Argument1 == RegNtPreRenameKey && Argument2)
    {
        NTSTATUS status;
        REG_RENAME_KEY_INFORMATION* data = (REG_RENAME_KEY_INFORMATION*)Argument2;
        PCUNICODE_STRING name;

        if (data->Object)
        {
            status = CmCallbackGetKeyObjectIDEx(
                &gCookie,
                data->Object,
                NULL,
                &name,
                0
            );
            if (!NT_SUCCESS(status))
            {
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Registry key is being renamed:\nOld name: Cannot query old name, New name: %S\n",
                    data->NewName->Buffer);
            }
            else
            {
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Registry key is being renamed:\nOld name: %S, New name: %S\n",
                    name->Buffer, data->NewName->Buffer);
            }
        }
        else
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Registry key is being renamed:\nOld name: Cannot query old name, New name: %S\n",
                data->NewName->Buffer);
        }
    }
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
    UNICODE_STRING altitude = RTL_CONSTANT_STRING(L"30000");

    __debugbreak();

    RtlInitUnicodeString(&deviceName, KERNEL_NAME);

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

    do {
        ntstatus = PsSetCreateProcessNotifyRoutineEx(
            OnProcessNotify,
            FALSE
        );
        if (!NT_SUCCESS(ntstatus)) {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "PsSetCreateProcessNotifyRoutineEx returned status %d\n", ntstatus);
        }
    } while (!NT_SUCCESS(ntstatus));

    do {
        ntstatus = PsSetLoadImageNotifyRoutine(
            OnImageLoad
        );
        if (!NT_SUCCESS(ntstatus)) {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "PsSetCreateProcessNotifyRoutineEx returned status %d\n", ntstatus);
        }
    } while (!NT_SUCCESS(ntstatus));

    do {
        ntstatus = CmRegisterCallbackEx(
            RegistryCallback,
            &altitude,
            DriverObject,
            NULL,
            &gCookie,
            NULL
        );
        if (!NT_SUCCESS(ntstatus)) {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CmRegisterCallbackEx returned status %d\n", ntstatus);
        }
    } while (!NT_SUCCESS(ntstatus));

    DriverObject->DriverUnload = MyUnload;

    return STATUS_SUCCESS;
}