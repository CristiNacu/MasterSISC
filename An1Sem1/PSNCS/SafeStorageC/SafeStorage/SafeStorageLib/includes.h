#ifndef _INCLUDES_H_
#define _INCLUDES_H_ 

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif 

#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif 

#include <crtdbg.h>
#include <stdio.h>
#include <assert.h>

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <intsafe.h>
#include <strsafe.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) == STATUS_SUCCESS)
#endif

#endif //_INCLUDES_H_