
#include "CommSharedDefs.h"
#include <ntstatus.h>
#include <devguid.h>

DECLARE_CONST_UNICODE_STRING(
COMM_QUEUE_DEVICE_PROTECTION,
L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGWGX;;;WD)(A;;GRGWGX;;;RC)");
extern const UNICODE_STRING  COMM_QUEUE_DEVICE_PROTECTION;

DECLARE_CONST_UNICODE_STRING(
COMM_QUEUE_DEVICE_PROTECTION_FULL,
L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;WD)(A;;GA;;;RC)");
extern const UNICODE_STRING  COMM_QUEUE_DEVICE_PROTECTION_FULL;

typedef struct _COMM_QUEUE_DEVICE_CONTEXT {
    PCOMM_DATA       Data;
    WDFQUEUE         NotificationQueue;
    volatile LONG    Sequence;
} COMM_QUEUE_DEVICE_CONTEXT, * PCOMM_QUEUE_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(COMM_QUEUE_DEVICE_CONTEXT, CommGetContextFromDevice)

EVT_WDF_DEVICE_FILE_CREATE                  CommEvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE                          CommEvtFileClose;
EVT_WDF_FILE_CLEANUP                        CommpIngonreOperation;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL          CommEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL CommEvtIoInternalDeviceControl;

VOID
CommpOperationNotSupported(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request
)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Request);

    NT_ASSERTMSG("Operation not allowed!", FALSE);

    WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);

    return;
}

NTSTATUS
CommpGetRequestBuffers(
    _In_ WDFREQUEST Request,
    _In_ SIZE_T InputBufferLength,
    _In_ SIZE_T OutputBufferLength,
    _Out_ PVOID* InputBuffer,
    _Out_ PVOID* OutputBuffer)
{
    size_t bufferSize = 0;

    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        Request,
        InputBufferLength,
        InputBuffer,
        &bufferSize
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = WdfRequestRetrieveOutputBuffer(
        Request,
        OutputBufferLength,
        OutputBuffer,
        &bufferSize
    );
    if (!NT_SUCCESS(status))
    {
        *OutputBuffer = NULL;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
CommpDispatchUserRequest(
    _In_  PCOMM_DATA Data,
    _In_  WDFREQUEST Request,
    _In_  SIZE_T     InputBufferLength,
    _In_  SIZE_T     OutputBufferLength,
    _Out_ PUINT32    BytesWritten
)
{
    PVOID  inputBuffer = NULL;
    PVOID outputBuffer = NULL;

    *BytesWritten = 0;

    if (!Data->Configuration.CommReceiveData)
    {
        return STATUS_SUCCESS;
    }

    NTSTATUS status = CommpGetRequestBuffers(Request,
        InputBufferLength, OutputBufferLength, &inputBuffer, &outputBuffer);
    if (NT_SUCCESS(status))
    {
        status = Data->Configuration.CommReceiveData(WdfRequestGetFileObject(Request),
            inputBuffer, (UINT32)InputBufferLength, outputBuffer, (UINT32)OutputBufferLength,
            BytesWritten);
    }

    return status;
}

NTSTATUS
CommpEnqueueInvertedCallRequest(
    _In_opt_ WDFQUEUE NotificationQueue,
    _In_     WDFREQUEST Request
)
{
    if (!NotificationQueue)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    return WdfRequestForwardToIoQueue(Request, NotificationQueue);
}

NTSTATUS
CommpDispatchInvertedCallReply(
    _In_  PCOMM_DATA Data,
    _In_  WDFREQUEST Request,
    _In_  SIZE_T  InputBufferLength,
    _In_  SIZE_T  OutputBufferLength,
    _Out_ PUINT32 BytesWritten
)
{
    PVOID  inputBuffer = NULL;
    PVOID  outputBuffer = NULL;
    BOOLEAN found = FALSE;
    int replyIndex;

    NTSTATUS status = CommpGetRequestBuffers(
        Request,
        InputBufferLength,
        OutputBufferLength,
        &inputBuffer,
        &outputBuffer);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    for (replyIndex = 0; replyIndex < COMM_QUEUE_MAX_REPLY_DATA_COUNT; ++replyIndex)
    {
        if (((PCOMM_INVERTED_HEADER)inputBuffer)->Sequence == Data->Replies[replyIndex].Sequence)
        {
            found = TRUE;

            Data->Replies[replyIndex].ReplyRequest = Request;
            Data->Replies[replyIndex].InputBuffer = inputBuffer;
            Data->Replies[replyIndex].InputBufferSize = InputBufferLength;
            Data->Replies[replyIndex].OutputBuffer = outputBuffer;
            Data->Replies[replyIndex].OutputBufferSize = OutputBufferLength;

            KeSetEvent(&Data->Replies[replyIndex].ReplyAvailable, IO_NO_INCREMENT, FALSE);
            break;
        }
    }

    if (!found)
    {
        *BytesWritten = 0;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    status = KeWaitForSingleObject(&Data->Replies[replyIndex].ReplyProcessed,
        Executive, KernelMode, FALSE, NULL);
    RtlZeroMemory(&Data->Replies[replyIndex], sizeof(Data->Replies[replyIndex]));

    *BytesWritten = sizeof(COMM_INVERTED_HEADER);
    return STATUS_SUCCESS;
}

VOID
CommEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    NTSTATUS status = STATUS_SUCCESS;
    UINT32 bytesWritten = 0;
    BOOLEAN completeRequest = TRUE;

    __debugbreak();

    PCOMM_QUEUE_DEVICE_CONTEXT ctx = CommGetContextFromDevice(WdfIoQueueGetDevice(Queue));
    if (!ctx)
    {
        WdfRequestCompleteWithInformation(Request, STATUS_INVALID_DEVICE_REQUEST, bytesWritten);
        return;
    }


    switch (IoControlCode)
    {
    case COMM_QUEUE_IOCTL_MSG_UM_TO_KM_DIRECT:
    {
        status = CommpDispatchUserRequest(ctx->Data,
            Request, InputBufferLength, OutputBufferLength, &bytesWritten);
    }
    break;
    case COMM_QUEUE_IOCTL_INVERTED_CALL_GET_MESSAGE:
    {
        __debugbreak();
        status = CommpEnqueueInvertedCallRequest(ctx->NotificationQueue, Request);
        completeRequest = NT_SUCCESS(status) ? FALSE : TRUE;
    }
    break;
    case COMM_QUEUE_IOCTL_INVERTED_CALL_REPLY:
    {
        status = CommpDispatchInvertedCallReply(ctx->Data,
            Request, InputBufferLength, OutputBufferLength, &bytesWritten);
    }
    break;
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        bytesWritten = 0;
        break;
    }

    if (completeRequest)
    {
        WdfRequestCompleteWithInformation(Request, status, bytesWritten);
    }

    return;
}

VOID
CommEvtIoInternalDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    UNREFERENCED_PARAMETER(IoControlCode);

    __debugbreak();

    UINT32 bytesWritten = 0;
    PVOID inputBuffer = NULL;
    PVOID outputBuffer = NULL;

    PCOMM_QUEUE_DEVICE_CONTEXT ctx = CommGetContextFromDevice(WdfIoQueueGetDevice(Queue));
    if (!ctx)
    {
        WdfRequestCompleteWithInformation(Request, STATUS_INVALID_DEVICE_REQUEST, 0);
        return;
    }

    if (!ctx->Data->Configuration.CommInternalDeviceControl)
    {
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, 0);
        return;
    }

    NTSTATUS status = CommpGetRequestBuffers(Request,
        COMM_QUEUE_MIN_MESSAGE_SIZE,
        COMM_QUEUE_MIN_MESSAGE_SIZE,
        &inputBuffer,
        &outputBuffer
    );
    if (NT_SUCCESS(status))
    {
        status = ctx->Data->Configuration.CommInternalDeviceControl(WdfRequestGetFileObject(Request),
            inputBuffer, (UINT32)InputBufferLength, outputBuffer, (UINT32)OutputBufferLength, &bytesWritten);
    }

    WdfRequestCompleteWithInformation(Request, status, bytesWritten);
}

VOID
CommEvtDeviceFileCreate(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ WDFFILEOBJECT FileObject
)
{
    NTSTATUS status = STATUS_SUCCESS;

    PCOMM_QUEUE_DEVICE_CONTEXT ctx = CommGetContextFromDevice(Device);
    if (ctx->Data->Configuration.CommClientConnected)
    {
        status = ctx->Data->Configuration.CommClientConnected(FileObject);
        if (NT_SUCCESS(status))
        {
            status = STATUS_SUCCESS;
        }
    }

    WdfRequestComplete(Request, status);
}

VOID
CommEvtFileClose(
    _In_ WDFFILEOBJECT FileObject
)
{
    PCOMM_QUEUE_DEVICE_CONTEXT ctx = CommGetContextFromDevice(WdfFileObjectGetDevice(FileObject));
    if (ctx->Data->Configuration.CommClientDisconnected)
    {
        ctx->Data->Configuration.CommClientDisconnected(FileObject);
    }
}

VOID
CommpIngonreOperation(
    _In_ WDFFILEOBJECT FileObject
)
{
    UNREFERENCED_PARAMETER(FileObject);
}

NTSTATUS
CommInitialize(
    _In_  WDFDRIVER Driver,
    _In_  PCOMM_CONFIGURATION Configuration,
    _Out_ PCOMM_DATA Data
)
{
    WDFQUEUE                        queue = { 0 };
    WDF_IO_QUEUE_CONFIG             ioQueueConfig = { 0 };
    WDF_FILEOBJECT_CONFIG           fileConfig = { 0 };
    WDF_OBJECT_ATTRIBUTES           objAttributes;
    WDFDEVICE                       controlDevice = NULL;
    PCOMM_QUEUE_DEVICE_CONTEXT      devContext = NULL;
    PWDFDEVICE_INIT                 deviceInit = NULL;
    UNICODE_STRING                  deviceName = { 0 };

    RtlSecureZeroMemory(Data, sizeof(COMM_DATA));
    Data->Configuration = *Configuration;

    KeInitializeEvent(&Data->StopEvent, NotificationEvent, FALSE);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objAttributes,
        COMM_QUEUE_DEVICE_CONTEXT);

    objAttributes.ExecutionLevel = WdfExecutionLevelPassive;
    objAttributes.EvtCleanupCallback = CommpIngonreOperation;
    objAttributes.EvtDestroyCallback = CommpIngonreOperation;

    deviceInit = WdfControlDeviceInitAllocate(Driver, &COMM_QUEUE_DEVICE_PROTECTION_FULL);
    if (deviceInit == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitUnicodeString(&deviceName, Data->Configuration.NativeDeviceName);
    NTSTATUS status = WdfDeviceInitAssignName(deviceInit, &deviceName);
    if (!NT_SUCCESS(status))
    {
        WdfDeviceInitFree(deviceInit);
        deviceInit = NULL;
        return status;
    }

    status = WdfDeviceInitAssignSDDLString(deviceInit, &COMM_QUEUE_DEVICE_PROTECTION_FULL);
    if (!NT_SUCCESS(status))
    {
        WdfDeviceInitFree(deviceInit);
        deviceInit = NULL;
        return status;
    }

    WdfDeviceInitSetCharacteristics(deviceInit, (FILE_DEVICE_SECURE_OPEN | FILE_CHARACTERISTIC_PNP_DEVICE), FALSE);

    WDF_FILEOBJECT_CONFIG_INIT(&fileConfig,
        CommEvtDeviceFileCreate,
        CommEvtFileClose,
        CommpIngonreOperation
    );

    fileConfig.AutoForwardCleanupClose = WdfTrue;

    WdfDeviceInitSetFileObjectConfig(deviceInit, &fileConfig, WDF_NO_OBJECT_ATTRIBUTES);

    status = WdfDeviceCreate(&deviceInit, &objAttributes, &controlDevice);
    if (!NT_SUCCESS(status))
    {
        WdfDeviceInitFree(deviceInit);
        deviceInit = NULL;
        return status;
    }

    RtlInitUnicodeString(&deviceName, Data->Configuration.UserDeviceName);
    status = WdfDeviceCreateSymbolicLink(controlDevice, &deviceName);
    if (!NT_SUCCESS(status))
    {
        WdfObjectDelete(Data->CommDevice);
        return status;
    }

    Data->CommDevice = controlDevice;

    devContext = CommGetContextFromDevice(controlDevice);
    if (devContext == NULL)
    {
        WdfObjectDelete(Data->CommDevice);
        return STATUS_INVALID_DEVICE_STATE;
    }

    devContext->Data = Data;
    devContext->Sequence = 0;
    devContext->NotificationQueue = NULL;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);
    ioQueueConfig.Settings.Parallel.NumberOfPresentedRequests = ((ULONG)-1);

    ioQueueConfig.EvtIoDefault = (PFN_WDF_IO_QUEUE_IO_DEFAULT)CommpOperationNotSupported;
    ioQueueConfig.EvtIoRead = (PFN_WDF_IO_QUEUE_IO_READ)CommpOperationNotSupported;
    ioQueueConfig.EvtIoWrite = (PFN_WDF_IO_QUEUE_IO_WRITE)CommpOperationNotSupported;
    ioQueueConfig.EvtIoStop = (PFN_WDF_IO_QUEUE_IO_STOP)CommpOperationNotSupported;
    ioQueueConfig.EvtIoResume = (PFN_WDF_IO_QUEUE_IO_RESUME)CommpOperationNotSupported;
    ioQueueConfig.EvtIoCanceledOnQueue = (PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE)CommpOperationNotSupported;

    ioQueueConfig.EvtIoDeviceControl = CommEvtIoDeviceControl;
    ioQueueConfig.EvtIoInternalDeviceControl = CommEvtIoInternalDeviceControl;

    ioQueueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(controlDevice, &ioQueueConfig, NULL, &queue);
    if (!NT_SUCCESS(status))
    {
        WdfObjectDelete(Data->CommDevice);
        return status;
    }

    WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchManual);

    status = WdfIoQueueCreate(controlDevice, &ioQueueConfig, NULL, &devContext->NotificationQueue);
    if (!NT_SUCCESS(status))
    {
        WdfObjectDelete(Data->CommDevice);
        return status;
    }

    WdfControlFinishInitializing(controlDevice);

    return status;
}

NTSTATUS
CommStart(
    _In_ PCOMM_DATA Data
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (Data->CommDevice)
    {
        WdfIoQueuePurge(
            WdfDeviceGetDefaultQueue(Data->CommDevice),
            WDF_NO_EVENT_CALLBACK,
            WDF_NO_CONTEXT
        );

        WdfIoQueueStart(WdfDeviceGetDefaultQueue(Data->CommDevice));
    }
    else
    {
        status = STATUS_DEVICE_NOT_READY;
    }

    return status;
}

NTSTATUS
CommStop(
    _In_ PCOMM_DATA Data
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (Data->CommDevice)
    {
        WDFQUEUE defaultQueue;

        KeSetEvent(&Data->StopEvent, IO_NO_INCREMENT, FALSE);

        defaultQueue = WdfDeviceGetDefaultQueue(Data->CommDevice);

        if (defaultQueue)
        {
            WdfIoQueuePurgeSynchronously(defaultQueue);
            WdfIoQueueStop(defaultQueue, NULL, NULL);
        }
    }
    else
    {
        status = STATUS_DEVICE_NOT_READY;
    }

    return status;
}

NTSTATUS
CommpDequeueInvertedCallRequest(
    _In_  WDFFILEOBJECT FileObject,
    _Out_ WDFREQUEST* Request
)
{
    WDFDEVICE device = WdfFileObjectGetDevice(FileObject);
    if (!device)
    {
        return STATUS_DEVICE_NOT_READY;
    }

    PCOMM_QUEUE_DEVICE_CONTEXT devContext = CommGetContextFromDevice(device);
    if (!devContext)
    {
        return STATUS_DEVICE_NOT_READY;
    }

    return WdfIoQueueRetrieveRequestByFileObject(
        devContext->NotificationQueue, FileObject, Request);
}

PCOMM_REPLY_EVENT_DATA
CommpGetEmptyReplyData(
    _In_ PCOMM_INVERTED_HEADER InvertedMessageHeader,
    _In_ PCOMM_DATA Data
)
{
    PCOMM_REPLY_EVENT_DATA replyData = NULL;
    LONG idx = 0;
    LONG64 seqId = InvertedMessageHeader->Sequence;
    do
    {
        idx = (_InterlockedIncrement(&Data->NextFree) % COMM_QUEUE_MAX_REPLY_DATA_COUNT);
    } while ((Data->Replies[idx].Sequence != 0));

    replyData = &Data->Replies[idx];
    replyData->Sequence = seqId;
    return replyData;
}

NTSTATUS
CommpWaitForReply(
    _In_  PCOMM_DATA Data,
    _In_  PCOMM_INVERTED_HEADER InvertedMessageHeader,
    _In_  WDFREQUEST Request,
    _In_  UINT32 InputBufferSize,
    _In_  UINT64 Timeout,
    _In_  UINT32 OutputBufferSize,
    _Out_ PVOID OutputBuffer,
    _Out_ UINT32* ActualOutputBufferSize
)
{
    PCOMM_REPLY_EVENT_DATA replyData = CommpGetEmptyReplyData(InvertedMessageHeader, Data);

    KeInitializeEvent(&replyData->ReplyAvailable, NotificationEvent, FALSE);
    KeInitializeEvent(&replyData->ReplyProcessed, NotificationEvent, FALSE);
    replyData->ReplyRequest = NULL;

    PKEVENT events[2];
    events[0] = &replyData->ReplyAvailable;
    events[1] = &Data->StopEvent;

    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, InputBufferSize + sizeof(COMM_INVERTED_HEADER));
    Request = NULL;

    LARGE_INTEGER timeout = { 0 };
    timeout.QuadPart = Timeout;
    NTSTATUS waitStatus = KeWaitForMultipleObjects(2, events, WaitAny, Executive, KernelMode, FALSE, timeout.QuadPart ? &timeout : NULL, NULL);
    if (waitStatus == STATUS_WAIT_0)
    {
        InvertedMessageHeader = (PCOMM_INVERTED_HEADER)replyData->OutputBuffer;
        if (NT_SUCCESS(InvertedMessageHeader->Status))
        {
            if (OutputBufferSize <= (replyData->OutputBufferSize - sizeof(COMM_INVERTED_HEADER)))
            {
                RtlCopyMemory(OutputBuffer,
                    ((PUINT8)replyData->OutputBuffer + sizeof(COMM_INVERTED_HEADER)),
                    (replyData->OutputBufferSize - sizeof(COMM_INVERTED_HEADER))
                );
            }
            else
            {
                return STATUS_BUFFER_TOO_SMALL;
            }

            if (ActualOutputBufferSize)
            {
                *ActualOutputBufferSize = (UINT32)(replyData->OutputBufferSize - sizeof(COMM_INVERTED_HEADER));
            }
        }
        else
        {
            if (ActualOutputBufferSize)
            {
                *ActualOutputBufferSize = 0;
            }

            return InvertedMessageHeader->Status;
        }
    }
    else
    {
        return STATUS_TIMEOUT;
    }

    KeSetEvent(&replyData->ReplyProcessed, IO_NO_INCREMENT, FALSE);
    return STATUS_SUCCESS;
}

NTSTATUS
CommSendMessage(
    _In_ PCOMM_DATA Data,
    _In_ WDFFILEOBJECT FileObject,
    _In_ PVOID          InputBuffer,
    _In_ UINT32         InputBufferSize,
    _Inout_opt_ PVOID   OutputBuffer,
    _Inout_opt_ UINT32  OutputBufferSize,
    _Out_opt_   UINT32* ActualOutputBufferSize,
    _In_opt_    UINT64 Timeout
)
{
    WDFREQUEST request = NULL;

    if (0 == InputBufferSize || InputBufferSize > COMM_QUEUE_MAX_MESSAGE_SIZE)
    {
        return STATUS_INVALID_PARAMETER_4;
    }

    WDFDEVICE device = WdfFileObjectGetDevice(FileObject);
    if (!device)
    {
        return STATUS_DEVICE_NOT_READY;
    }

    PCOMM_QUEUE_DEVICE_CONTEXT devContext = CommGetContextFromDevice(device);
    if (!devContext)
    {
        return STATUS_DEVICE_NOT_READY;
    }

    NTSTATUS status = WdfIoQueueRetrieveRequestByFileObject(
        devContext->NotificationQueue, FileObject, &request);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    PCOMM_INVERTED_HEADER invertedMessageHeader = NULL;
    size_t bufferSize = 0;
    status = WdfRequestRetrieveOutputBuffer(
        request,
        sizeof(COMM_INVERTED_HEADER),
        &invertedMessageHeader,
        &bufferSize
    );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (bufferSize < (InputBufferSize + sizeof(COMM_INVERTED_HEADER)))
    {
        WdfRequestRequeue(request);
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlSecureZeroMemory((PUINT8)invertedMessageHeader + sizeof(invertedMessageHeader), bufferSize - sizeof(invertedMessageHeader));
    invertedMessageHeader->Sequence = _InterlockedIncrement(&devContext->Sequence);
    invertedMessageHeader->Status = STATUS_SUCCESS;
    invertedMessageHeader->ReplySize = 0;
    if ((OutputBuffer != NULL) && (OutputBufferSize != 0))
    {
        invertedMessageHeader->ReplySize = OutputBufferSize;
    }

    RtlCopyMemory((PUINT8)invertedMessageHeader + sizeof(COMM_INVERTED_HEADER), InputBuffer, InputBufferSize);

    if ((OutputBuffer != NULL) && (OutputBufferSize != 0))
    {
        status = CommpWaitForReply(Data,
            invertedMessageHeader,
            request,
            InputBufferSize,
            Timeout,
            OutputBufferSize,
            OutputBuffer,
            ActualOutputBufferSize
        );
    }
    else
    {
        if (ActualOutputBufferSize)
        {
            *ActualOutputBufferSize = 0;
        }
    }

    if (NT_SUCCESS(status))
    {
        WdfRequestCompleteWithInformation(request, status, InputBufferSize + sizeof(COMM_INVERTED_HEADER));
    }
    else
    {
        WdfRequestComplete(request, status);
    }

    return status;
}

NTSTATUS
CommUninitialize(
    _In_ PCOMM_DATA Data
)
{
    if (!Data->CommDevice)
    {
        return STATUS_SUCCESS;
    }

    NTSTATUS status = CommStop(Data);
    NT_ASSERT(status);

    WdfObjectDelete(Data->CommDevice);
    Data->CommDevice = NULL;

    return STATUS_SUCCESS;
}
