#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

void
KeyboardDriverInit(void (*HandleEnterCallback)(char*));

void
KeyboardDriver();

#endif