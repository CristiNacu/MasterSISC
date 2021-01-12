#include "console.h"
#include "utils.h"
#include "screen.h"
#include "disk.h"
#include "logging.h"

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
    else if (strcmp_custom(buff, "READSECTOR") == 0)
    {
        QWORD readAddress = ReadSector(0,0,0);
        char address[17];

        Log("Cylinder Head Sector 0 0 0 read to: ");

        for (int i = 0; i <= 16; i++)
        {
            address[16 - i] = readAddress % 16;
            if (address[16 - i] > 9)
            {
                address[16 - i] = address[16 - i] - 10 + 'A';
            }
            else
            {
                address[16 - i] = address[16 - i] + '0';
            }
        }

        PutString(address);

    }
    else
    {
        PutString(buff);
    }

}
