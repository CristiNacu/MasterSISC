#include "Globals.h"
#include "ThreadPool.h"

GLOBAL_DATA gGlobalData;


_Use_decl_annotations_
NTSTATUS
GlobalDataInitialize()
{
    NTSTATUS status;

    gGlobalData.UserPath = NULL;
    gGlobalData.UserAuthenticated = FALSE;

    status = ThreadPoolInit(
        &gGlobalData.FileTransferThreadPool
    );
    if (!NT_SUCCESS(status))
    {
        printf("ThreadPoolInit exited with status %x\n", status);
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
VOID
GlobalDataUninitialize()
{
    gGlobalData.UserPath = NULL;
    gGlobalData.UserAuthenticated = FALSE;
    
    ThreadPoolUninit(
        &gGlobalData.FileTransferThreadPool
    );

    return;
}