#ifndef _KERNEL_THREADPOOL_H_
#define _KERNEL_THREADPOOL_H_


//#include <wdm.h>
#include <fltKernel.h>
#include <windef.h>

typedef struct _THREADPOOL
{
    KEVENT      Stop;
    KEVENT      WorkAvailable;

    HANDLE*     Threads;
    DWORD       NoThreads;
    
    KSPIN_LOCK  ListtSpinlock;
    /*KMUTEX      ListMutex;*/
    
    LIST_ENTRY  ListHead;

} THREADPOOL;

typedef struct _WORK_QUANTA
{
    LIST_ENTRY      ListEntry;

    KSTART_ROUTINE* WorkRoutine;
    PVOID           Context;

}WORK_QUANTA;


NTSTATUS
ThreadpoolInit(
    THREADPOOL *Threadpool,
    DWORD NoThreads
);

NTSTATUS
ThreadpoolUninit(
    THREADPOOL* Threadpool
);

NTSTATUS
ThreadpoolAddWork(
    THREADPOOL* Threadpool,
    KSTART_ROUTINE* WorkRoutine,
    PVOID Context
);


#endif