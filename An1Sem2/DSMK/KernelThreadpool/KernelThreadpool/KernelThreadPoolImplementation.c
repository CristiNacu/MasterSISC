#include "KernelThreadPoolSpecification.h"

#define THREADPOOL_TAG 'LPHT'
NTSTATUS
_WorkerRoutine(
    PVOID WorkerThreadData
);

NTSTATUS
ThreadpoolInit(
    THREADPOOL *Threadpool, 
    DWORD NoThreads
)
{
    if (!Threadpool)
        return STATUS_INVALID_PARAMETER_1;

    if (!NoThreads)
        return STATUS_INVALID_PARAMETER_2;

    InitializeListHead(&Threadpool->ListHead);

    KeInitializeSpinLock(&Threadpool->ListtSpinlock);

    //KeInitializeMutex(&Threadpool->ListMutex, 0);

    KeInitializeEvent(&Threadpool->Stop, NotificationEvent, FALSE);
    KeInitializeEvent(&Threadpool->WorkAvailable, SynchronizationEvent, FALSE);

    Threadpool->Threads = 
        (HANDLE *)ExAllocatePoolWithTag(
            NonPagedPool, 
            NoThreads * sizeof(HANDLE), 
            THREADPOOL_TAG
        );

    if (!Threadpool->Threads)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(
        Threadpool->Threads, 
        NoThreads * sizeof(HANDLE)
    );

    Threadpool->NoThreads = 0;

    for (DWORD i = 0; i < NoThreads; i++)
    {
        NTSTATUS status = PsCreateSystemThread(
            &Threadpool->Threads[i],
            GENERIC_ALL,
            NULL,
            NULL,
            NULL,
            _WorkerRoutine,
            Threadpool
        );
        if (!NT_SUCCESS(status))
        {
            __debugbreak();
            ThreadpoolUninit(Threadpool);
            return status;
        }
        Threadpool->NoThreads++;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
ThreadpoolUninit(
    THREADPOOL *Threadpool
)
{
    KeSetEvent(&Threadpool->Stop, 0, FALSE);

    for (DWORD i = 0; i < Threadpool->NoThreads; i++)
    {
        if (Threadpool->Threads[i] != NULL)
        {
            ZwWaitForSingleObject(Threadpool->Threads[i], FALSE, NULL);
            ZwClose(Threadpool->Threads[i]);
            Threadpool->Threads[i] = NULL;
        }
    }
    if (Threadpool->Threads)
        ExFreePoolWithTag(
            Threadpool->Threads, 
            THREADPOOL_TAG
        );

    _No_competing_thread_begin_
    while (!IsListEmpty(&Threadpool->ListHead))
    {
        LIST_ENTRY* entry = RemoveHeadList(&Threadpool->ListHead);
        WORK_QUANTA* workItem = CONTAINING_RECORD(entry, WORK_QUANTA, ListEntry);

        workItem->WorkRoutine( 
            workItem->Context
        );

        ExFreePoolWithTag(
            workItem, 
            THREADPOOL_TAG
        );
    }
    _No_competing_thread_end_

    return STATUS_SUCCESS;
}

NTSTATUS
ThreadpoolAddWork(
    THREADPOOL* Threadpool,
    KSTART_ROUTINE* WorkRoutine,
    PVOID Context
)
{
    KIRQL oldIrql;
    WORK_QUANTA* workItem = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(WORK_QUANTA),
        THREADPOOL_TAG
    );
    if (!workItem)
        return STATUS_INSUFFICIENT_RESOURCES;

    workItem->WorkRoutine = WorkRoutine;
    workItem->Context = Context;


    ExAcquireSpinLock(&Threadpool->ListtSpinlock, &oldIrql);
    InsertHeadList(&Threadpool->ListHead, &workItem->ListEntry);
    ExReleaseSpinLock(&Threadpool->ListtSpinlock, oldIrql);

    KeSetEvent(&Threadpool->WorkAvailable, 0, FALSE);
    return STATUS_SUCCESS;
}

NTSTATUS
_WorkerRoutine(
    PVOID WorkerThreadData
)
{
    THREADPOOL* threadpool = (THREADPOOL*)WorkerThreadData;
    BOOLEAN shouldWork = TRUE;
    PVOID waitHandles[2];
    
    waitHandles[0] = &threadpool->Stop;
    waitHandles[1] = &threadpool->WorkAvailable;

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "thread id %X entered threadpool\n", PsGetCurrentThreadId());

    while (shouldWork)
    {
        NTSTATUS status = KeWaitForMultipleObjects(
            2,
            waitHandles,
            WaitAny,
            Executive,
            KernelMode,
            FALSE,
            NULL,
            NULL
        );

        if (status == STATUS_WAIT_1)
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Thread id %X woke\n", PsGetCurrentThreadId());

            BOOLEAN hasWork = TRUE;
            while (hasWork)
            {
                KIRQL oldIrql;

                ExAcquireSpinLock(
                    &threadpool->ListtSpinlock,
                    &oldIrql
                );

                if (!IsListEmpty(&threadpool->ListHead))
                {
                    LIST_ENTRY *entry = RemoveTailList(&threadpool->ListHead);
                    WORK_QUANTA* workQuanta = (WORK_QUANTA*)entry;

                    if (!IsListEmpty(&threadpool->ListHead))
                    {
                        KeSetEvent(&threadpool->WorkAvailable, 0, FALSE);
                    }

                    if (workQuanta->WorkRoutine == NULL)
                    {
                        // Da un mesaj de eroare
                        goto unsuccesfulWork;
                    }

                    ExReleaseSpinLock(
                        &threadpool->ListtSpinlock,
                        oldIrql
                    );

                    workQuanta->WorkRoutine(workQuanta->Context);
                    ExFreePoolWithTag(
                        workQuanta,
                        THREADPOOL_TAG
                    );
                    
                }
                else
                {
                    KeResetEvent(&threadpool->WorkAvailable);
                    hasWork = FALSE;
                }

                continue;
            unsuccesfulWork:

            ExReleaseSpinLock(
                &threadpool->ListtSpinlock,
                oldIrql
            );

            }
        }
        else
        {
            shouldWork = FALSE;
            continue;
        }
    }

    return STATUS_SUCCESS;
}