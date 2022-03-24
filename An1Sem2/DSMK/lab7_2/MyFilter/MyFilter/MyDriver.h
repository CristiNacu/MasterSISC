#ifndef _MY_DRIVER_HPP_INCLUDED_
#define _MY_DRIVER_HPP_INCLUDED_

#include "Communication.h"

typedef
NTSTATUS
(NTAPI* PFUNC_ZwQueryInformationProcess) (
    _In_      HANDLE           ProcessHandle,
    _In_      PROCESSINFOCLASS ProcessInformationClass,
    _Out_     PVOID            ProcessInformation,
    _In_      ULONG            ProcessInformationLength,
    _Out_opt_ PULONG           ReturnLength
    );

PFUNC_ZwQueryInformationProcess pfnZwQueryInformationProcess;

typedef struct _GLOBLA_DATA
{
    PFLT_FILTER FilterHandle;
    APP_COMMUNICATION Communication;
    ULONG MonitoringStarted;
    PDRIVER_OBJECT DriverObject;
    UINT32 ActiveMonitoring;

}GLOBLA_DATA, *PGLOBLA_DATA;

NTSTATUS
GetProcessName(
    HANDLE Pid,
    PUNICODE_STRING* ProcessName
);

extern GLOBLA_DATA gDrv;

#endif