#ifndef _PROCESS_FILTER_HPP_INCLUDED_
#define _PROCESS_FILTER_HPP_INCLUDED_

#include "MyDriver.h"

NTSTATUS
ProcFltInitialize();

NTSTATUS
ProcFltUninitialize();

#endif