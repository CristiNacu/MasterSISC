#ifndef _COMMANDS_H_INCLUDED_
#define _COMMANDS_H_INCLUDED_

#include "globaldata.h"

#define Ui_CMD_MAX_NUMBER_OF_PARAMETERS 5

//
// Ui_COMMAND_PARAMETER_TYPE
//
typedef enum _Ui_COMMAND_PARAMETER_TYPE
{
    paramNoMoreParameters = -1,
    paramInteger = 0,
    paramString  = 1,
}Ui_COMMAND_PARAMETER_TYPE, *PUi_COMMAND_PARAMETER_TYPE;


//
// Ui_COMMAND_PARAMETER
//
typedef struct _Ui_COMMAND_PARAMETER
{
    Ui_COMMAND_PARAMETER_TYPE Type;
    union 
    {
        __int64 IntegerValue;
        PWSTR  StringValue;
    }Value;
}Ui_COMMAND_PARAMETER, *PUi_COMMAND_PARAMETER;

//
// Ui_COMMAND_PARAMETER_DECLARATION
//
typedef struct _Ui_COMMAND_PARAMETER_DECLARATION
{
    PCWSTR                       Name;    // name of the parameter. Used when printig help
    Ui_COMMAND_PARAMETER_TYPE    Type;    // type of the parameter
}Ui_COMMAND_PARAMETER_DECLARATION, *PUi_COMMAND_PARAMETER_DECLARATION;

//
// Ui_COMMAND_RETURN_STATUS
//
typedef enum _Ui_COMMAND_RETURN_STATUS
{
    retCmdSuccess               = 0,
    retCmdSuccessNoMoreCommands = 1,
    retCmdFailed                = 2,
    retCmdFailedNoMoreCommands  = 3,
}Ui_COMMAND_RETURN_STATUS, *PUi_COMMAND_RETURN_STATUS;


//
// PFUNC_UiCommandHandler
//
typedef Ui_COMMAND_RETURN_STATUS(*PFUNC_UiCommandHandler)(
    _In_ Ui_COMMAND_PARAMETER Parameters[Ui_CMD_MAX_NUMBER_OF_PARAMETERS]
    ); 

//
// _Ui_COMMAND
//
typedef struct _Ui_COMMAND
{
    PCWSTR                               CommandKey;                                     // Command key
    Ui_COMMAND_PARAMETER_DECLARATION  Parameters[Ui_CMD_MAX_NUMBER_OF_PARAMETERS]; // Parameters for the command
    PCWSTR                               CommandMessage;                                 // Message for the command
    PFUNC_UiCommandHandler            Handler;
} Ui_COMMAND, *PUi_COMMAND;


extern Ui_COMMAND gUiCommands[];       // all the defined  commands
extern DWORD      gUiNumberOfCommands; // the number of  commands


//
// CmdHandlerHelp
//
Ui_COMMAND_RETURN_STATUS
CmdHandlerHelp(
    _In_ Ui_COMMAND_PARAMETER Parameters[Ui_CMD_MAX_NUMBER_OF_PARAMETERS]
    );

//
// CmdHandlerExit
//
Ui_COMMAND_RETURN_STATUS
CmdHandlerExit(
    _In_ Ui_COMMAND_PARAMETER Parameters[Ui_CMD_MAX_NUMBER_OF_PARAMETERS]
    );

//
// CmdHandlerStartMonitoring
//
Ui_COMMAND_RETURN_STATUS
CmdHandlerStartMonitoring(
    _In_ Ui_COMMAND_PARAMETER Parameters[Ui_CMD_MAX_NUMBER_OF_PARAMETERS]
    );

//
// CmdHandlerStopMonitoring
//
Ui_COMMAND_RETURN_STATUS
CmdHandlerStopMonitoring(
    _In_ Ui_COMMAND_PARAMETER Parameters[Ui_CMD_MAX_NUMBER_OF_PARAMETERS]
    );

#endif //_COMMANDS_H_INCLUDED