#include "RegistryFilter.h"
#include "CommShared.h"
#include <windef.h>
#include "Trace.h"

LARGE_INTEGER gCookie = { 0 };


typedef NTSTATUS (*GenericRegistryFunction)(PVOID EventStructure);

NTSTATUS RegistryCreate(PVOID EventStructure);
NTSTATUS RegistrySetValue(PVOID EventStructure);
NTSTATUS RegistryDeleteKey(PVOID EventStructure);
NTSTATUS RegistryDeleteValue(PVOID EventStructure);
NTSTATUS RegistryLoadKey(PVOID EventStructure);
NTSTATUS RegistryRenameKey(PVOID EventStructure);


const GenericRegistryFunction RegistryCallbackFunctionArray[] = {
    RegistryCreate,
    RegistrySetValue,
    RegistryDeleteKey,
    RegistryDeleteValue,
    RegistryLoadKey,
    RegistryRenameKey
};


NTSTATUS RegistryCallback(
    _In_ PVOID CallbackContext,
    _In_opt_ PVOID Argument1,
    _In_opt_ PVOID Argument2
)
{
    UNREFERENCED_PARAMETER(CallbackContext);

    NTSTATUS status;
    REG_NOTIFY_CLASS regNotifyClass = ((REG_NOTIFY_CLASS)(ULONG_PTR)Argument1);

    // Create Key 
    // Only pre events are intercepted.
    if (regNotifyClass == RegNtPreCreateKey)
        status = RegistryCallbackFunctionArray[0](Argument2);

    // Set Value
    // Only pre events are intercepted.
    else if (regNotifyClass == RegNtSetValueKey)
        status = RegistryCallbackFunctionArray[1](Argument2);

    // Delete Key
    // Only pre events are intercepted.
    else if (regNotifyClass == RegNtDeleteKey)
        status = RegistryCallbackFunctionArray[2](Argument2);

    // Delete Value
    // Only pre events are intercepted.
    else if (regNotifyClass == RegNtDeleteValueKey)
        status = RegistryCallbackFunctionArray[3](Argument2);

    // Load Key
    // Only pre events are intercepted.
    else if (regNotifyClass == RegNtPreLoadKey)
        status = RegistryCallbackFunctionArray[4](Argument2);

    // Rename key
    else if (regNotifyClass == RegNtRenameKey)
        status = RegistryCallbackFunctionArray[5](Argument2);

    else
        status = STATUS_SUCCESS;

    return status;
}


NTSTATUS
RegFltInitialize(
    PDRIVER_OBJECT DriverObject
)
{
    if (gCookie.QuadPart)
    {
        return STATUS_ALREADY_INITIALIZED;
    }

    UNICODE_STRING altitude = RTL_CONSTANT_STRING(L"12345.67");

    return CmRegisterCallbackEx(
        RegistryCallback,
        &altitude,
        DriverObject,
        NULL,
        &gCookie,
        NULL
    );
}

NTSTATUS
RegFltUninitialize()
{
    if (!gCookie.QuadPart)
    {
        return STATUS_NOT_SUPPORTED;
    }

    return CmUnRegisterCallback(
        gCookie
    );
}


NTSTATUS
GetRegistryPath(
    PVOID Object,
    PUNICODE_STRING* RegistryPath
)
{
    NTSTATUS status;
    PUNICODE_STRING path;
    status = CmCallbackGetKeyObjectIDEx(
        &gCookie,
        Object,
        NULL,
        &path,
        0
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    *RegistryPath = path;
    return status;
}

void FreeRegistryPath(
    PUNICODE_STRING RegistryPath
)
{
    CmCallbackReleaseKeyObjectIDEx(
        RegistryPath
    );
}

NTSTATUS 
RegistryCreate(
    PVOID EventStructure)
{
    NTSTATUS status;
    PLARGE_INTEGER currentTime;
    UINT32 processId;
    PMY_DRIVER_MSG_REGISTRY_NOTIFICATION message = NULL;
    PREG_PRE_CREATE_KEY_INFORMATION data = 
        (PREG_PRE_CREATE_KEY_INFORMATION) EventStructure;
    


    KeQuerySystemTime(&currentTime);
    processId = HandleToUlong(PsGetCurrentProcessId());

    PUNICODE_STRING processName;
    UINT32          messageSize = sizeof(MY_DRIVER_MSG_REGISTRY_NOTIFICATION);
    status = GetProcessName(
        (HANDLE)processId,
        &processName
    );
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }
    
    messageSize += processName->Length;
    messageSize += data->CompleteName->Length;

    message = (PMY_DRIVER_MSG_REGISTRY_NOTIFICATION) ExAllocatePoolWithTag(
            PagedPool,
            messageSize,
            REGISTRY_COMMUNICATION_POOL_TAG);
    if (!message)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    message->Header.MessageCode = msgRegistryCreate;
    message->MessagePayload.RegistryCreateEvent.Timestamp.QuadPart = currentTime->QuadPart;
    message->MessagePayload.RegistryCreateEvent.ProcessId = processId;
    message->MessagePayload.RegistryCreateEvent.ProcessPathSize = processName->Length;
    message->MessagePayload.RegistryCreateEvent.KeyNameSize = data->CompleteName->Length;

    memcpy(&message->MessagePayload.RegistryCreateEvent.Data[0],
        processName->Buffer,
        processName->Length);

    memcpy(&message->MessagePayload.RegistryCreateEvent.Data[0 + processName->Length],
        data->CompleteName->Buffer,
        data->CompleteName->Length);

    ExFreePoolWithTag(
        processName,
        REGISTRY_COMMUNICATION_POOL_TAG
    );

    return CommSendMessage(message, messageSize, NULL, NULL);

cleanup:
    if (message)
    {
        ExFreePoolWithTag(
            message,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }
    if (processName)
    {
        ExFreePoolWithTag(
            processName,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }
    return status;
}

NTSTATUS 
RegistrySetValue(
    PVOID EventStructure)
{
    NTSTATUS status;
    UINT32 processId;
    PLARGE_INTEGER currentTime;
    PUNICODE_STRING registryKeyName = NULL;
    PMY_DRIVER_MSG_REGISTRY_NOTIFICATION message = NULL;
    PREG_SET_VALUE_KEY_INFORMATION data = 
        (PREG_SET_VALUE_KEY_INFORMATION) EventStructure;
    PUNICODE_STRING processName;
    UINT32 messageSize = sizeof(MY_DRIVER_MSG_REGISTRY_NOTIFICATION);


    KeQuerySystemTime(&currentTime);
    processId = HandleToUlong(PsGetCurrentProcessId());

    status = GetProcessName(
        (HANDLE)processId,
        &processName
    );
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    status = GetRegistryPath(data->Object, &registryKeyName);
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    messageSize += processName->Length;
    messageSize += data->DataSize;
    messageSize += registryKeyName->Length;

    message = (PMY_DRIVER_MSG_REGISTRY_NOTIFICATION)ExAllocatePoolWithTag(
        PagedPool,
        messageSize,
        REGISTRY_COMMUNICATION_POOL_TAG);
    if (!message)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    message->Header.MessageCode = msgRegistrySetValue;
    message->MessagePayload.RegistrySetValueEvent.Timestamp.QuadPart = currentTime->QuadPart;
    message->MessagePayload.RegistrySetValueEvent.ProcessId = processId;

    message->MessagePayload.RegistrySetValueEvent.ProcessPathSize = processName->Length;
    message->MessagePayload.RegistrySetValueEvent.KeyNameSize = registryKeyName->Length;
    message->MessagePayload.RegistrySetValueEvent.ValueSize = data->DataSize;

    memcpy(&message->MessagePayload.RegistrySetValueEvent.Data[0],
        processName->Buffer,
        processName->Length);

    memcpy(&message->MessagePayload.RegistrySetValueEvent.Data[processName->Length],
        registryKeyName->Buffer,
        registryKeyName->Length);

    memcpy(&message->MessagePayload.RegistrySetValueEvent.Data[processName->Length + registryKeyName->Length],
        data->Data,
        data->DataSize);

    if (registryKeyName)
    {
        FreeRegistryPath(registryKeyName);
    }

    ExFreePoolWithTag(
        processName,
        REGISTRY_COMMUNICATION_POOL_TAG
    );

    return CommSendMessage(message, messageSize, NULL, NULL);

cleanup:
    if (message)
    {
        ExFreePoolWithTag(
            message,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }
    if (registryKeyName)
    {
        FreeRegistryPath(registryKeyName);
    }
    if (processName)
    {
        ExFreePoolWithTag(
            processName,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }
    return status;
}

NTSTATUS 
RegistryDeleteKey(
    PVOID EventStructure)
{
    PREG_DELETE_KEY_INFORMATION data =
        (PREG_DELETE_KEY_INFORMATION)EventStructure;
    PUNICODE_STRING registryKeyName = NULL;
    NTSTATUS status;
    UINT32 processId;
    PLARGE_INTEGER currentTime;
    PMY_DRIVER_MSG_REGISTRY_NOTIFICATION message = NULL;
    PUNICODE_STRING processName;
    UINT32 messageSize = sizeof(MY_DRIVER_MSG_REGISTRY_NOTIFICATION);


    KeQuerySystemTime(&currentTime);
    processId = HandleToUlong(PsGetCurrentProcessId());

    status = GetProcessName(
        (HANDLE)processId,
        &processName
    );
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    status = GetRegistryPath(data->Object, &registryKeyName);
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    messageSize += processName->Length;
    messageSize += registryKeyName->Length;

    message = (PMY_DRIVER_MSG_REGISTRY_NOTIFICATION)ExAllocatePoolWithTag(
        PagedPool,
        messageSize,
        REGISTRY_COMMUNICATION_POOL_TAG);
    if (!message)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }
    message->Header.MessageCode = msgRegistryDeleteKey;
    message->MessagePayload.RegistryDeleteKeyEvent.Timestamp.QuadPart = currentTime->QuadPart;
    message->MessagePayload.RegistryDeleteKeyEvent.ProcessId = processId;

    message->MessagePayload.RegistryDeleteKeyEvent.ProcessPathSize = processName->Length;
    message->MessagePayload.RegistryDeleteKeyEvent.KeyNameSize = registryKeyName->Length;

    memcpy(&message->MessagePayload.RegistryDeleteKeyEvent.Data[0],
        processName->Buffer,
        processName->Length);

    memcpy(&message->MessagePayload.RegistryDeleteKeyEvent.Data[processName->Length],
        registryKeyName->Buffer,
        registryKeyName->Length);

    if (registryKeyName)
    {
        FreeRegistryPath(registryKeyName);
    }

    ExFreePoolWithTag(
        processName,
        REGISTRY_COMMUNICATION_POOL_TAG
    );

    return CommSendMessage(message, messageSize, NULL, NULL);

cleanup:
    if (message)
    {
        ExFreePoolWithTag(
            message,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }

    if (registryKeyName)
    {
        FreeRegistryPath(registryKeyName);
    }
    if (processName)
    {
        ExFreePoolWithTag(
            processName,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }
    return status;
}

NTSTATUS RegistryDeleteValue(PVOID EventStructure)
{
    PREG_DELETE_VALUE_KEY_INFORMATION data = 
        (PREG_DELETE_VALUE_KEY_INFORMATION)EventStructure;
    PUNICODE_STRING registryKeyName = NULL;
    NTSTATUS status;
    UINT32 processId;
    PLARGE_INTEGER currentTime;
    PMY_DRIVER_MSG_REGISTRY_NOTIFICATION message = NULL;
    PUNICODE_STRING processName;
    UINT32 messageSize = sizeof(MY_DRIVER_MSG_REGISTRY_NOTIFICATION);


    KeQuerySystemTime(&currentTime);
    processId = HandleToUlong(PsGetCurrentProcessId());

    status = GetProcessName(
        (HANDLE)processId,
        &processName
    );
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }
    
    status = GetRegistryPath(data->Object, &registryKeyName);
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    messageSize += processName->Length;
    messageSize += registryKeyName->Length;
    messageSize += data->ValueName->Length;

    message = (PMY_DRIVER_MSG_REGISTRY_NOTIFICATION)ExAllocatePoolWithTag(
        PagedPool,
        messageSize,
        REGISTRY_COMMUNICATION_POOL_TAG);
    if (!message)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    message->Header.MessageCode = msgRegistryDeleteValue;
    message->MessagePayload.RegistryDeleteValueEvent.Timestamp.QuadPart = currentTime->QuadPart;
    message->MessagePayload.RegistryDeleteValueEvent.ProcessId = processId;

    message->MessagePayload.RegistryDeleteValueEvent.ProcessPathSize = processName->Length;
    message->MessagePayload.RegistryDeleteValueEvent.KeyNameSize = registryKeyName->Length;
    message->MessagePayload.RegistryDeleteValueEvent.ValueSize = data->ValueName->Length;

    memcpy(&message->MessagePayload.RegistryDeleteValueEvent.Data[0],
        processName->Buffer,
        processName->Length);

    memcpy(&message->MessagePayload.RegistryDeleteValueEvent.Data[0 + processName->Length],
        registryKeyName->Buffer,
        registryKeyName->Length);

    memcpy(&message->MessagePayload.RegistryDeleteValueEvent.Data[0 + processName->Length + registryKeyName->Length],
        data->ValueName->Buffer,
        data->ValueName->Length);

    if (registryKeyName)
    {
        FreeRegistryPath(registryKeyName);
    }

    ExFreePoolWithTag(
        processName,
        REGISTRY_COMMUNICATION_POOL_TAG
    );

    return CommSendMessage(message, messageSize, NULL, NULL);


cleanup:
    if (message)
    {
        ExFreePoolWithTag(
            message,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }
    
    if (registryKeyName)
    {
        FreeRegistryPath(registryKeyName);
    }

    if (processName)
    {
        ExFreePoolWithTag(
            processName,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }
    return status;
}

NTSTATUS 
RegistryLoadKey(
    PVOID EventStructure)
{
    NTSTATUS status;
    PREG_LOAD_KEY_INFORMATION data = 
        (PREG_LOAD_KEY_INFORMATION)EventStructure;
    UINT32 processId;
    PLARGE_INTEGER currentTime;
    PMY_DRIVER_MSG_REGISTRY_NOTIFICATION message = NULL;
    PUNICODE_STRING processName;
    UINT32 messageSize = sizeof(MY_DRIVER_MSG_REGISTRY_NOTIFICATION);


    KeQuerySystemTime(&currentTime);
    processId = HandleToUlong(PsGetCurrentProcessId());

    status = GetProcessName(
        (HANDLE)processId,
        &processName
    );
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    messageSize += processName->Length;
    messageSize += data->KeyName->Length;

    message = (PMY_DRIVER_MSG_REGISTRY_NOTIFICATION)ExAllocatePoolWithTag(
        PagedPool,
        messageSize,
        REGISTRY_COMMUNICATION_POOL_TAG);
    if (!message)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    message->Header.MessageCode = msgRegistryDeleteValue;
    message->MessagePayload.RegistryLoadKeyEvent.Timestamp.QuadPart = currentTime->QuadPart;
    message->MessagePayload.RegistryLoadKeyEvent.ProcessId = processId;

    message->MessagePayload.RegistryLoadKeyEvent.ProcessPathSize = processName->Length;
    message->MessagePayload.RegistryLoadKeyEvent.RootKeySize = data->KeyName->Length;

    memcpy(&message->MessagePayload.RegistryDeleteValueEvent.Data[0],
        processName->Buffer,
        processName->Length);

    memcpy(&message->MessagePayload.RegistryDeleteValueEvent.Data[0 + processName->Length],
        data->KeyName->Buffer,
        data->KeyName->Length);

    ExFreePoolWithTag(
        processName,
        REGISTRY_COMMUNICATION_POOL_TAG
    );

    return CommSendMessage(message, messageSize, NULL, NULL);



cleanup:
    if (message)
    {
        ExFreePoolWithTag(
            message,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }

    if (processName)
    {
        ExFreePoolWithTag(
            processName,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }
    return status;
}

NTSTATUS 
RegistryRenameKey(
    PVOID EventStructure)
{
    NTSTATUS status;
    PUNICODE_STRING registryKeyName = NULL;
    PREG_RENAME_KEY_INFORMATION data = 
        (PREG_RENAME_KEY_INFORMATION)EventStructure;
    UINT32 processId;
    PLARGE_INTEGER currentTime;
    PMY_DRIVER_MSG_REGISTRY_NOTIFICATION message = NULL;
    PUNICODE_STRING processName;
    UINT32 messageSize = sizeof(MY_DRIVER_MSG_REGISTRY_NOTIFICATION);
   
    KeQuerySystemTime(&currentTime);
    processId = HandleToUlong(PsGetCurrentProcessId());

    status = GetProcessName(
        (HANDLE)processId,
        &processName
    );
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    status = GetRegistryPath(data->Object, &registryKeyName);
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    messageSize += processName->Length;
    messageSize += registryKeyName->Length;
    messageSize += data->NewName->Length;
    message = (PMY_DRIVER_MSG_REGISTRY_NOTIFICATION)ExAllocatePoolWithTag(
        PagedPool,
        messageSize,
        REGISTRY_COMMUNICATION_POOL_TAG);
    if (!message)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }
    message->Header.MessageCode = msgRegistryRenameKey;
    message->MessagePayload.RegistryRenameKeyEvent.Timestamp.QuadPart = currentTime->QuadPart;
    message->MessagePayload.RegistryRenameKeyEvent.ProcessId = processId;

    message->MessagePayload.RegistryRenameKeyEvent.ProcessPathSize = processName->Length;
    message->MessagePayload.RegistryRenameKeyEvent.OldKeyNameSize = registryKeyName->Length;
    message->MessagePayload.RegistryRenameKeyEvent.NewKeyNameSize = data->NewName->Length;

    memcpy(&message->MessagePayload.RegistryDeleteValueEvent.Data[0],
        processName->Buffer,
        processName->Length);

    memcpy(&message->MessagePayload.RegistryDeleteValueEvent.Data[0 + processName->Length],
        registryKeyName->Buffer,
        registryKeyName->Length);

    memcpy(&message->MessagePayload.RegistryDeleteValueEvent.Data[0 + processName->Length + registryKeyName->Length],
        data->NewName->Buffer,
        data->NewName->Length);

    if (registryKeyName)
    {
        FreeRegistryPath(registryKeyName);
    }

    ExFreePoolWithTag(
        processName,
        REGISTRY_COMMUNICATION_POOL_TAG
    );

    return CommSendMessage(message, messageSize, NULL, NULL);

cleanup:

    if (message)
    {
        ExFreePoolWithTag(
            message,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }

    if (registryKeyName)
    {
        FreeRegistryPath(registryKeyName);
    }

    if (processName)
    {
        ExFreePoolWithTag(
            processName,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }

    return status;
}