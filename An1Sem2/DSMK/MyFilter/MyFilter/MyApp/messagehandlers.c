#include "messagehandlers.h"
#include <ntstatus.h>
#include "globaldata.h"
#include "CommShared.h"
#include <malloc.h>

//
// MsgHandleUnknownMessage
//
_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleUnknownMessage(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(InputBufferSize);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    PMY_DRIVER_MESSAGE_HEADER pHeader = (PMY_DRIVER_MESSAGE_HEADER)(InputBuffer + 1);

    wprintf(L"[Error] Unknown message received form driver. Id = %u", pHeader->MessageCode);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}

//
// MsgHandleProcessCreate
//
_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleProcessCreate(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    PMY_DRIVER_PROCESS_NOTIFICATION_FULL_MESSAGE pInput = (PMY_DRIVER_PROCESS_NOTIFICATION_FULL_MESSAGE)InputBuffer;
    PMY_DRIVER_PROCESS_CREATE_FULL_MESSAGE_REPLY  pOutput = (PMY_DRIVER_PROCESS_CREATE_FULL_MESSAGE_REPLY)OutputBuffer;

    *BytesWritten = 0;
    if (InputBufferSize < sizeof(*pInput))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    if (OutputBufferSize < sizeof(*pOutput))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    if (sizeof(*pInput) + pInput->Message.ImagePathLength < sizeof(*pInput))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    if (InputBufferSize < sizeof(*pInput) + pInput->Message.ImagePathLength)
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    if (pOutput)
    {
        *BytesWritten = sizeof(*pOutput);
        pOutput->Reply.Status = STATUS_SUCCESS;
    }

    if (!pInput->Message.ImagePathLength)
    {
        wprintf(L"[PROC] Process Created: id = %d, path = NULL\n", pInput->Message.ProcessId);
        return STATUS_SUCCESS;
    }

    PWCHAR path = malloc(pInput->Message.ImagePathLength + sizeof(WCHAR));
    PWCHAR cmdline = malloc(pInput->Message.CommandLineLength + sizeof(WCHAR));
    if (!path)
    {
        wprintf(L"[PROC] Process Created: id = %d, path = BAD_ALLOC\n", pInput->Message.ProcessId);
        return STATUS_SUCCESS;
    }
    if (!cmdline)
    {
        wprintf(L"[PROC] Process Created: id = %d, path = BAD_ALLOC\n", pInput->Message.ProcessId);
        return STATUS_SUCCESS;
    }

    memcpy(path, &pInput->Message.Data[0], pInput->Message.ImagePathLength);
    memcpy(cmdline, &pInput->Message.Data[0 + pInput->Message.ImagePathLength], pInput->Message.CommandLineLength);
    path[pInput->Message.ImagePathLength >> 1] = L'\0';
    cmdline[pInput->Message.CommandLineLength >> 1] = L'\0';
    wprintf(L"[PROC] Process Created: id = %d, path = %s, cmdline = %s\n", pInput->Message.ProcessId, path, cmdline);
    free(path);
    free(cmdline);

    return STATUS_SUCCESS;
}

_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleProcessTerminate(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    PMY_DRIVER_PROCESS_TERMINATE_MSG_FULL msg = (PMY_DRIVER_PROCESS_TERMINATE_MSG_FULL)InputBuffer;

    if (!InputBuffer || InputBufferSize < sizeof(*msg))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    wprintf(L"[PROC] Pid %u terminates. \n", msg->Msg.ProcessId);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}

_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleStringMessage(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    PMY_DRIVER_MSG_STRING_NOTIFICATION msg = (PMY_DRIVER_MSG_STRING_NOTIFICATION)(InputBuffer + 1);

    if (!InputBuffer || InputBufferSize < sizeof(*msg))
    {
        return STATUS_INVALID_USER_BUFFER;
    }
    
    WCHAR* string= malloc(msg->Length + sizeof(WCHAR));
    if (!string)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    memcpy(string, msg->Data, msg->Length);
    string[msg->Length / sizeof(WCHAR)] = L'\0';
    wprintf(L"%s\n", string);
    free(string);
    
    *BytesWritten = 0;
    return STATUS_SUCCESS;
}

_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleRegistryCreateMessage(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    PMY_DRIVER_MSG_REGISTRY_NOTIFICATION msg = (PMY_DRIVER_MSG_REGISTRY_NOTIFICATION)(InputBuffer + 1);
    
    if (!InputBuffer || InputBufferSize < sizeof(*msg))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    WCHAR* processName = malloc(msg->MessagePayload.RegistryCreateEvent.ProcessPathSize + sizeof(WCHAR));
    WCHAR* keyName = malloc(msg->MessagePayload.RegistryCreateEvent.KeyNameSize + sizeof(WCHAR));
    if (!processName || !keyName)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    memcpy(processName,
        &msg->MessagePayload.RegistryCreateEvent.Data[0],
        msg->MessagePayload.RegistryCreateEvent.ProcessPathSize);

    memcpy(keyName,
        &msg->MessagePayload.RegistryCreateEvent.Data[0 + msg->MessagePayload.RegistryCreateEvent.ProcessPathSize],
        msg->MessagePayload.RegistryCreateEvent.KeyNameSize);

    processName[msg->MessagePayload.RegistryCreateEvent.ProcessPathSize / sizeof(WCHAR)] = L'\0';
    keyName[msg->MessagePayload.RegistryCreateEvent.KeyNameSize / sizeof(WCHAR)] = L'\0';

    wprintf(L"[%lld] [REGISTRY KEY CREATE] [PID: %ud] [PROCESS NAME: %s] [KEY NAME: %s]\n", 
        msg->MessagePayload.RegistryCreateEvent.Timestamp.QuadPart,
        msg->MessagePayload.RegistryCreateEvent.ProcessId,
        processName,
        keyName
    );

    free(processName);
    free(keyName);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}

_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleRegistrySetValueMessage(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    PMY_DRIVER_MSG_REGISTRY_NOTIFICATION msg = (PMY_DRIVER_MSG_REGISTRY_NOTIFICATION)(InputBuffer + 1);

    if (!InputBuffer || InputBufferSize < sizeof(*msg))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    WCHAR* processName = malloc(msg->MessagePayload.RegistrySetValueEvent.ProcessPathSize + sizeof(WCHAR));
    WCHAR* keyName = malloc(msg->MessagePayload.RegistrySetValueEvent.KeyNameSize + sizeof(WCHAR));
    WCHAR* value = malloc(msg->MessagePayload.RegistrySetValueEvent.ValueSize + sizeof(WCHAR));
    if (!processName || !keyName || !value)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    memcpy(processName,
        &msg->MessagePayload.RegistrySetValueEvent.Data[0],
        msg->MessagePayload.RegistrySetValueEvent.ProcessPathSize);

    memcpy(keyName,
        &msg->MessagePayload.RegistrySetValueEvent.Data[0 + msg->MessagePayload.RegistrySetValueEvent.ProcessPathSize],
        msg->MessagePayload.RegistrySetValueEvent.KeyNameSize);

    memcpy(value,
        &msg->MessagePayload.RegistrySetValueEvent.Data[0 + msg->MessagePayload.RegistrySetValueEvent.ProcessPathSize + msg->MessagePayload.RegistrySetValueEvent.KeyNameSize],
        msg->MessagePayload.RegistrySetValueEvent.ValueSize);

    processName[msg->MessagePayload.RegistrySetValueEvent.ProcessPathSize / sizeof(WCHAR)] = L'\0';
    keyName[msg->MessagePayload.RegistrySetValueEvent.KeyNameSize / sizeof(WCHAR)] = L'\0';
    value[msg->MessagePayload.RegistrySetValueEvent.ValueSize / sizeof(WCHAR)] = L'\0';

    wprintf(L"[%lld] [REGISTRY KEY SET VALUE] [PID: %ud] [PROCESS NAME: %s] [KEY NAME: %s] [VALUE: %s]\n",
        msg->MessagePayload.RegistrySetValueEvent.Timestamp.QuadPart,
        msg->MessagePayload.RegistrySetValueEvent.ProcessId,
        processName,
        keyName,
        value
    );

    free(processName);
    free(keyName);
    free(value);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}

_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleRegistryDeleteKeyMessage(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    PMY_DRIVER_MSG_REGISTRY_NOTIFICATION msg = (PMY_DRIVER_MSG_REGISTRY_NOTIFICATION)(InputBuffer + 1);

    if (!InputBuffer || InputBufferSize < sizeof(*msg))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    WCHAR* processName = malloc(msg->MessagePayload.RegistryDeleteKeyEvent.ProcessPathSize + sizeof(WCHAR));
    WCHAR* keyName = malloc(msg->MessagePayload.RegistryDeleteKeyEvent.KeyNameSize + sizeof(WCHAR));
    if (!processName || !keyName)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    memcpy(processName,
        &msg->MessagePayload.RegistryDeleteKeyEvent.Data[0],
        msg->MessagePayload.RegistryDeleteKeyEvent.ProcessPathSize);

    memcpy(keyName,
        &msg->MessagePayload.RegistryDeleteKeyEvent.Data[0 + msg->MessagePayload.RegistryDeleteKeyEvent.ProcessPathSize],
        msg->MessagePayload.RegistryDeleteKeyEvent.KeyNameSize);

    processName[msg->MessagePayload.RegistryDeleteKeyEvent.ProcessPathSize / sizeof(WCHAR)] = L'\0';
    keyName[msg->MessagePayload.RegistryDeleteKeyEvent.KeyNameSize / sizeof(WCHAR)] = L'\0';

    wprintf(L"[%lld] [REGISTRY KEY DELETE KEY] [PID: %ud] [PROCESS NAME: %s] [KEY NAME: %s]\n",
        msg->MessagePayload.RegistryDeleteKeyEvent.Timestamp.QuadPart,
        msg->MessagePayload.RegistryDeleteKeyEvent.ProcessId,
        processName,
        keyName
    );

    free(processName);
    free(keyName);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}

_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleRegistryDeleteValueMessage(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    PMY_DRIVER_MSG_REGISTRY_NOTIFICATION msg = (PMY_DRIVER_MSG_REGISTRY_NOTIFICATION)(InputBuffer + 1);

    if (!InputBuffer || InputBufferSize < sizeof(*msg))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    WCHAR* processName = malloc(msg->MessagePayload.RegistryDeleteValueEvent.ProcessPathSize + sizeof(WCHAR));
    WCHAR* keyName = malloc(msg->MessagePayload.RegistryDeleteValueEvent.KeyNameSize + sizeof(WCHAR));
    WCHAR* value = malloc(msg->MessagePayload.RegistryDeleteValueEvent.ValueSize + sizeof(WCHAR));
    if (!processName || !keyName || !value)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    memcpy(processName,
        &msg->MessagePayload.RegistryDeleteValueEvent.Data[0],
        msg->MessagePayload.RegistryDeleteValueEvent.ProcessPathSize);

    memcpy(keyName,
        &msg->MessagePayload.RegistryDeleteValueEvent.Data[0 + msg->MessagePayload.RegistryDeleteValueEvent.ProcessPathSize],
        msg->MessagePayload.RegistryDeleteValueEvent.KeyNameSize);

    memcpy(value,
        &msg->MessagePayload.RegistryDeleteValueEvent.Data[0 + msg->MessagePayload.RegistryDeleteValueEvent.ProcessPathSize + msg->MessagePayload.RegistryDeleteValueEvent.KeyNameSize],
        msg->MessagePayload.RegistryDeleteValueEvent.ValueSize);

    processName[msg->MessagePayload.RegistryDeleteValueEvent.ProcessPathSize / sizeof(WCHAR)] = L'\0';
    keyName[msg->MessagePayload.RegistryDeleteValueEvent.KeyNameSize / sizeof(WCHAR)] = L'\0';
    value[msg->MessagePayload.RegistryDeleteValueEvent.ValueSize / sizeof(WCHAR)] = L'\0';

    wprintf(L"[%lld] [REGISTRY KEY DELETE VALUE] [PID: %ud] [PROCESS NAME: %s] [KEY NAME: %s] [VALUE: %s]\n",
        msg->MessagePayload.RegistryDeleteValueEvent.Timestamp.QuadPart,
        msg->MessagePayload.RegistryDeleteValueEvent.ProcessId,
        processName,
        keyName,
        value
    );

    free(processName);
    free(keyName);
    free(value);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}

_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleRegistryLoadKeyMessage(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    PMY_DRIVER_MSG_REGISTRY_NOTIFICATION msg = (PMY_DRIVER_MSG_REGISTRY_NOTIFICATION)(InputBuffer + 1);

    if (!InputBuffer || InputBufferSize < sizeof(*msg))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    WCHAR* processName = malloc(msg->MessagePayload.RegistryLoadKeyEvent.ProcessPathSize + sizeof(WCHAR));
    WCHAR* keyName = malloc(msg->MessagePayload.RegistryLoadKeyEvent.RootKeySize + sizeof(WCHAR));
    if (!processName || !keyName )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    memcpy(processName,
        &msg->MessagePayload.RegistryLoadKeyEvent.Data[0],
        msg->MessagePayload.RegistryLoadKeyEvent.ProcessPathSize);

    memcpy(keyName,
        &msg->MessagePayload.RegistryLoadKeyEvent.Data[0 + msg->MessagePayload.RegistryLoadKeyEvent.ProcessPathSize],
        msg->MessagePayload.RegistryLoadKeyEvent.RootKeySize);


    processName[msg->MessagePayload.RegistryDeleteValueEvent.ProcessPathSize / sizeof(WCHAR)] = L'\0';
    keyName[msg->MessagePayload.RegistryDeleteValueEvent.KeyNameSize / sizeof(WCHAR)] = L'\0';

    wprintf(L"[%lld] [REGISTRY KEY LOAD] [PID: %ud] [PROCESS NAME: %s] [KEY NAME: %s]\n",
        msg->MessagePayload.RegistryLoadKeyEvent.Timestamp.QuadPart,
        msg->MessagePayload.RegistryLoadKeyEvent.ProcessId,
        processName,
        keyName
    );

    free(processName);
    free(keyName);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}

_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleRegistryRenameKeyMessage(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    PMY_DRIVER_MSG_REGISTRY_NOTIFICATION msg = (PMY_DRIVER_MSG_REGISTRY_NOTIFICATION)(InputBuffer + 1);

    if (!InputBuffer || InputBufferSize < sizeof(*msg))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    WCHAR* processName = malloc(msg->MessagePayload.RegistryRenameKeyEvent.ProcessPathSize + sizeof(WCHAR));
    WCHAR* oldKeyName = malloc(msg->MessagePayload.RegistryRenameKeyEvent.OldKeyNameSize + sizeof(WCHAR));
    WCHAR* newKeyName = malloc(msg->MessagePayload.RegistryRenameKeyEvent.NewKeyNameSize + sizeof(WCHAR));
    if (!processName || !oldKeyName || !newKeyName)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    memcpy(processName,
        &msg->MessagePayload.RegistryRenameKeyEvent.Data[0],
        msg->MessagePayload.RegistryRenameKeyEvent.ProcessPathSize);

    memcpy(oldKeyName,
        &msg->MessagePayload.RegistryRenameKeyEvent.Data[0 + msg->MessagePayload.RegistryRenameKeyEvent.ProcessPathSize],
        msg->MessagePayload.RegistryRenameKeyEvent.OldKeyNameSize);

    memcpy(newKeyName,
        &msg->MessagePayload.RegistryRenameKeyEvent.Data[0 + msg->MessagePayload.RegistryRenameKeyEvent.ProcessPathSize + msg->MessagePayload.RegistryRenameKeyEvent.OldKeyNameSize],
        msg->MessagePayload.RegistryRenameKeyEvent.NewKeyNameSize);

    processName[msg->MessagePayload.RegistryRenameKeyEvent.ProcessPathSize / sizeof(WCHAR)] = L'\0';
    oldKeyName[msg->MessagePayload.RegistryRenameKeyEvent.OldKeyNameSize / sizeof(WCHAR)] = L'\0';
    newKeyName[msg->MessagePayload.RegistryRenameKeyEvent.NewKeyNameSize / sizeof(WCHAR)] = L'\0';

    wprintf(L"[%lld] [REGISTRY KEY RENAME] [PID: %ud] [PROCESS NAME: %s] [OLD KEY NAME: %s] [NEW KEY NAME: %s]\n",
        msg->MessagePayload.RegistryRenameKeyEvent.Timestamp.QuadPart,
        msg->MessagePayload.RegistryRenameKeyEvent.ProcessId,
        processName,
        oldKeyName,
        newKeyName
    );

    free(processName);
    free(oldKeyName);
    free(newKeyName);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}

NTSTATUS
MsgHandleThreadCreationMessage(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _In_ MY_DRIVER_MESSAGE_CODE MessageCode,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    PMY_DRIVER_MSG_THREAD_NOTIFICATION msg = (PMY_DRIVER_MSG_THREAD_NOTIFICATION)(InputBuffer + 1);

    if (!InputBuffer || InputBufferSize < sizeof(*msg))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    WCHAR* processName = malloc(msg->ImagePathLength + sizeof(WCHAR));
    if (!processName)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    memcpy(processName,
        &msg->Data[0],
        msg->ImagePathLength);


    processName[msg->ImagePathLength / sizeof(WCHAR)] = L'\0';

    WCHAR* actionName =
        MessageCode == msgThreadCreate ? L"THREAD CREATE" :
        MessageCode == msgThreadTerminate ? L"THREAD TERMINATE" :
        L"UNKOWN THREAD OPERATION";

    wprintf(L"[%lld] [%s] [PID: %ud] [PROCESS NAME: %s]\n",
        msg->Timestamp.QuadPart,
        actionName,
        msg->ProcessId,
        processName
    );

    free(processName);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}

NTSTATUS
MsgGenericFileHandler(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _In_ MY_DRIVER_MESSAGE_CODE MessageCode,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    PMY_DRIVER_MSG_FILE_NOTIFICATION msg = (PMY_DRIVER_MSG_FILE_NOTIFICATION)(InputBuffer + 1);

    if (!InputBuffer || InputBufferSize < sizeof(*msg))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    WCHAR* processName = malloc(msg->ImagePathLength + sizeof(WCHAR));
    WCHAR* filePath = malloc(msg->FilePathLength + sizeof(WCHAR));
    if (!processName || !filePath)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    memcpy(processName,
        &msg->Data[0],
        msg->ImagePathLength);

    memcpy(filePath,
        &msg->Data[0 + msg->ImagePathLength],
        msg->FilePathLength);


    processName[msg->ImagePathLength / sizeof(WCHAR)] = L'\0';
    filePath[msg->FilePathLength / sizeof(WCHAR)] = L'\0';

    WCHAR* actionName =
        MessageCode == msgFileCreate ? L"FILE CREATE" :
        MessageCode == msgFileClose ? L"FILE CLOSE" :
        MessageCode == msgFileCleanup ? L"FILE CLEANUP" :
        MessageCode == msgFileRead ? L"FILE READ" :
        MessageCode == msgFileWrite ? L"FILE WRITE" :
        MessageCode == msgFileSetAttributes ? L"FILE SET ATTRIBUTES" :
        L"UNKOWN FILE OPERATION";

    wprintf(L"[%lld] [%s] [PID: %ud] [PROCESS NAME: %s] [FILE PATH: %s]\n",
        msg->Timestamp.QuadPart,
        actionName,
        msg->ProcessId,
        processName,
        filePath
    );

    free(processName);
    free(filePath);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}

NTSTATUS
MsgHandleImageLoadMessage(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    PMY_DRIVER_MSG_IMAGE_NOTIFICATION msg = (PMY_DRIVER_MSG_IMAGE_NOTIFICATION)(InputBuffer + 1);

    if (!InputBuffer || InputBufferSize < sizeof(*msg))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    WCHAR* processName = malloc(msg->ImagePathLength + sizeof(WCHAR));
    if (!processName)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    memcpy(processName,
        &msg->Data[0],
        msg->ImagePathLength);


    processName[msg->ImagePathLength / sizeof(WCHAR)] = L'\0';

    wprintf(L"[%lld] [IMAGE LOAD] [PID: %ud] [PROCESS NAME: %s]\n",
        msg->Timestamp.QuadPart,
        msg->ProcessId,
        processName
    );

    free(processName);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}

//
// MsgDispatchNewMessage
//
_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgDispatchNewMessage(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    
    DWORD bytesWritten = 0;

    if (InputBufferSize < sizeof(FILTER_MESSAGE_HEADER) + sizeof(MY_DRIVER_COMMAND_HEADER))
    {
        wprintf(L"[Error] Message size is too small to dispatch. Size = %d\n", InputBufferSize);
        return STATUS_BUFFER_TOO_SMALL;
    }

    PMY_DRIVER_MESSAGE_HEADER pHeader = (PMY_DRIVER_MESSAGE_HEADER)(InputBuffer + 1);

    if (pHeader->MessageCode == msgProcessCreate)
        status = MsgHandleProcessCreate(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, &bytesWritten);
    else if (pHeader->MessageCode == msgProcessTerminate)
        status = MsgHandleProcessTerminate(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, &bytesWritten);
    else if (pHeader->MessageCode == msgSendString)
        status = MsgHandleStringMessage(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, &bytesWritten);
    else if (pHeader->MessageCode == msgRegistryCreate)
        status = MsgHandleRegistryCreateMessage(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, &bytesWritten);
    else if (pHeader->MessageCode == msgRegistrySetValue)
        status = MsgHandleRegistrySetValueMessage(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, &bytesWritten);
    else if (pHeader->MessageCode == msgRegistryDeleteKey)
        status = MsgHandleRegistryDeleteKeyMessage(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, &bytesWritten);
    else if (pHeader->MessageCode == msgRegistryDeleteValue)
        status = MsgHandleRegistryDeleteValueMessage(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, &bytesWritten);
    else if (pHeader->MessageCode == msgRegistryLoadkey)
        status = MsgHandleRegistryLoadKeyMessage(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, &bytesWritten);
    else if (pHeader->MessageCode == msgRegistryRenameKey)
        status = MsgHandleRegistryRenameKeyMessage(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, &bytesWritten);
    else if (pHeader->MessageCode > msgFileOperationStartDoNotUse && pHeader->MessageCode < msgFileOperationEndDoNotUse)
        status = MsgGenericFileHandler(InputBuffer, InputBufferSize, pHeader->MessageCode, OutputBuffer, OutputBufferSize, &bytesWritten);
    else if (pHeader->MessageCode == msgThreadCreate || pHeader->MessageCode == msgThreadTerminate)
        status = MsgHandleThreadCreationMessage(InputBuffer, InputBufferSize, pHeader->MessageCode, OutputBuffer, OutputBufferSize, &bytesWritten);
    else if (pHeader->MessageCode == msgImageLoad)
        status = MsgHandleImageLoadMessage(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, &bytesWritten);
    else
        status =  MsgHandleUnknownMessage(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, &bytesWritten);

    if (BytesWritten)
    {
        *BytesWritten = bytesWritten;
    }

    return status;
}
