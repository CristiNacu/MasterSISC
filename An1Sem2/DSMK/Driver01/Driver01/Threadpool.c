#include "ThreadPool.h"

#define MAX_THREAD_WAIT_TIME 5000 //ms
#define THREAD_FORCE_CLOSE_STATUS 0xDEAD

#define EXIT_EVENT_INDEX 0
#define WORK_AVAILABLE_EVENT_INDEX 1

#define STATUS_QUEUE_EMPTY              0xE85678
#define STATUS_THREAD_WORK_FAILED       0xE85679

_Use_decl_annotations_
NTSTATUS
WINAPI
_ThreadpoolExecuteWork(
    _In_ PSLIST_HEADER Queue,
    _In_ BOOL Terminate
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PTHREAD_POOL_WORK_ITEM workItem = (PTHREAD_POOL_WORK_ITEM)InterlockedPopEntrySList(Queue);
    DWORD workStatus = 0;
    if (workItem == NULL)
    {
        return STATUS_QUEUE_EMPTY;
    }
    else
    {
        if (Terminate)
        {
            workStatus = workItem->NotProcessedRoutine(workItem->Context);
        }
        else
        {
            workStatus = workItem->CallbackRoutine(workItem->Context);
        }
        if (workStatus != 0)
        {
            printf("Method calling failed with status %d\n", workStatus);
            status = STATUS_THREAD_WORK_FAILED;
        }
    }

    _aligned_free(workItem);

    return status;
}

_Use_decl_annotations_
DWORD
WINAPI
_ThreadpoolSpinMethod(
    _In_ LPVOID lpParam
)
{
    PTHREAD_POOL threadPool = (PTHREAD_POOL)lpParam;
    NTSTATUS status = 0;
    DWORD dwWaitResult;
    HANDLE events[2];
    BOOL spin = TRUE;

    events[EXIT_EVENT_INDEX] = threadPool->TerminationEvent;
    events[WORK_AVAILABLE_EVENT_INDEX] = threadPool->ItemEnqueuedEvent;

    while (spin)
    {
        dwWaitResult = WaitForMultipleObjects(
            2,
            events,
            FALSE,
            INFINITE);

        if (dwWaitResult == WAIT_OBJECT_0 + EXIT_EVENT_INDEX)  // Exit
        {
            status = _ThreadpoolExecuteWork(
                &threadPool->Queue,
                TRUE
            );

            spin = FALSE;
        }
        else if (dwWaitResult == (WAIT_OBJECT_0 + WORK_AVAILABLE_EVENT_INDEX)) // Work available
        {
            status = _ThreadpoolExecuteWork(
                &threadPool->Queue,
                FALSE
            );

            if (status == STATUS_QUEUE_EMPTY)
                ResetEvent(threadPool->ItemEnqueuedEvent);
        }
        else
        {
            printf("Wait for multiple objects returned status %d!\n", GetLastError());
        }
    }

    return 0;
}

_Use_decl_annotations_
NTSTATUS
ThreadPoolInit(
    _Pre_invalid_ _Post_valid_ PTHREAD_POOL ThreadPool
)
{
    NTSTATUS status;
    if (!ThreadPool)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    InitializeSListHead(
        &(ThreadPool->Queue)
    );

    ThreadPool->ItemEnqueuedEvent = CreateEvent(
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("ElementsAvailableInQueue")  // object name
    );
    if (ThreadPool->ItemEnqueuedEvent == INVALID_HANDLE_VALUE)
    {
        printf("CreateEvent failed (%d)\n", GetLastError());
        status = STATUS_ABANDONED;
        goto cleanup;
    }

    ThreadPool->TerminationEvent = CreateEvent(
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("TerminateThreadpool")  // object name
    );
    if (ThreadPool->TerminationEvent == INVALID_HANDLE_VALUE)
    {
        printf("CreateEvent failed (%d)\n", GetLastError());
        status = STATUS_ABANDONED;
        goto cleanup;
    }

    for (int i = 0; i < 4; i++)
    {
        ThreadPool->Threads[i] = CreateThread(
            NULL,                       // default security attributes
            0,                          // use default stack size  
            _ThreadpoolSpinMethod,       // thread function name
            ThreadPool,          // argument to thread function 
            0,                      // use default creation flags 
            NULL);   // returns the thread identifier 
        if (ThreadPool->Threads[i] == NULL)
        {
            printf("CreateThread failed (%d)\n", GetLastError());
            status = STATUS_ABANDONED;
            goto cleanup;
        }

    }

    return STATUS_SUCCESS;

cleanup:

    ThreadPoolUninit(
        ThreadPool
    );

    return status;
}

_Use_decl_annotations_
VOID
ThreadPoolUninit(
    _Pre_valid_ _Post_invalid_ PTHREAD_POOL ThreadPool
)
{
    if (ThreadPool->TerminationEvent)
    {
        SetEvent(ThreadPool->TerminationEvent);
    }

    for (int i = 0; i < 4; i++)
    {
        if (ThreadPool->Threads[i])
        {
            DWORD threadStatus;
            threadStatus = WaitForSingleObject(
                ThreadPool->Threads[i],
                MAX_THREAD_WAIT_TIME
            );
            if (threadStatus != WAIT_OBJECT_0)
            {
                if (threadStatus == WAIT_FAILED)
                    printf("Wait for thread failed with error: (%d)!\n", GetLastError());
                else if (threadStatus == WAIT_ABANDONED)
                    printf("Wait abandoned for thread!\n");

                if (!TerminateThread(
                    ThreadPool->Threads[i],
                    THREAD_FORCE_CLOSE_STATUS
                ))
                {
                    printf("Could not close thread! (%d)\n", GetLastError());
                }
            }
        }
    }

    if (ThreadPool->ItemEnqueuedEvent)
        CloseHandle(ThreadPool->ItemEnqueuedEvent);

    if (ThreadPool->TerminationEvent)
        CloseHandle(ThreadPool->TerminationEvent);
    return;
}

_Use_decl_annotations_
NTSTATUS
ThreadPoolEnqueue(
    _Inout_ _Pre_valid_ _Post_valid_ PTHREAD_POOL ThreadPool,
    _In_ LPTHREAD_START_ROUTINE ThreadRoutine,
    _In_ LPTHREAD_START_ROUTINE NotProcessedRoutine,
    _In_opt_ PVOID Context
)
{
    PTHREAD_POOL_WORK_ITEM workItem;

    if (!ThreadPool)
    {
        return STATUS_INVALID_PARAMETER_1;
    }
    if (!ThreadRoutine)
    {
        return STATUS_INVALID_PARAMETER_2;
    }
    if (!NotProcessedRoutine)
    {
        return STATUS_INVALID_PARAMETER_3;
    }


    workItem = (PTHREAD_POOL_WORK_ITEM)_aligned_malloc(
        sizeof(THREAD_POOL_WORK_ITEM),
        MEMORY_ALLOCATION_ALIGNMENT
    );
    if (!workItem)
    {
        printf("[ERROR] Could not allocate memory for work item!\n");
        return STATUS_MEMORY_NOT_ALLOCATED;
    }
    workItem->NotProcessedRoutine = NotProcessedRoutine;
    workItem->CallbackRoutine = ThreadRoutine;
    workItem->Context = Context;

    InterlockedPushEntrySList(
        &ThreadPool->Queue,
        (PSLIST_ENTRY)workItem
    );

    if (!SetEvent(ThreadPool->ItemEnqueuedEvent))
    {
        printf("[WARNING] Could not set enqueued event!\n");
    }

    return STATUS_SUCCESS;
}