#include "includes.h"

_Use_decl_annotations_
NTSTATUS
CopyPath(
    _Inout_ PCHAR *NewPath,
    _In_ PCHAR InitialPath
);

_Use_decl_annotations_
NTSTATUS
CopyString(
    _In_ PCHAR *NewString,
    _In_ PCHAR InitialString,
    _In_ DWORD InitialStringSize
);

_Use_decl_annotations_
NTSTATUS
GetCurrentWorkingDirectory(
    _Inout_ PCHAR *CurrentDirectory
);

_Use_decl_annotations_
BOOL
DirectoryExists(
    _In_ PCHAR FilePath
);

_Use_decl_annotations_
NTSTATUS
GetFileDirectoryPath(
    _Inout_ PCHAR FilePath
);

_Use_decl_annotations_
BOOL
FileExists(
    _In_ PCHAR FilePath
);

_Use_decl_annotations_
BOOL
AccessibleFile(
    _In_ PCHAR FilePath
);

_Use_decl_annotations_
NTSTATUS
ConcatenatePath(
    _In_ PCHAR PathBase,
    _In_ PCHAR PathExtension,
    _In_ BOOL AddSlashBefore
);

_Use_decl_annotations_
NTSTATUS
GetFileName(
    _In_ PCHAR FilePath,
    _Out_ PCHAR *FileName
);

_Use_decl_annotations_
void
StringToLower(
    _In_reads_bytes_opt_(UsernameSize) PCHAR Username,
    _In_ DWORD UsernameSize
);

_Use_decl_annotations_
NTSTATUS
CreateDirectoryStructure(
    _In_ PCHAR destinationPath,
    _In_ LPSECURITY_ATTRIBUTES directoryAttributes
);