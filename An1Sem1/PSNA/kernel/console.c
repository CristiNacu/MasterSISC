#include "console.h"
#include "utils.h"
#include "screen.h"

void HandleConsoleInput(char* buff)
{
    if (strcmp_custom(buff, "PRINT") == 0)
    {
        PutString("Print command ok\n");
    }
    else if (strcmp_custom(buff, "CLRSCR") == 0)
    {
        ClearScreen();
    }
    else if (strcmp_custom(buff, "MAGIC") == 0)
    {
        __magic();
    }
    else if (strcmp_custom(buff, "REBOOT") == 0)
    {
        __outbyte(0x64, 0xFE);
    }

}
