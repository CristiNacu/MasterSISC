#ifndef _COMMON_H_
#define _COMMON_H_

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <windef.h>
#include <wdm.h>

const wchar_t OPERATION_NAME[] = {
	L"Process Create ",
	L"Process Terminate ",

	L"Thread Create ",
	L"Thread Terminate ",

	L"Image Loaded ",

	L"Registry Create ",
	L"Registry Set Value ",
	L"Registry Delete Key ",
	L"Registry Delete Value ",
	L"Registry Load Key ",
	L"Registry Rename Key ",

	L"File Create ",
	L"File Close ",
	L"File Cleanup ",
	L"File Read ",
	L"File Write ",
	L"File Set Attributes ",
}

typedef enum MONITORING_COMPONENT = {

	COMPONENT_PROCESS_CREATE,
	COMPONENT_PROCESS_TERMINATE,
	COMPONENT_THREAD_CREATE,
	COMPONENT_THREAD_TERMINATE,
	COMPONENT_IMAGE_LOAD,
	COMPONENT_REGISTRY_CREATE,
	COMPONENT_REGISTRY_SET_VALUE,
	COMPONENT_REGISTRY_DELETE_KEY,
	COMPONENT_REGISTRY_DELETE_VALUE,
	COMPONENT_REGISTRY_LOAD_KEY,
	COMPONENT_REGISTRY_RENAME_KEY,
	COMPONENT_FILE_CREATE,
	COMPONENT_FILE_CLOSE,
	COMPONENT_FILE_CLEANUP,
	COMPONENT_FILE_READ,
	COMPONENT_FILE_WRITE,
	COMPONENT_FILE_SET_ATTRIBUTES,
}

#define BIT(n)		(1 << (n))
#define ENABLE_COMPONENT_BIT	BIT(31)		
#define ENABLE_MONITORING_COMPONENT(component)		((component) | ENABLE_COMPONENT_BIT)
#define DISABLE_MONITORING_COMPONENT(component)		((component) & (~ENABLE_COMPONENT_BIT))
#define ENABLE_COMPONENT(component)					((component) & ENABLE_COMPONENT_BIT))

#define GET_COMPONENT_ID(component)					((component) & (~ENABLE_COMPONENT_BIT))

typedef struct _DRIVER_COMPONENT_STATUS {

	union {
		struct {
			DWORD ProcessCreate			: 1;
			DWORD ProcessTerminate		: 1;
			DWORD ThreadCreate			: 1;
			DWORD ThreadTerminate		: 1;
			DWORD ImageLoad				: 1;
			DWORD RegistryCreate		: 1;
			DWORD RegistrySetValue		: 1;
			DWORD RegistryDeleteKey		: 1;
			DWORD RegistryDeleteValue	: 1;
			DWORD RegistryLoadKey		: 1;
			DWORD RegistryRenameKey		: 1;
			DWORD FileCreate			: 1;
			DWORD FileClose				: 1;
			DWORD FileCleanup			: 1;
			DWORD FileCleanup			: 1;
		}

		DWORD Raw;
	
	} NOTIFICATION_COMPONENT_STATUS;

} DRIVER_COMPONENT_STATUS;

typedef struct _DRIVER_GLOBALS {

	DRIVER_COMPONENT_STATUS ComponentStatus;
	KSPIN_LOCK ComponentInitializationSpinLock;
	
} DRIVER_GLOBALS;

#define FUNCTION_ERRORR(funct_name, status) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Function " ## funct_name ## "returned status %X\n", status)



#endif // !_COMMON_H_
