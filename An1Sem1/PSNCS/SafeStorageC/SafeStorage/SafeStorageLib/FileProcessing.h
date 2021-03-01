#include "includes.h"

typedef struct _FILE_COPY_TASK {
    HANDLE SourceFileHandle;
    HANDLE DestinationFileHandle;
    DWORD Offset;
    DWORD SizeOfCopy;

    HANDLE Mutex;
    PDWORD NumberOfWorkSplitsThreads;
    PCHAR DestinationFilePath;
} FILE_COPY_TASK, *PFILE_COPY_TASK;

_Use_decl_annotations_
NTSTATUS
AddFileProcessingJob(
    _In_ PCHAR Source,
    _In_ PCHAR Destination
);

