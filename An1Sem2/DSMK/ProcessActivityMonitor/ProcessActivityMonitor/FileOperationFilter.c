#include "FileOperationFilter.h"


FLT_PREOP_CALLBACK_STATUS
DelProtectPreCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext);

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
	{ IRP_MJ_CREATE, 0, DelProtectPreCreate, NULL },
	{ IRP_MJ_CLOSE, 0, DelProtectPreCreate, NULL },
	{ IRP_MJ_CLEANUP, 0, DelProtectPreCreate, NULL },
	{ IRP_MJ_READ, 0, DelProtectPreCreate, NULL },
	{ IRP_MJ_WRITE, 0, DelProtectPreCreate, NULL },
	{ IRP_MJ_SET_INFORMATION, 0, DelProtectPreCreate, NULL },
	{ IRP_MJ_OPERATION_END }
};

FLT_PREOP_CALLBACK_STATUS 
DelProtectPreCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext)
{
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(FltObjects);
	
	WCHAR message = "";

	LARGE_INTEGER timestamp = { 0 };
	KeQuerySystemTime(&timestamp);
	
	DWORD size = 300;
	UNICODE_STRING* processName = (UNICODE_STRING*)ExAllocatePool(PagedPool, size);

	ZwQueryInformationProcess(NtCurrentProcess(), ProcessImageFileName,
		processName, size - sizeof(WCHAR), NULL);

	if (Data->Iopb->Parameters.Create.Options & FILE_DELETE_ON_CLOSE)
	{
	wsprintf(message, "[%lld] Operation: File Create PID: %d Process Name: %S Path: %S Result: %S Details: %S",
		Data);

	}

}

NTSTATUS RegisterFilesystemMinifilter()
{

	return STATUS_SUCCESS;
}
