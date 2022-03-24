#include <stdio.h>
#include "input_parser.h"
#include <ntstatus.h>
#include <wchar.h>
#include "commands.h"
#include <malloc.h>

//
// UiP_COMMAND_PARSER
//
typedef struct _UiP_COMMAND_PARSER
{
    PCWSTR  CommandBuffer;
    DWORD   CommandBufferSize;
    DWORD   CursorPosition;
}UiP_COMMAND_PARSER, *PUiP_COMMAND_PARSER;

//
// UiP_COMMAND_PARSER_STATUS
//
typedef enum _UiP_COMMAND_PARSER_STATUS
{
    atpsParseFailed = -1,   // the fail status
    atpsTokenOk     = 0,
    atpsTokenInvalid= 1,
    atpsNoMoreTokens= 2,
}UiP_COMMAND_PARSER_STATUS, *PUiP_COMMAND_PARSER_STATUS;


//
// UipInitializeCommandParser
//
VOID
__forceinline
UipInitializeCommandParser(
    _Out_ PUiP_COMMAND_PARSER Parser,
    _In_  PCWSTR Command,
    _In_  DWORD CommandSize
    )
{
    Parser->CommandBuffer = Command;
    Parser->CommandBufferSize = CommandSize;
    Parser->CursorPosition = 0;
}

//
// UipGenerateStringParam
//
_Success_(return == atpsTokenOk)
UiP_COMMAND_PARSER_STATUS
__forceinline
UipGenerateStringParam(
    _Inout_ PUiP_COMMAND_PARSER   Parser,
    _Out_   PUi_COMMAND_PARAMETER Parameter,
    _In_    DWORD                 ParsedSize
    )
{
    Parameter->Type = paramString;
    Parameter->Value.StringValue = malloc((ParsedSize + 1)*sizeof(WCHAR));
    if (NULL == Parameter->Value.StringValue)
    {
        return atpsParseFailed;
    }
    else
    {
        RtlCopyMemory(Parameter->Value.StringValue,
            &Parser->CommandBuffer[Parser->CursorPosition],
            ParsedSize*sizeof(WCHAR));

        Parameter->Value.StringValue[ParsedSize] = L'\0';

        Parser->CursorPosition += ParsedSize;    
    }

    return atpsTokenOk;
}

//
// UipGetStringParameter
//
_Success_(return == atpsTokenOk)
UiP_COMMAND_PARSER_STATUS
__forceinline
UipGetStringParameter(
    _Inout_ PUiP_COMMAND_PARSER   Parser,
    _Out_   PUi_COMMAND_PARAMETER Parameter
    )
{
    UiP_COMMAND_PARSER_STATUS status = atpsParseFailed;
    DWORD dwMaxSizeToParse = 0;
    BOOLEAN bShouldCheckEndQuote = FALSE;
    DWORD i; 

    // not checking for underflow due to application logic
    dwMaxSizeToParse = Parser->CommandBufferSize - Parser->CursorPosition;
    for (i = Parser->CursorPosition; i <= dwMaxSizeToParse; i++)
    {
        //
        // Check for end of the string
        //
        if (Parser->CommandBuffer[i] == L' ' ||
            Parser->CommandBuffer[i] == L'\0')
        {
            status = UipGenerateStringParam(Parser, Parameter, i);
            break;
        }
        
        //
        // Check for quote escaped characters
        //
        if (Parser->CommandBuffer[i] == L'"')
        {
            if (bShouldCheckEndQuote)
            {
                status = UipGenerateStringParam(Parser, Parameter, i-1);
                break;
            }

            if (i == 0)
            {
                bShouldCheckEndQuote = TRUE;
                Parser->CursorPosition++;
            }
            else
            {
                status = atpsParseFailed;
                break;
            }
        }
    }

    return status;
}

//
// UipGetIntegerParameter
//
_Success_(return == atpsTokenOk)
UiP_COMMAND_PARSER_STATUS
__forceinline
UipGetIntegerParameter(
    _Inout_  PUiP_COMMAND_PARSER   Parser,
    _Out_    PUi_COMMAND_PARAMETER Parameter
    )
{
    UiP_COMMAND_PARSER_STATUS status = atpsParseFailed;
    __int8 intBase = 10;
    __int64 intValue = 0;
    DWORD dwMaxSizeToParse = 0;
    DWORD i;

    // not checking for underflow due to application logic
    dwMaxSizeToParse = Parser->CommandBufferSize - Parser->CursorPosition + 1;


    //
    // Check for base
    //
    if ((dwMaxSizeToParse > 2) &&
        Parser->CommandBuffer[0] == L'0' &&
        (Parser->CommandBuffer[1] == L'x' || Parser->CommandBuffer[1] == L'X')
        )
    {
        Parser->CommandBufferSize += 2;
        intBase = 16;
    }

    for (i = Parser->CursorPosition; i < dwMaxSizeToParse; ++i)
    {
        //
        // Check for end of the string
        //
        if (Parser->CommandBuffer[i] == L' ' ||
            Parser->CommandBuffer[i] == L'\0')
        {
            status = atpsTokenOk;
            Parser->CursorPosition += i;
            Parameter->Type = paramInteger;
            Parameter->Value.IntegerValue = intValue;
            break;
        }

        if (L'0' <= Parser->CommandBuffer[i] && Parser->CommandBuffer[i] <= L'9')
        {
            intValue = intValue *intBase + (Parser->CommandBuffer[i] - L'0');
            continue;
        }

        if (intBase == 16) 
        {
            if (L'a' <= Parser->CommandBuffer[i] && Parser->CommandBuffer[i] <= L'f')
            {
                if (intValue <= intValue *intBase + (Parser->CommandBuffer[i] - L'a' + 10))
                {
                    intValue = intValue *intBase + (Parser->CommandBuffer[i] - L'a' + 10);
                    continue;
                }  
                
            }
            else if (L'A' <= Parser->CommandBuffer[i] && Parser->CommandBuffer[i] <= L'F')
            {
                if (intValue <= intValue *intBase + (Parser->CommandBuffer[i] - L'A' + 10))
                {
                    intValue = intValue *intBase + (Parser->CommandBuffer[i] - L'A' + 10);
                    continue;
                }
            }
        }

        status = atpsParseFailed;
        break;
    }

    return status;
}

//
// UipGetCommandParameter
//
_Success_(return == atpsParseFailed)
UiP_COMMAND_PARSER_STATUS
__forceinline
UipGetCommandParameter(
    _Inout_  PUiP_COMMAND_PARSER       Parser,
    _In_     Ui_COMMAND_PARAMETER_TYPE Type,
    _Inout_  PUi_COMMAND_PARAMETER     Parameter 
    )
{
    switch (Type)
    {
    case paramInteger:
        return UipGetIntegerParameter(Parser, Parameter);
    case paramString:
        return UipGetStringParameter(Parser, Parameter);
    default:
        return atpsParseFailed;
    }
}

//
// UipGetCommadName
//


//
// UipDispatchCommand
//
NTSTATUS
UipDispatchCommand( 
    _In_ PWCHAR Command,
    _Out_ PUi_COMMAND_RETURN_STATUS CommandStatus)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    Ui_COMMAND_PARAMETER commandName;
    Ui_COMMAND_PARAMETER parameter[Ui_CMD_MAX_NUMBER_OF_PARAMETERS];
    UiP_COMMAND_PARSER   parser;
    UiP_COMMAND_PARSER_STATUS parserStatus;
    BOOLEAN                 bIsCommandFound = FALSE;
    DWORD                   dwCommandIndex = 0;
    DWORD i;

    RtlZeroMemory(&commandName, sizeof(commandName));
    RtlZeroMemory(parameter, sizeof(parameter));

    __try
    {
        //
        // Set the command to the failed status
        //
        *CommandStatus = retCmdFailed;

        //
        // Initialize our mini parser
        //
        UipInitializeCommandParser(&parser, Command, (DWORD)wcsnlen_s(Command, Ui_MAX_INPUT_BUFFER_SIZE - 1) + 1);

        //
        // Find our command
        //

        // get the name
        parserStatus = UipGetCommandParameter(&parser, paramString, &commandName);
        if (atpsParseFailed == parserStatus)
        {
            status = STATUS_INVALID_PARAMETER_2;
            *CommandStatus = retCmdFailed;
            __leave;
        }

        for (i = 0; i < gUiNumberOfCommands; ++i)
        {
            if (0 == wcscmp(commandName.Value.StringValue, gUiCommands[i].CommandKey))
            {
                dwCommandIndex = i;
                bIsCommandFound = TRUE;
                break;
            }
        }

        if (!bIsCommandFound)
        {
            wprintf(L"[Error] Command %s is not defined. Try help. \n", commandName.Value.StringValue);
            status = STATUS_INVALID_PARAMETER_2;
            __leave;
        }

        //
        // Get the parameters
        //
        for (i = 0; i < Ui_CMD_MAX_NUMBER_OF_PARAMETERS; i++)
        {
            if (paramNoMoreParameters == gUiCommands[dwCommandIndex].Parameters[i].Type)
            {
                if (1 != parser.CommandBufferSize - parser.CursorPosition)
                {
                    wprintf(L"[Warning] too many parameters for command %s. Extra parameters are ignored.\n", commandName.Value.StringValue);
                }

                *CommandStatus = gUiCommands[dwCommandIndex].Handler(parameter);

                break;
            }
            else
            {
                parserStatus = UipGetCommandParameter(&parser,
                    gUiCommands[dwCommandIndex].Parameters[i].Type,
                    &parameter[i]);
                if (atpsParseFailed == parserStatus)
                {
                    wprintf(L"[Error] Parameter %d is invalid for command %s.\n", i, commandName.Value.StringValue);
                    status = STATUS_INVALID_PARAMETER_2;
                    *CommandStatus = retCmdFailed;
                    __leave;
                }
            }
        }
        

        status = STATUS_SUCCESS;
    }
    __finally
    {
        //
        // Free the memory
        //
        if (NULL != commandName.Value.StringValue)
        {
            free(commandName.Value.StringValue);
        }

        for (i = 0; i < Ui_CMD_MAX_NUMBER_OF_PARAMETERS; i++)
        {
            if (paramString == parameter[i].Type && NULL != parameter[i].Value.StringValue)
            {
                free(parameter[i].Value.StringValue);
            }
        }

    }

    return status;
}

//
// UiHandleCommands
//
_Use_decl_annotations_
int
UiHandleCommands(
    )
{
    WCHAR pCurrentCommand[Ui_MAX_INPUT_BUFFER_SIZE];
    DWORD dwCurrentPosition = 0;
    WCHAR wchCurrentChar;
    Ui_COMMAND_RETURN_STATUS retStatus;
    BOOLEAN bIsExitSignaled = FALSE;

    RtlZeroMemory(pCurrentCommand, Ui_MAX_INPUT_BUFFER_SIZE);

    wprintf(L"[Info] Application is waiting for commands.\n");

    while (!bIsExitSignaled)
    {
        wchCurrentChar = _getwche();
        if (L'\r' == wchCurrentChar)
        {
            //
            // Prepare the command
            //
            pCurrentCommand[dwCurrentPosition] = L'\0';
            
            //
            // Dispatch the command
            //
            UipDispatchCommand(pCurrentCommand, &retStatus);

            //
            // Handle the return status
            // 
            switch (retStatus)
            {
            case retCmdSuccess:
                {
                wprintf(L"[Info] Command %s executed successfully.\n", pCurrentCommand);
                }
                break;
            case retCmdSuccessNoMoreCommands:
                {
                wprintf(L"[Info] Command %s executed successfully.\n", pCurrentCommand);
                    bIsExitSignaled = TRUE;
                }
                break;
            case retCmdFailed:
                {
                wprintf(L"[Error] Command %s failed.\n", pCurrentCommand);
                }
                break;
            case retCmdFailedNoMoreCommands:
                {
                    wprintf( L"[Error] Command %s failed.\n", pCurrentCommand);
                    bIsExitSignaled = TRUE;
                }
            break;
            default:
                {
                wprintf(L"[Error] Invalid return status received while running command %s.\n", pCurrentCommand);
                }
                break;
            }

            dwCurrentPosition = 0;
            wprintf(L">");
            continue;
        }

        pCurrentCommand[dwCurrentPosition] = wchCurrentChar;
        dwCurrentPosition++;
    }

    return STATUS_SUCCESS;
}
