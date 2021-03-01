#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_ 

#include "includes.h"

/// This file contains the basic definition for the thread pool.
/// You must not modify THREAD_POOL and THREAD_POOL_WORK_ITEM structures or the APIs.
/// You are required to implement the APIs (in ThreadPool.c).
///
/// At an high level overview, a ThreadPool maintains a list of multiple threads waiting for tasks.
/// The main benefit of using a thread pool is that creation and destruction of threads is an
///    expensive operation and this overhead will be restricted to the creation of the ThreadPool.
/// 
/// Your task is to implement a generic ThreadPool.
/// This can be achieved by providing a generic CallbackRoutine of type LPTHREAD_START_ROUTINE
///    which takes a generic argument PVOID Context which is caller defined.
/// ThreadPool might start Uninitializing when the work items is not empty, thus we need a routine
///    to be called to ensure the resources are properly freed. This is the NotProcessedRoutine.
/// Storing the work items is usually done using a synchronized queue. You are required to implement
///    a queue using Single Linked Lists APIs which provides atomic operations such as
///    InterlockedPushEntrySList and InterlockedPopEntrySList, an example can be found below
///    https://docs.microsoft.com/en-us/windows/win32/sync/using-singly-linked-lists
/// Regarding the number of threads, for simplicity, we are working with a fixed number of 4 threads.
///    An example of how to create, wait and destroy threads is available below
///    https://docs.microsoft.com/en-us/windows/win32/procthread/creating-threads
/// For performance reasons, the threads will sleep when no work item is available. To signal that
///    a work item was enqueued and must be processed, you can use WIN32 Events
///    An example of how to use win32 events is available below
///    https://docs.microsoft.com/en-us/windows/win32/sync/using-event-objects
/// Another event will be used to signal the threads that ThreadPool will be uninitialized is TerminationEvent.
///    Threads can wait for both events using WaitForMultipleObjects API 
///    An example is provided here: https://docs.microsoft.com/en-us/windows/win32/sync/waiting-for-multiple-objects
///
/// Be careful of the events modes (auto-reset / manual-reset) and the initial state (signaled / not signaled).
/// You can use NTSTATUS_FROM_WIN32 macro to convert the value returned from GetLastError() to a proper NTSTATUS

#pragma pack(push, 16)
typedef struct _THREAD_POOL_WORK_ITEM
{
    SLIST_ENTRY Entry;
    PVOID Context;
    LPTHREAD_START_ROUTINE CallbackRoutine;
    LPTHREAD_START_ROUTINE NotProcessedRoutine;
} THREAD_POOL_WORK_ITEM, *PTHREAD_POOL_WORK_ITEM;

typedef struct _THREAD_POOL
{
    SLIST_HEADER Queue;
    HANDLE ItemEnqueuedEvent;
    HANDLE TerminationEvent;
    HANDLE Threads[4];
}THREAD_POOL, *PTHREAD_POOL;
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