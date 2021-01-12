#include "keyboard.h"
#include "main.h"
#include "screen.h"

char gKeyboardBuffer[255];
int crtWritingPosition;

void (*gHandleEnter)(char*) = NULL;

#define ESCAPE 1
#define BACKSPACE 2
#define ENTER 3
#define LEFT 4
#define RIGHT 5
#define UP 6
#define DOWN 7
#define UNKNOWN 0xFF
#define MAX_KEYBORD_BUFFER_LEN 255

BYTE KeyboardBuffer[MAX_KEYBORD_BUFFER_LEN + 1];
BYTE idx = 0;
BYTE lastChar = 0;

#define     uppercase (abs(('a' - 'A')))

typedef struct _KEY_EVENT
{
    BYTE pressed;
    char key;
} KEY_EVENT;

BYTE
DecodeKey(
    BYTE Code
)
{
    BYTE c = Code & 0x7F;

    if (c > 0x1 && c < 0xC)
    {
        return '0' + abs(((int)c - 0xB));
    }
    switch (c)
    {
    case 0x1:
        return ESCAPE;
    case 0xE:
        return BACKSPACE;
    case 0x10:
        return 'q' - uppercase;
    case 0x11:
        return 'w' - uppercase;
    case 0x12:
        return 'e' - uppercase;
    case 0x13:
        return 'r' - uppercase;
    case 0x14:
        return 't' - uppercase;
    case 0x15:
        return 'y' - uppercase;
    case 0x16:
        return 'u' - uppercase;
    case 0x17:
        return 'i' - uppercase;
    case 0x18:
        return 'o' - uppercase;
    case 0x19:
        return 'p' - uppercase;
    case 0x1C:
        return ENTER;
    case 0x1E:
        return 'a' - uppercase;
    case 0x1F:
        return 's' - uppercase;
    case 0x20:
        return 'd' - uppercase;
    case 0x21:
        return 'f' - uppercase;
    case 0x22:
        return 'g' - uppercase;
    case 0x23:
        return 'h' - uppercase;
    case 0x24:
        return 'j' - uppercase;
    case 0x25:
        return 'k' - uppercase;
    case 0x26:
        return 'l' - uppercase;
    case 0x2C:
        return 'z' - uppercase;
    case 0x2D:
        return 'x' - uppercase;
    case 0x2E:
        return 'c' - uppercase;
    case 0x2F:
        return 'v' - uppercase;
    case 0x30:
        return 'b' - uppercase;
    case 0x31:
        return 'n' - uppercase;
    case 0x32:
        return 'm' - uppercase;

    case 0x48:
        return UP;
    case 0x50:
        return DOWN;
    }

    return UNKNOWN;

}



void KeyboardDriverInit(void (*HandleEnterCallback)(char*))
{
    for (int i = 0; i < 255; i++)
    {
        gKeyboardBuffer[i] = 0;
    }

    gHandleEnter = HandleEnterCallback;
}

#define     KEYBOARD_DATA_PORT      0x60
#define     KEYBOARD_CONTROL_PORT   0x64
#define     KEYBOARD_STATUS_PORT    0x64

#define     OUTPUT_BUFFER_STATUS    BIT(0)
#define     INPUT_BUFFER_STATUS     BIT(1)

void KeyboardDriver()
{
    if (!(__inbyte(KEYBOARD_STATUS_PORT) & OUTPUT_BUFFER_STATUS))
    {
        return;
    }

    char c = __inbyte(KEYBOARD_DATA_PORT);
    char pressed = (c & (BYTE)0x80) != 0;
    char decoded = DecodeKey(c);

    if (decoded == ENTER && gHandleEnter)
    {
        Log("Enter encountered\n");
        gHandleEnter(gKeyboardBuffer);
        ClearCommandPortion();
        for (int i = 0; i <= MAX_KEYBORD_BUFFER_LEN; i++)
        {
            KeyboardBuffer[i] = 0;
        }
        crtWritingPosition = 0;
    }
    else if (decoded != UNKNOWN && !pressed && crtWritingPosition < c)
    {
        gKeyboardBuffer[crtWritingPosition] = decoded;
        PutKeyboardChar(gKeyboardBuffer[crtWritingPosition]);
        crtWritingPosition++;
    }
    else if (crtWritingPosition >= MAX_KEYBORD_BUFFER_LEN)
    {
        Log("[Warning] keyboard buffer is full");
    }
}
