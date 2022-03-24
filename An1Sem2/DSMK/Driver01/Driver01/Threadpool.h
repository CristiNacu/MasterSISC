#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_ 

#include "Includes.h"


#pragma pack(push, 16)
typedef struct _THREAD_POOL_WORK_ITEM
{
    SLIST_ENTRY Entry;
    PVOID Context;
    LPTHREAD_START_ROUTINE CallbackRoutine;
    LPTHREAD_START_ROUTINE NotProcessedRoutine;
} THREAD_POOL_WORK_ITEM, * PTHREAD_POOL_WORK_ITEM;

typedef struct _THREAD_POOL
{
    SLIST_HEADER Queue;
    HANDLE ItemEnqueuedEvent;
    HANDLE TerminationEvent;
    HANDLE Threads[4];
}THREAD_POOL, * PTHREAD_POOL;
#pragma pack(pop)


// This method will initialize a THREAD_POOL structure:
//    * It properly initializes the singly linked list member "Queue" which is used to store work items
//    * It creates the "ItemEnqueuedEvent" which will be used to signal the threads when a work item is available
//    * It creates the "TerminationEvent" which will be used to signal whether the ThreadPool is Uninitializing
//    * Creates 4 threads which automatically are going to wait for one of the events.
// This method returns STATUS_SUCCESS if everything succeeded or a proper error status otherwise.
_No_competing_thread_
_Must_inspect_result_
NTSTATUS
ThreadPoolInit(
    _Pre_invalid_ _Post_valid_ PTHREAD_POOL ThreadPool
);

// This method will uninitialize a THREAD_POOL structure and cleanup the memory
//    * It sets the TerminationEvent
//    * Wait for all 4 threads to finish processing and close the handles
//    * Empty the work items queue and perform proper cleanup of the allocated resources by
//      calling the NotProcessedRoutine for each item left in queue.
//    * Destroy the events
_No_competing_thread_
VOID
ThreadPoolUninit(
    _Pre_valid_ _Post_invalid_ PTHREAD_POOL ThreadPool
);

// This method will enqueue a new work item:
//    * It will allocate a new THREAD_POOL_WORK_ITEM structure to be associated with the work item
//    * It will insert the work item in Queue and signal the ItemEnqueuedEvent
// This method returns STATUS_SUCCESS if everything succeeded or a proper error status otherwise.
_Must_inspect_result_
NTSTATUS
ThreadPoolEnqueue(
    _Inout_ _Pre_valid_ _Post_valid_ PTHREAD_POOL ThreadPool,
    _In_ LPTHREAD_START_ROUTINE ThreadRoutine,
    _In_ LPTHREAD_START_ROUTINE NotProcessedRoutine,
    _In_opt_ PVOID Context
);

#endif //_THREAD_POOL_H_