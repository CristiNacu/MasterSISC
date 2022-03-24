#include "ThreadFilter.h"
#include "CommShared.h"

void OnThreadNotify(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create
)
{
    NTSTATUS status;
    PMY_DRIVER_MSG_THREAD_NOTIFICATION message = NULL;
    UINT32 messageSize = sizeof(MY_DRIVER_MSG_THREAD_NOTIFICATION);
    PUNICODE_STRING processName = NULL;
    
    if (ProcessId)
    {
        status = GetProcessName(
            ProcessId,
            &processName
        );
        if (!NT_SUCCESS(status))
        {
            goto cleanup;
        }
        messageSize += processName->Length;
    }

    message = ExAllocatePoolWithTag(
        PagedPool,
        messageSize,
        REGISTRY_COMMUNICATION_POOL_TAG
    );
    if (!message)
    {
        goto cleanup;
    }

    message->Header.MessageCode = Create ? msgThreadCreate : msgThreadTerminate;
    message->ProcessId = HandleToULong(ProcessId);
    message->ThreadId = HandleToULong(ThreadId);
    message->ImagePathLength = processName->Length;
    KeQuerySystemTime(&message->Timestamp);

    memcpy(
        &message->Data[0],
        processName->Buffer,
        processName->Length
    );

    CommSendMessage(message, messageSize, NULL, NULL);
    return;

cleanup:
    if (processName)
    {
        ExFreePoolWithTag(
            processName,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }
    if (message)
    {
        ExFreePoolWithTag(
            message,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }

    return;
}



NTSTATUS
ThreadFltInitialize()
{
    return PsSetCreateThreadNotifyRoutine(OnThreadNotify);
}

NTSTATUS
ThreadFltUninitialize()
{
    return PsRemoveCreateThreadNotifyRoutine(OnThreadNotify);
}
