#ifndef _COMMANDS_H_
#define _COMMANDS_H_
//
//   Copyright (C) 2018 BitDefender S.R.L.
//   Author(s)    : Radu PORTASE(rportase@bitdefender.com)
//

#include "CommShared.h"
#include <fltUser.h>
#include <ntstatus.h>

/// Command structures are defined in "atccommshared.h"

//
// CmdGetDriverVersion
//
NTSTATUS
CmdGetDriverVersion(
    _Out_ PULONG DriverVersion
    );

//
// CmdStartMonitoring
//

NTSTATUS
CmdStartMonitoring(
    _In_ UINT32 EnableOptions
    );

//
// CmdStopMonitoring
//

NTSTATUS
CmdStopMonitoring(
    _In_ UINT32 DisableOptions
    );

#endif//_COMMANDS_H_