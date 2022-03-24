#include "KernelThreadPoolSpecification.h"

THREADPOOL gThreadpool;
#define TP_TEST_TAG 'STPT'
 

void
TestMethod(
    PVOID StartContext
)
{

    LARGE_INTEGER timeout;
    timeout.QuadPart = -10 * 1000 * 1000;

    DWORD* workIndex = (DWORD*)StartContext;

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Processed test number %d on thread id %X\n", *workIndex, PsGetCurrentThreadId());
    ExFreePoolWithTag(
        StartContext,
        TP_TEST_TAG
    );
    KeDelayExecutionThread(KernelMode, FALSE, &timeout);

    return;
}

void
TestThreadpool(
    DWORD stressLevel
)
{
    for (DWORD i = 0; i < stressLevel; i++)
    {
        NTSTATUS status;

        DWORD *workIndex = (DWORD*)ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(DWORD),
            TP_TEST_TAG
        );

        *workIndex = i;

        status = ThreadpoolAddWork(
            &gThreadpool,
            TestMethod,
            workIndex
        );
        if (!NT_SUCCESS(status))
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "ThreadpoolAddWork exited with status %d\n", status);
        }
    }
}

void
Unload(
    PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    ThreadpoolUninit(&gThreadpool);
}

NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
    DriverObject->DriverUnload = Unload;

    if(NT_SUCCESS(ThreadpoolInit(&gThreadpool, 10)))
        TestThreadpool(1000);


    return STATUS_SUCCESS;
}