#include "commands.h"
#include "globaldata.h"
#include <stdio.h>
#include "driver_commands.h"

#pragma region List of commands
//
// This array contains all the commands defined for the  Console Application
//
Ui_COMMAND gUiCommands[] = {
    //
    // exit
    //
    {
        L"exit",
        {
            { NULL, paramNoMoreParameters} // no more parameters
        },
        L"quit the  Console",
        CmdHandlerExit
    },

    //
    // help
    //
    {
        L"help",
        {
            { NULL, paramNoMoreParameters } // no more parameters
        },
        L"print the help message",
        CmdHandlerHelp
    },
    //
    // startmonitoring
    //
    {
        L"startmonitoring",
        {
            { L"FilterEnable", paramInteger },
            { NULL, paramNoMoreParameters } // no more parameters
        },
    L"Start monoitoring system activity",
    CmdHandlerStartMonitoring
    },
    //
    // stopmonitoring
    //
    {
        L"stopmonitoring",
        {
            { L"FilterDisable", paramInteger },
            { NULL, paramNoMoreParameters } // no more parameters
        },
    L"Stop monoitoring system activity",
    CmdHandlerStopMonitoring
    },
};
DWORD   gUiNumberOfCommands = (sizeof(gUiCommands)/sizeof(gUiCommands[0]));

#pragma endregion List of commands

#pragma region Command handlers
//
// CmdHandlerHelp
//
Ui_COMMAND_RETURN_STATUS
CmdHandlerHelp(
    _In_ Ui_COMMAND_PARAMETER Parameters[Ui_CMD_MAX_NUMBER_OF_PARAMETERS]
    )
{
    UNREFERENCED_PARAMETER(Parameters);
    DWORD i;
    DWORD j;

    wprintf(L"Commands supported by CON:\n");

    for (i = 0; i<gUiNumberOfCommands; i++)
    {
        // print the command name
        wprintf(L" * %s( ", gUiCommands[i].CommandKey);
        
        // print the parameter info
        for (j = 0; j < Ui_CMD_MAX_NUMBER_OF_PARAMETERS; j++)
        {
            if (gUiCommands[i].Parameters[j].Type == paramNoMoreParameters)
            {
                wprintf(L");\n");
                break;
            }

            switch (gUiCommands[i].Parameters[j].Type)
            {
            case paramInteger:
                wprintf(L"INT64 %s", gUiCommands[i].Parameters[j].Name);
                break;
            case paramString:
                wprintf(L"PCWSTR %s", gUiCommands[i].Parameters[j].Name);
                break;
            default:
                break;
            }

            if (gUiCommands[i].Parameters[j + 1].Type != paramNoMoreParameters)
            {
                wprintf(L", ");
            }
        }

        wprintf(L"       -- %s\n", gUiCommands[i].CommandMessage);
    }

    return retCmdSuccess;
}

//
// CmdHandlerExit
//
Ui_COMMAND_RETURN_STATUS
CmdHandlerExit(
    _In_ Ui_COMMAND_PARAMETER Parameters[Ui_CMD_MAX_NUMBER_OF_PARAMETERS]
    )
{
    UNREFERENCED_PARAMETER(Parameters);

    wprintf(L"[Info] Application is exiting...\n");
    return retCmdSuccessNoMoreCommands;
}

//
// CmdHandlerStartMonitoring
//
Ui_COMMAND_RETURN_STATUS
CmdHandlerStartMonitoring(
    _In_ Ui_COMMAND_PARAMETER Parameters[Ui_CMD_MAX_NUMBER_OF_PARAMETERS]
    )
{ 
    CmdStartMonitoring(
        (UINT32)Parameters[0].Value.IntegerValue
    );
    return retCmdSuccess;
}

//
// CmdHandlerStopMonitoring
//
Ui_COMMAND_RETURN_STATUS
CmdHandlerStopMonitoring(
    _In_ Ui_COMMAND_PARAMETER Parameters[Ui_CMD_MAX_NUMBER_OF_PARAMETERS]
    )
{
    CmdStopMonitoring(
        (UINT32)Parameters[0].Value.IntegerValue
    );
    return retCmdSuccess;
}

#pragma endregion Command Handlers
