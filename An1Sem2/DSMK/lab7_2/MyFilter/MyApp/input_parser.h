#ifndef _INPUT_PARSER_H_INCULDED_
#define _INPUT_PARSER_H_INCULDED_

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS

#define Ui_MAX_INPUT_BUFFER_SIZE 0x100

int
UiHandleCommands(
    );

#endif