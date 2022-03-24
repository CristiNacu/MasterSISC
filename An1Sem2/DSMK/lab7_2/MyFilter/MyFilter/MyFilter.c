
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "Trace.h"
#include "MyFilter.tmh"
#include "MyDriver.h"
#include "Communication.h"

#include "CommShared.h"
#include "ProcessFilter.h"
#include "FileFilterCallbacks.h"
#include "ImageFilter.h"
#include "RegistryFilter.h"
#include "ThreadFilter.h"
#include <windef.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

GLOBLA_DATA gDrv;

NTSTATUS
GetProcessName(
    HANDLE Pid,
    PUNICODE_STRING* ProcessName
)
{
    NTSTATUS status;
    HANDLE hProcess;
    ULONG dwObjectNameSize = 0;
    PUNICODE_STRING pProcessPath = NULL;
    OBJECT_ATTRIBUTES objectAttributes = { 0 };
    CLIENT_ID clientId = { 0 };

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        OBJ_KERNEL_HANDLE,
        NULL, NULL
    );

    clientId.UniqueProcess = Pid;
    clientId.UniqueThread = 0;

    status = ZwOpenProcess(
        &hProcess,
        PROCESS_QUERY_LIMITED_ATTRIBUTES,
        &objectAttributes,
        &clientId);
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    // Query process path size
    status = pfnZwQueryInformationProcess(
        hProcess,
        ProcessImageFileName,
        NULL,
        dwObjectNameSize,
        &dwObjectNameSize);
    if (status != STATUS_INFO_LENGTH_MISMATCH)
    {
        goto cleanup;
    }

    // Allocate required space
    pProcessPath = (PUNICODE_STRING)ExAllocatePoolWithTag(NonPagedPool,
        dwObjectNameSize, REGISTRY_COMMUNICATION_POOL_TAG);
    if (!pProcessPath)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    // Actually retrieve the image path
    status = pfnZwQueryInformationProcess(
        hProcess,
        ProcessImageFileName,
        pProcessPath,
        dwObjectNameSize,
        &dwObjectNameSize
    );
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    ZwClose(hProcess);

    *ProcessName = pProcessPath;

    return status;

cleanup:

    if (pProcessPath)
    {
        ExFreePoolWithTag(pProcessPath, REGISTRY_COMMUNICATION_POOL_TAG);
    }

    if (hProcess)
    {
        ZwClose(hProcess);
    }

    return status;
}


NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
MyFilterInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
MyFilterInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
MyFilterInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
MyFilterUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
MyFilterInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, MyFilterUnload)
#pragma alloc_text(PAGE, MyFilterInstanceQueryTeardown)
#pragma alloc_text(PAGE, MyFilterInstanceSetup)
#pragma alloc_text(PAGE, MyFilterInstanceTeardownStart)
#pragma alloc_text(PAGE, MyFilterInstanceTeardownComplete)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
         0,
         FileFilterCallbackCreatePre,
         NULL },

    { IRP_MJ_CLOSE,
         0,
         FileFilterCallbackClosePre,
         NULL },

    { IRP_MJ_CLEANUP,
         0,
         FileFilterCallbackCleanupPre,
         NULL },

     { IRP_MJ_READ,
         0,
         FileFilterCallbackReadPre,
         NULL },

    { IRP_MJ_WRITE,
         0,
         FileFilterCallbackWritePre,
         NULL },

     { IRP_MJ_SET_INFORMATION,
         0,
         FileFilterCallbackSetattribPre,
         NULL },

    { IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    MyFilterUnload,                           //  MiniFilterUnload

    MyFilterInstanceSetup,                    //  InstanceSetup
    MyFilterInstanceQueryTeardown,            //  InstanceQueryTeardown
    MyFilterInstanceTeardownStart,            //  InstanceTeardownStart
    MyFilterInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};


NTSTATUS
MyFilterInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    LogInfo("MyFilter!MyFilterInstanceSetup: Entered\n");

    return STATUS_SUCCESS;
}


NTSTATUS
MyFilterInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    LogInfo("MyFilter!MyFilterInstanceQueryTeardown: Entered\n");

    return STATUS_SUCCESS;
}


VOID
MyFilterInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    LogInfo("MyFilter!MyFilterInstanceTeardownStart: Entered\n");
}


VOID
MyFilterInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    LogInfo("MyFilter!MyFilterInstanceTeardownComplete: Entered\n");
}




/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

    LogInfo("MyFilter!DriverEntry: Entered\n");

    UNICODE_STRING ustrFunctionName = RTL_CONSTANT_STRING(L"ZwQueryInformationProcess");
    pfnZwQueryInformationProcess = (PFUNC_ZwQueryInformationProcess)(SIZE_T)MmGetSystemRoutineAddress(&ustrFunctionName);
    gDrv.DriverObject = DriverObject;

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gDrv.FilterHandle );
    if (!NT_SUCCESS(status))
    {
        LogError("MyFilter!DriverEntry: FltRegisterFilter error!\n");
        goto cleanup;
    }
    LogInfo("MyFilter!DriverEntry: FltRegisterFilter Success\n");

    //
    //  Prepare communication layer
    //
    status = CommInitializeFilterCommunicationPort();
    if (!NT_SUCCESS(status)) {

        LogError("MyFilter!DriverEntry: CommInitializeFilterCommunicationPort error!\n");
        goto cleanup;
    }
    LogInfo("MyFilter!DriverEntry: CommInitializeFilterCommunicationPort Success\n");

    status = ProcFltInitialize();
    if (!NT_SUCCESS(status))
    {
        LogError("MyFilter!DriverEntry: ProcFltInitialize error!\n");
        goto cleanup;
    }
    LogInfo("MyFilter!DriverEntry: ProcFltInitialize Success\n");
    gDrv.ActiveMonitoring &= PROCESS_NOTIFICATION;

    status = ImgFltInitialize();
    if (!NT_SUCCESS(status))
    {
        LogError("MyFilter!DriverEntry: ImgFltInitialize error!\n");
        goto cleanup;
    }
    LogInfo("MyFilter!DriverEntry: ImgFltInitialize Success\n");
    gDrv.ActiveMonitoring &= IMAGE_NOTIFICATION;

    status = RegFltInitialize(DriverObject);
    if (!NT_SUCCESS(status))
    {
        LogError("MyFilter!DriverEntry: RegFltInitialize error!\n");
        goto cleanup;
    }
    LogInfo("MyFilter!DriverEntry: RegFltInitialize Success\n");
    gDrv.ActiveMonitoring &= REGISTRY_NOTIFICATION;

    status = ThreadFltInitialize();
    if (!NT_SUCCESS(status))
    {
        LogError("MyFilter!DriverEntry: ThreadFltInitialize error!\n");
        goto cleanup;
    }
    LogInfo("MyFilter!DriverEntry: ThreadFltInitialize Success\n");
    gDrv.ActiveMonitoring &= THREAD_NOTIFICATION;

    //
    //  Start filtering i/o
    //
    status = FltStartFiltering( gDrv.FilterHandle );
    if (!NT_SUCCESS( status ))
    {
        LogError("MyFilter!DriverEntry: FltStartFiltering error!\n");
        goto cleanup;
    }
    LogInfo("MyFilter!DriverEntry: FltStartFiltering Success\n");
    gDrv.ActiveMonitoring &= FILE_NOTIFICATION;

    return status;

cleanup:
    return MyFilterUnload(0);
}

NTSTATUS
MyFilterUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    LogInfo("MyFilter!MyFilterUnload: Entered\n");

    if (gDrv.ActiveMonitoring & THREAD_NOTIFICATION)
    {
        ThreadFltUninitialize();
    }
    if (gDrv.ActiveMonitoring & REGISTRY_NOTIFICATION)
    {
        RegFltUninitialize();
    }
    if (gDrv.ActiveMonitoring & IMAGE_NOTIFICATION)
    {
        ImgFltUninitialize();
    }
    if (gDrv.ActiveMonitoring & PROCESS_NOTIFICATION)
    {
        ProcFltUninitialize();
    }
    if (gDrv.ActiveMonitoring & FILE_NOTIFICATION)
    {
        FltUnregisterFilter(gDrv.FilterHandle);
    }

    CommUninitializeFilterCommunicationPort();

    return STATUS_SUCCESS;
}