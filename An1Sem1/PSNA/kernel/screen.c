#include "screen.h"

static PSCREEN gVideo = (PSCREEN)(0x000B8000);
unsigned short gLocation = MAX_COLUMNS;
unsigned short gLocationWrite = MAX_COLUMNS * (MAX_LINES - 1) + 1;

void CursorMove(int row, int col)
{
    gLocation = (row * MAX_COLUMNS) + col;       /* Short is a 16bit type , the formula is used here*/

    //Cursor Low port
    __outbyte(0x3D4, 0x0F);                                    //Sending the cursor low byte to the VGA Controller
    __outbyte(0x3D5, (unsigned char)(gLocation & 0xFF));

    //Cursor High port
    __outbyte(0x3D4, 0x0E);                                    //Sending the cursor high byte to the VGA Controller
    __outbyte(0x3D5, (unsigned char)((gLocation >> 8) & 0xFF)); //Char is a 8bit type
}

void PutKeyboardChar(char C)
{
    int pos = gLocationWrite;
    if (pos > MAX_OFFSET)
    {
        pos = MAX_COLUMNS * (MAX_LINES - 1) + 1;
    }

    gVideo[pos].color = 10;
    gVideo[pos].c = C;
    pos++;
    gLocationWrite = pos;

}

void ClearCommandPortion()
{
    for (int i = MAX_COLUMNS * (MAX_LINES - 1) + 1; i < MAX_OFFSET; i++)
    {
        gVideo[i].color = 0;
        gVideo[i].c = ' ';
    }
    gLocationWrite = MAX_COLUMNS * (MAX_LINES - 1) + 1;
}

void CursorPosition(int pos)
{
    int row, col;

    if (pos > MAX_OFFSET)
    {
        pos = pos % MAX_OFFSET;
    }

    row = pos / MAX_COLUMNS;
    col = pos % MAX_COLUMNS;

    CursorMove(row, col);
}

void HelloBoot()
{
    int i, len;
	char boot[] = "Hello Boot! Greetings from C...";

	len = 0;
	while (boot[len] != 0)
	{
		len++;
	}

	for (i = 0; (i < len) && (i < MAX_OFFSET); i++)
	{
		gVideo[i].color = 10;
		gVideo[i].c = boot[i];
	}
    CursorPosition(i);
}

/// 0x000B8000      |   

void ClearScreen()
{
    unsigned int i;

    for (i = 0; i < MAX_OFFSET; i++)
    {
        gVideo[i].color = 0x33;
        gVideo[i].c = ' ';
    }

    CursorMove(0, 0);
}

void PutChar(char C, int Pos)
{
    unsigned int crtpos = Pos;

    gVideo[crtpos].color = 10;
    gVideo[crtpos].c = C;
}

void 
PutString(
    char* String
)
{
    unsigned int crtpos = gLocation;
    
    if(crtpos == 0)
        crtpos = MAX_COLUMNS;
    
    char *stringShadow = String;

    while (*stringShadow != 0)
    {
        if (crtpos >= (MAX_OFFSET - MAX_COLUMNS))
        {
            crtpos = MAX_COLUMNS;
        }

        gVideo[crtpos].color = 10;
        gVideo[crtpos].c = *stringShadow;

        crtpos++;
        stringShadow++;
    }

    CursorPosition(crtpos);
}

void PutStringAtPosition(char* String, int Pos)
{
    int crtpos = Pos;
    char* stringShadow = String;

    while(*stringShadow)
    {
        gVideo[crtpos].color = 10;
        gVideo[crtpos].c = *stringShadow;

        stringShadow++;
        crtpos++;
    }

}

