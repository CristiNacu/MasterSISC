
#include "ImageFilter.h"
#include "CommShared.h"

VOID
ImageNotifyRoutine(
	_In_opt_ PUNICODE_STRING FullImageName,
	_In_ HANDLE ProcessId,
	_In_ PIMAGE_INFO ImageInfo
)
{
    UNREFERENCED_PARAMETER(ImageInfo);
	PMY_DRIVER_MSG_IMAGE_NOTIFICATION message;
	UINT32 messageSize = sizeof(MY_DRIVER_MSG_FILE_NOTIFICATION);

	if (FullImageName)
	{
		messageSize += FullImageName->Length;
	}

    message = ExAllocatePoolWithTag(
        PagedPool,
        messageSize,
        REGISTRY_COMMUNICATION_POOL_TAG
    );
    if (!message)
    {
        goto cleanup;
    }
    
    message->Header.MessageCode = msgImageLoad;
    message->ProcessId = HandleToULong(ProcessId);
    KeQuerySystemTime(&message->Timestamp);
    

    if (FullImageName)
    {
        message->ImagePathLength = FullImageName->Length;
        memcpy(
            &message->Data[0],
            FullImageName->Buffer,
            FullImageName->Length
        );
    }
    else
    {
        message->ImagePathLength = 0;
    }

    CommSendMessage(message, messageSize, NULL, NULL);
    return;

cleanup:

    if (message)
    {
        ExFreePoolWithTag(
            message,
            REGISTRY_COMMUNICATION_POOL_TAG
        );
    }

	return;
}

NTSTATUS
ImgFltInitialize()
{
	return PsSetLoadImageNotifyRoutine(
		ImageNotifyRoutine
	);
}

NTSTATUS
ImgFltUninitialize()
{
	return PsRemoveLoadImageNotifyRoutine(
		ImageNotifyRoutine
	);
}