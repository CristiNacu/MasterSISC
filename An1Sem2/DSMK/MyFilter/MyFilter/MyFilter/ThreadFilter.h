#ifndef _THREAD_FILTER_H_
#define _THREAD_FILTER_H_

#include "MyDriver.h"

NTSTATUS
ThreadFltInitialize();

NTSTATUS
ThreadFltUninitialize();

#endif // !_THREAD_FILTER_H_
