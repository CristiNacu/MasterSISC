#include <ntddk.h>
#include <wdf.h>

typedef
NTSTATUS
(*PFUNC_CommClientConnected)(
    _In_ WDFFILEOBJECT WdfFileObject
    );

typedef
NTSTATUS
(*PFUNC_CommClientDisconected)(
    _In_ WDFFILEOBJECT WdfFileObject
    );

typedef
NTSTATUS
(*PFUNC_CommReceiveData) (
    _In_ WDFFILEOBJECT WdfFileObject,
    _In_ PVOID InputBuffer,
    _In_ UINT32 InputBufferLength,
    _Out_opt_ PVOID OutputBuffer,
    _In_opt_ UINT32 OutputBufferLength,
    _Out_    UINT32* BytesReturned
    );


typedef struct _COMM_CONFIGURATION
{
    PWCHAR NativeDeviceName;        // Device name in the form \device\xxxxx
    PWCHAR UserDeviceName;          // Device name in the form \\.\\xxxxx

    PFUNC_CommClientConnected   CommClientConnected;
    PFUNC_CommClientDisconected CommClientDisconnected;
    PFUNC_CommReceiveData       CommReceiveData;
    PFUNC_CommReceiveData       CommInternalDeviceControl;
}COMM_CONFIGURATION, * PCOMM_CONFIGURATION;

typedef struct _COMM_REPLY_EVENT_DATA
{
    UINT64 Sequence;
    KEVENT ReplyAvailable;
    KEVENT ReplyProcessed;
    WDFREQUEST ReplyRequest;
    PVOID InputBuffer;
    SIZE_T InputBufferSize;
    PVOID OutputBuffer;
    SIZE_T OutputBufferSize;
}COMM_REPLY_EVENT_DATA, * PCOMM_REPLY_EVENT_DATA;

#define COMM_QUEUE_MAX_REPLY_DATA_COUNT     64
typedef struct _COMM_DATA
{
    COMM_CONFIGURATION      Configuration;
    WDFDEVICE               CommDevice;
    COMM_REPLY_EVENT_DATA   Replies[COMM_QUEUE_MAX_REPLY_DATA_COUNT];
    volatile LONG           NextFree;
    KEVENT                  StopEvent;
}COMM_DATA, * PCOMM_DATA;

NTSTATUS
InitUmComm(
    _In_  WDFDRIVER Driver,
    _In_  PCOMM_CONFIGURATION Configuration,
    _Out_ PCOMM_DATA Data
);