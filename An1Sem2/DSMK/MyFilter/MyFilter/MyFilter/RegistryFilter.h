#ifndef _REGISTRY_FILTER_H_
#define _REGISTRY_FILTER_H_

#include "MyDriver.h"

NTSTATUS
RegFltInitialize(
    PDRIVER_OBJECT DriverObject
);

NTSTATUS
RegFltUninitialize();


#endif // !_REGISTRY_FILTER_H_
