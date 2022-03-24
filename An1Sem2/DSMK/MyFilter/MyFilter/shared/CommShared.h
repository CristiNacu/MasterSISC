#ifndef _COMM_SHARED_HPP_INCLUDED_
#define _COMM_SHARED_HPP_INCLUDED_

#ifdef KERNEL_MODE
#include <fltKernel.h>
#else
#include <fltUser.h>
#include "globaldata.h"
#endif

#define MY_FILTER_PORT_NAME L"\\MY_DRIVERCommPort"
#define PROCESS_QUERY_LIMITED_ATTRIBUTES 0x1000

#define BIT(x)                      (1 << x)

#define PROCESS_NOTIFICATION_BIT    0
#define THREAD_NOTIFICATION_BIT     1
#define IMAGE_NOTIFICATION_BIT      2
#define REGISTRY_NOTIFICATION_BIT   3
#define FILE_NOTIFICATION_BIT       4
#define FAIL_ALL_IF_ERROR_BIT       31

#define PROCESS_NOTIFICATION        BIT(PROCESS_NOTIFICATION_BIT)
#define THREAD_NOTIFICATION         BIT(THREAD_NOTIFICATION_BIT)
#define IMAGE_NOTIFICATION          BIT(IMAGE_NOTIFICATION_BIT)
#define REGISTRY_NOTIFICATION       BIT(REGISTRY_NOTIFICATION_BIT)
#define FILE_NOTIFICATION           BIT(FILE_NOTIFICATION_BIT)
#define FAIL_ALL_IF_ERROR           BIT(FAIL_ALL_IF_ERROR_BIT)


/// Event Handling Functions

#define REGISTRY_COMMUNICATION_POOL_TAG 'TPGR'


typedef struct _REGISTRY_CREATE_EVENT {
    LARGE_INTEGER   Timestamp;
    UINT32          ProcessId;

    USHORT          ProcessPathSize;
    USHORT          KeyNameSize;

    UCHAR           Data[0];
} REGISTRY_CREATE_EVENT;

typedef struct _REGISTRY_SET_VALUE_EVENT {
    LARGE_INTEGER   Timestamp;
    UINT32          ProcessId;

    USHORT          ProcessPathSize;
    USHORT          KeyNameSize;
    ULONG           ValueSize;

    UCHAR           Data[0];
} REGISTRY_SET_VALUE_EVENT;

typedef struct _REGISTRY_DELETE_KEY_EVENT {
    LARGE_INTEGER   Timestamp;
    UINT32          ProcessId;

    USHORT          ProcessPathSize;
    USHORT          KeyNameSize;

    UCHAR           Data[0];
} REGISTRY_DELETE_KEY_EVENT;

typedef struct _REGISTRY_DELETE_VALUE_EVENT {
    LARGE_INTEGER   Timestamp;
    UINT32          ProcessId;

    USHORT          ProcessPathSize;
    USHORT          KeyNameSize;
    USHORT          ValueSize;
    
    UCHAR           Data[0];
} REGISTRY_DELETE_VALUE_EVENT;

typedef struct _REGISTRY_LOAD_KEY_EVENT {
    LARGE_INTEGER   Timestamp;
    UINT32          ProcessId;

    USHORT          ProcessPathSize;
    USHORT          RootKeySize;

    UCHAR           Data[0];
} REGISTRY_LOAD_KEY_EVENT;

typedef struct _REGISTRY_RENAME_KEY_EVENT {
    LARGE_INTEGER   Timestamp;
    UINT32          ProcessId;

    USHORT          ProcessPathSize;
    USHORT          OldKeyNameSize;
    USHORT          NewKeyNameSize;

    UCHAR           Data[0];
} REGISTRY_RENAME_KEY_EVENT;


//
// FLT_PORT_CONNECTION_CONTEXT - connection context used by MY_DRIVER
//
typedef struct _FLT_PORT_CONNECTION_CONTEXT
{
    ULONG Version;
}FLT_PORT_CONNECTION_CONTEXT, *PFLT_PORT_CONNECTION_CONTEXT;


#pragma region Commands
/*++
This region contains definitions for all the commands that can be received by MY_DRIVER driver form MY_DRIVERCORE trough the FLT port
--*/

//
// MY_DRIVER_COMMAND_CODE
//
typedef enum _MY_DRIVER_COMMAND_CODE
{
    commGetVersion = 1,
    commStartMonitoring = 2,
    commStopMonitoring = 3,
    /// When adding new new commands please change _Field_range_ for MY_DRIVER_COMMAND_HEADER
}MY_DRIVER_COMMAND_CODE, *PMY_DRIVER_COMMAND_CODE;

#pragma pack (push, 1)
//
// MY_DRIVER_COMMAND_HEADER
//
typedef struct _MY_DRIVER_COMMAND_HEADER
{
    _Field_range_(commGetVersion, commStopMonitoring) UINT32 CommandCode;
}MY_DRIVER_COMMAND_HEADER, *PMY_DRIVER_COMMAND_HEADER;

/// All commands must start with the MY_DRIVER_COMMAND_HEADER structure

//
// COMM_CMD_GET_VERSION
//
typedef struct _COMM_CMD_GET_VERSION
{
    MY_DRIVER_COMMAND_HEADER Header;
    ULONG Version;
}COMM_CMD_GET_VERSION, *PCOMM_CMD_GET_VERSION;

//
// COMM_CMD_START_MONITORING
//
typedef struct _COMM_CMD_START_MONITORING
{
    MY_DRIVER_COMMAND_HEADER Header;
    UINT32 EnableOptions;
}COMM_CMD_START_MONITORING, *PCOMM_CMD_START_MONITORING;

//
// COMM_CMD_STOP_MONITORING
//
typedef struct _COMM_CMD_STOP_MONITORING
{
    MY_DRIVER_COMMAND_HEADER Header;
    UINT32 DisableOptions;
}COMM_CMD_STOP_MONITORING, *PCOMM_CMD_STOP_MONITORING;

#pragma pack (pop)
#pragma endregion Commands

#pragma region Messages
/*++
   This region contains definitions for all the messages that can be received by MY_DRIVERCORE from the MY_DRIVER driver
--*/
typedef enum _MY_DRIVER_MESSAGE_CODE
{
    msgSendString = 0,

    msgProcessCreate = 1,
    msgProcessTerminate = 2,

    msgThreadCreate = 3,
    msgThreadTerminate,

    msgImageLoad,

    msgRegistryCreate,
    msgRegistrySetValue,
    msgRegistryDeleteKey,
    msgRegistryDeleteValue,
    msgRegistryLoadkey,
    msgRegistryRenameKey,

    msgFileOperationStartDoNotUse,
    msgFileCreate,
    msgFileClose,
    msgFileCleanup,
    msgFileRead,
    msgFileWrite,
    msgFileSetAttributes,
    msgFileOperationEndDoNotUse,
    msgMaxValue,
} MY_DRIVER_MESSAGE_CODE, *PMY_DRIVER_MESSAGE_CODE;

/// All messages must start with FILTER_MESSAGE_HEADER
/// All replies must start with  FILTER_REPLY_HEADER

#pragma pack(push, 1)
//
// MY_DRIVER_MESSAGE_HEADER
//
typedef struct _MY_DRIVER_MESSAGE_HEADER
{
     MY_DRIVER_MESSAGE_CODE MessageCode;
} MY_DRIVER_MESSAGE_HEADER, *PMY_DRIVER_MESSAGE_HEADER;

//
// MY_DRIVER_MSG_PROCESS_NOTIFICATION
//
typedef struct _MY_DRIVER_MSG_PROCESS_NOTIFICATION
{
    MY_DRIVER_MESSAGE_HEADER Header;
    UINT32 ProcessId;           // the process id
    USHORT ImagePathLength;     // size in bytes of the path
    USHORT CommandLineLength;
    UCHAR  Data[0];
} MY_DRIVER_MSG_PROCESS_NOTIFICATION, *PMY_DRIVER_MSG_PROCESS_NOTIFICATION;

//
// MY_DRIVER_MSG_THREAD_NOTIFICATION
//
typedef struct _MY_DRIVER_MSG_THREAD_NOTIFICATION
{
    MY_DRIVER_MESSAGE_HEADER Header;
    LARGE_INTEGER   Timestamp;
    UINT32          ProcessId;           // the process id
    UINT32          ThreadId;           // the process id
    USHORT          ImagePathLength;     // size in bytes of the path
    UCHAR           Data[0];
} MY_DRIVER_MSG_THREAD_NOTIFICATION, * PMY_DRIVER_MSG_THREAD_NOTIFICATION;

//
// MY_DRIVER_MSG_IMAGE_NOTIFICATION
//
typedef struct _MY_DRIVER_MSG_IMAGE_NOTIFICATION
{
    MY_DRIVER_MESSAGE_HEADER Header;
    UINT32          ProcessId;           // the process id
    LARGE_INTEGER   Timestamp;
    USHORT          ImagePathLength;     // size in bytes of the path
    UCHAR            Data[0];
} MY_DRIVER_MSG_IMAGE_NOTIFICATION, * PMY_DRIVER_MSG_IMAGE_NOTIFICATION;

//
// MY_DRIVER_MSG_REGISTRY_NOTIFICATION
//
typedef struct _MY_DRIVER_MSG_REGISTRY_NOTIFICATION
{
    MY_DRIVER_MESSAGE_HEADER Header;
    
    union {
        
        REGISTRY_CREATE_EVENT       RegistryCreateEvent;
        REGISTRY_SET_VALUE_EVENT    RegistrySetValueEvent;
        REGISTRY_DELETE_KEY_EVENT   RegistryDeleteKeyEvent;
        REGISTRY_DELETE_VALUE_EVENT RegistryDeleteValueEvent;
        REGISTRY_LOAD_KEY_EVENT     RegistryLoadKeyEvent;
        REGISTRY_RENAME_KEY_EVENT   RegistryRenameKeyEvent;

    } MessagePayload;

} MY_DRIVER_MSG_REGISTRY_NOTIFICATION, * PMY_DRIVER_MSG_REGISTRY_NOTIFICATION;

//
// MY_DRIVER_MSG_FILE_NOTIFICATION
//
typedef struct _MY_DRIVER_MSG_FILE_NOTIFICATION
{
    MY_DRIVER_MESSAGE_HEADER    Header;
    LARGE_INTEGER               Timestamp;
    UINT32                      ProcessId;           // the process id
    USHORT                      ImagePathLength;     // size in bytes of the path
    USHORT                      FilePathLength;
    UCHAR                       Data[0];
} MY_DRIVER_MSG_FILE_NOTIFICATION, * PMY_DRIVER_MSG_FILE_NOTIFICATION;


//
// MY_DRIVER_MSG_STRING_NOTIFICATION
//
typedef struct _MY_DRIVER_MSG_STRING_NOTIFICATION
{
    MY_DRIVER_MESSAGE_HEADER Header;
    USHORT Length;     // size in bytes of the path
    UCHAR  Data[0];
} MY_DRIVER_MSG_STRING_NOTIFICATION, *PMY_DRIVER_MSG_STRING_NOTIFICATION;

//
// MY_DRIVER_PROCESS_CREATE_MESSAGE_REPLY
//
typedef struct _MY_DRIVER_PROCESS_CREATE_MESSAGE_REPLY
{
    NTSTATUS Status;
} MY_DRIVER_PROCESS_CREATE_MESSAGE_REPLY, *PMY_DRIVER_PROCESS_CREATE_MESSAGE_REPLY;

//
// MY_DRIVER_PROCESS_CREATE_MESSAGE_REPLY
//
typedef struct _MY_DRIVER_SEND_STRING_REPLY
{
    NTSTATUS Status;
} MY_DRIVER_SEND_STRING_REPLY, * PMY_DRIVER_SEND_STRING_REPLY;


//
// MY_DRIVER_PROCESS_NOTIFICATION_FULL_MESSAGE
//
typedef struct _MY_DRIVER_PROCESS_NOTIFICATION_FULL_MESSAGE
{
    FILTER_MESSAGE_HEADER Header;
    MY_DRIVER_MSG_PROCESS_NOTIFICATION Message;
} MY_DRIVER_PROCESS_NOTIFICATION_FULL_MESSAGE, *PMY_DRIVER_PROCESS_NOTIFICATION_FULL_MESSAGE;

//
// MY_DRIVER_PROCESS_CREATE_FULL_MESSAGE_REPLY
//
typedef struct _MY_DRIVER_PROCESS_CREATE_FULL_MESSAGE_REPLY
{
    FILTER_REPLY_HEADER Header;
    MY_DRIVER_PROCESS_CREATE_MESSAGE_REPLY Reply;
}MY_DRIVER_PROCESS_CREATE_FULL_MESSAGE_REPLY, *PMY_DRIVER_PROCESS_CREATE_FULL_MESSAGE_REPLY;

typedef struct _MY_DRIVER_PROCESS_TERMINATE_MSG
{
    MY_DRIVER_MESSAGE_HEADER Header;
    ULONG32 ProcessId;
}MY_DRIVER_PROCESS_TERMINATE_MSG, *PMY_DRIVER_PROCESS_TERMINATE_MSG;

typedef struct _MY_DRIVER_PROCESS_TERMINATE_MSG_FULL
{
    FILTER_MESSAGE_HEADER Header;
    MY_DRIVER_PROCESS_TERMINATE_MSG Msg;
}MY_DRIVER_PROCESS_TERMINATE_MSG_FULL, *PMY_DRIVER_PROCESS_TERMINATE_MSG_FULL;


#pragma pack(pop)
#pragma endregion Messages


#endif