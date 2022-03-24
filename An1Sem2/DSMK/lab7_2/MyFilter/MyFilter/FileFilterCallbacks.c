#include "FileFilterCallbacks.h"
#include "CommShared.h"

NTSTATUS
GenericFileHandler(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ MY_DRIVER_MESSAGE_CODE MessageCode
)
{
    NTSTATUS status;
    PLARGE_INTEGER currentTime;
    UINT32 processId;
    PMY_DRIVER_MSG_FILE_NOTIFICATION message = NULL;
    PUNICODE_STRING processName;
    UINT32 messageSize = sizeof(MY_DRIVER_MSG_FILE_NOTIFICATION);
    BOOLEAN containsFilePath = FALSE;

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

    containsFilePath = !(!FltObjects ||
        !FltObjects->FileObject ||
        !FltObjects->FileObject->FileName.Length ||
        !FltObjects->FileObject->FileName.Buffer);

    messageSize += processName->Length;
    if (containsFilePath)
        messageSize += FltObjects->FileObject->FileName.Length;

    message = (PMY_DRIVER_MSG_FILE_NOTIFICATION)ExAllocatePoolWithTag(
        PagedPool,
        messageSize,
        REGISTRY_COMMUNICATION_POOL_TAG);
    if (!message)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    message->Header.MessageCode = MessageCode;
    message->ProcessId = processId;
    message->Timestamp.QuadPart = currentTime->QuadPart;

    message->ImagePathLength = processName->Length;
    if (containsFilePath)
        message->FilePathLength = FltObjects->FileObject->FileName.Length;
    else
        message->FilePathLength = 0;

    memcpy(&message->Data[0],
        processName->Buffer,
        processName->Length);

    if (containsFilePath)
        memcpy(&message->Data[processName->Length],
            FltObjects->FileObject->FileName.Buffer,
            FltObjects->FileObject->FileName.Length);

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
            REGISTRY_COMMUNICATION_POOL_TAG);
    }

    if (processName)
    {
        ExFreePoolWithTag(
            processName,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_PREOP_CALLBACK_STATUS
FileFilterCallbackCreatePre(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    return GenericFileHandler(
        FltObjects,
        msgFileCreate
    );
}

FLT_PREOP_CALLBACK_STATUS
FileFilterCallbackClosePre(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    return GenericFileHandler(
        FltObjects,
        msgFileClose
    );
}

FLT_PREOP_CALLBACK_STATUS
FileFilterCallbackCleanupPre(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    return GenericFileHandler(
        FltObjects,
        msgFileCleanup
    );
}

FLT_PREOP_CALLBACK_STATUS
FileFilterCallbackReadPre(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    return GenericFileHandler(
        FltObjects,
        msgFileRead
    );
}

FLT_PREOP_CALLBACK_STATUS
FileFilterCallbackWritePre(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    return GenericFileHandler(
        FltObjects,
        msgFileWrite
    );
}

FLT_PREOP_CALLBACK_STATUS
FileFilterCallbackSetattribPre(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    return GenericFileHandler(
        FltObjects,
        msgFileSetAttributes
    );
}