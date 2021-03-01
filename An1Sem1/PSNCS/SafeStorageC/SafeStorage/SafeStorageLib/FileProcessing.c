#include "FileProcessing.h"
#include "Globals.h"
#include "Common.h"

#define     KILO (1024)
#define     MEGA (1024 * KILO)
#define     GIGA (1024 * MEGA)

#define FILE_CHUNK_SIZE (4 * KILO)
#define MAX_WAIT_TIME 5000 //ms

//Cool stuff
#define ROUND_UP_PTR(Ptr,Pow2)  ((void *) ((((ULONG_PTR)(Ptr)) + (Pow2) - 1) & (~(((LONG_PTR)(Pow2)) - 1))))

_Use_decl_annotations_
DWORD
WINAPI
_FileProcessingRoutine(
    _In_ LPVOID lpParam
)
{
    PFILE_COPY_TASK copyTask = (PFILE_COPY_TASK)lpParam;
    BOOL functionStatus;
    DWORD timerStatus;
    PVOID buffer = (PVOID)malloc(copyTask->SizeOfCopy * sizeof(CHAR));

    DWORD bytesRead = 0;

    HANDLE workEventFinished = CreateEventA(
        NULL,
        TRUE,
        FALSE,
        NULL
    );
    if (workEventFinished == INVALID_HANDLE_VALUE)
    {
        printf("Invalid event handle created\n");
        goto cleanup;
    }

    OVERLAPPED overlapped;
    overlapped.Pointer = (PVOID)(size_t)copyTask->Offset;
    overlapped.hEvent = workEventFinished;

    functionStatus = ReadFile(
        copyTask->SourceFileHandle,
        buffer,
        copyTask->SizeOfCopy,
        &bytesRead,
        &overlapped
    );
    if (!functionStatus && (GetLastError()!= ERROR_IO_PENDING))
    {
        printf("ReadFile exited with status %x\n", GetLastError());
        goto cleanup;
    }

    timerStatus = WaitForSingleObject(
        workEventFinished,
        MAX_WAIT_TIME
    );
    if (timerStatus == WAIT_TIMEOUT)
    {
        printf("Read timer expired, aborting file transfer\n");
        goto cleanup;
    }
    if (timerStatus != WAIT_OBJECT_0)
    {
        printf("Timer exited with last error %x\n", GetLastError());
        goto cleanup;
    }
    
    ResetEvent(workEventFinished);

    functionStatus = WriteFile(
        copyTask->DestinationFileHandle,
        buffer,
        copyTask->SizeOfCopy,
        &bytesRead,
        &overlapped
    );
    if (!functionStatus && (GetLastError() != ERROR_IO_PENDING))
    {
        printf("WriteFile exited with status %x\n", GetLastError());
        goto cleanup;
    }
    timerStatus = WaitForSingleObject(
        workEventFinished,
        MAX_WAIT_TIME
    );
    if (timerStatus == WAIT_TIMEOUT)
    {
        printf("Write timer expire, aborting file transfer\n");
        goto cleanup;
    }
    if (timerStatus != WAIT_OBJECT_0)
    {
        printf("Timer exited with last error %x\n", GetLastError());
        goto cleanup;
    }

cleanup:
    free(buffer);
    CloseHandle(workEventFinished);

    timerStatus = WaitForSingleObject(
        copyTask->Mutex,    // handle to mutex
        MAX_WAIT_TIME
    );  // no time-out interval
    if (timerStatus == WAIT_OBJECT_0)
    {
        (*copyTask->NumberOfWorkSplitsThreads)--;

        if (*copyTask->NumberOfWorkSplitsThreads == 0)
        {
            free(copyTask->NumberOfWorkSplitsThreads);
            CloseHandle(copyTask->SourceFileHandle);
            CloseHandle(copyTask->DestinationFileHandle);
            free(copyTask->DestinationFilePath);
            ReleaseMutex(copyTask->Mutex);
            CloseHandle(copyTask->Mutex);
            printf("File copied successfully\n");

        }
        else if (!ReleaseMutex(copyTask->Mutex))
        {
            printf("[ERROR] COULD NOT RELEASE MUTEX!!\n");
        }
    }
    else
    {
        printf("WaitForSingleObject returned error %x and status %x!\n", GetLastError(), timerStatus);
    }

    free(copyTask);

    return 0;

}

_Use_decl_annotations_
DWORD
WINAPI
_FileNotProcessedRoutine(
    _In_ LPVOID lpParam
)
{
    PFILE_COPY_TASK copyTask = (PFILE_COPY_TASK)lpParam;
    DWORD timerStatus;

    timerStatus = WaitForSingleObject(
        copyTask->Mutex,    // handle to mutex
        INFINITE
    );  // no time-out interval
    if (timerStatus == WAIT_OBJECT_0)
    {
        (*copyTask->NumberOfWorkSplitsThreads)--;

        if (*copyTask->NumberOfWorkSplitsThreads == 0)
        {
            free(copyTask->NumberOfWorkSplitsThreads);
            CloseHandle(copyTask->SourceFileHandle);
            CloseHandle(copyTask->DestinationFileHandle);

            printf("Aborting copy for file %s\n", copyTask->DestinationFilePath);
            if (!DeleteFileA(
                copyTask->DestinationFilePath
            ))
            {
                printf("[ERROR] Could not remove partially copied file %s last error %x!\n", copyTask->DestinationFilePath, GetLastError());
            }
            free(copyTask->DestinationFilePath);

            ReleaseMutex(copyTask->Mutex);
            CloseHandle(copyTask->Mutex);
            printf("File copying aborted\n");

        }
        else if (!ReleaseMutex(copyTask->Mutex))
        {
            printf("[ERROR] COULD NOT RELEASE MUTEX!!\n");
        }
    }
    else
    {
        printf("WaitForSingleObject returned error %x and status %x!\n", GetLastError(), timerStatus);
    }

    free(copyTask);


    return 0;

}

_Use_decl_annotations_
NTSTATUS
AddFileProcessingJob(
    _In_ PCHAR Source,
    _In_ PCHAR Destination
)
{
    NTSTATUS status;
    FILE_COPY_TASK copyTaskTemplate;

    DWORD attributes;
    HANDLE sourceFileHandle;
    HANDLE destinationFileHandle;

    LARGE_INTEGER fileSize;
    BOOL getFileSizeStatus;
    
    attributes = GetFileAttributesA(Source);

    destinationFileHandle = CreateFileA(
        Destination,
        (GENERIC_READ | GENERIC_WRITE),
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_FLAG_OVERLAPPED,
        NULL
    );
    if (destinationFileHandle == INVALID_HANDLE_VALUE)
    {
        printf("CreateFileA exited with status %x\n", GetLastError());
        goto cleanup;
    }

    sourceFileHandle = CreateFileA(
        Source,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
    );
    if (sourceFileHandle == INVALID_HANDLE_VALUE)
    {
        printf("CreateFileA exited with status %x\n", GetLastError());
        goto cleanup;
    }
    
    getFileSizeStatus = GetFileSizeEx(
        sourceFileHandle,
        &fileSize
    );
    if (!getFileSizeStatus)
    {
        printf("getFileSizeStatus returned status %d\n", GetLastError());
        goto cleanup;
    }
    if (fileSize.QuadPart == 0)
    {
        printf("Cannot copy 0 bytes files\n");
        goto cleanup;
    }


    
    copyTaskTemplate.SourceFileHandle = sourceFileHandle;
    copyTaskTemplate.DestinationFileHandle = destinationFileHandle;

    copyTaskTemplate.SizeOfCopy = FILE_CHUNK_SIZE;

    DWORD numberOfWorkSplitsThreads = (DWORD)((DWORD)(fileSize.QuadPart / FILE_CHUNK_SIZE) + ((fileSize.QuadPart % FILE_CHUNK_SIZE == 0) ? 0 : 1));

    copyTaskTemplate.NumberOfWorkSplitsThreads = (PDWORD)malloc(sizeof(DWORD));
    *copyTaskTemplate.NumberOfWorkSplitsThreads = numberOfWorkSplitsThreads;

    status = CopyPath(&copyTaskTemplate.DestinationFilePath, Destination);
    if (!NT_SUCCESS(status))
    {
        printf("CopyPath exited with status %x\n", status);
        goto cleanup;
    }
    ///store  c:\Users\Cristi\Desktop\nasasun.jpg poza4
    copyTaskTemplate.Mutex = CreateMutexA(
        NULL,
        FALSE,
        NULL
    );
    printf("Started copying...\n");
    for (DWORD i = 0; i < numberOfWorkSplitsThreads; i++)
    {
        PFILE_COPY_TASK finalCopyTask =
            (PFILE_COPY_TASK)malloc(sizeof(FILE_COPY_TASK));
        memcpy(finalCopyTask, &copyTaskTemplate, sizeof(FILE_COPY_TASK));

        finalCopyTask->Offset = i * FILE_CHUNK_SIZE;
        finalCopyTask->SizeOfCopy = min(FILE_CHUNK_SIZE, (DWORD)(fileSize.QuadPart - (((LONGLONG)(size_t)i) * FILE_CHUNK_SIZE)));

        status = ThreadPoolEnqueue(
            &gGlobalData.FileTransferThreadPool,
            _FileProcessingRoutine,
            _FileNotProcessedRoutine,
            (PVOID)finalCopyTask
        );
    }

    

cleanup:
    return STATUS_SUCCESS;
}