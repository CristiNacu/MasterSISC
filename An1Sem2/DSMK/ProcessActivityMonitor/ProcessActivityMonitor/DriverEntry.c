#include "Common.h"
#include "FileOperationFilter.h"
#include "ImageFilter.h"
#include "ProcessFilter.h"
#include "RegistryFilter.h"
#include "ThreadFilter.h"

NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS status;

	status = RegisterFilesystemMinifilter();
	if (!NT_SUCCESS(status))
	{
		FUNCTION_ERRORR("RegisterFilesystemMinifilter", status);
		goto cleanup;
	}

	status = RegisterImageFilter();
	if (!NT_SUCCESS(status))
	{
		FUNCTION_ERRORR("RegisterImageFilter", status);
		goto cleanup;
	}

	status = RegisterProcessFilter();
	if (!NT_SUCCESS(status))
	{
		FUNCTION_ERRORR("RegisterProcessFilter", status);
		goto cleanup;
	}

	status = RegisterRegistryFilter();
	if (!NT_SUCCESS(status))
	{
		FUNCTION_ERRORR("RegisterRegistryFilter", status);
		goto cleanup;
	}
	
	status = RegisterThreadFilter();
	if (!NT_SUCCESS(status))
	{
		FUNCTION_ERRORR("RegisterThreadFilter", status);
		goto cleanup;
	}


	return STATUS_SUCCESS;

cleanup:
	return status;
}