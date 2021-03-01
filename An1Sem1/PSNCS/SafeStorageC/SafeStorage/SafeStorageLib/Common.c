#include "Common.h"

_Use_decl_annotations_
NTSTATUS
CopyPath(
    _Inout_ PCHAR *NewPath,
    _In_ PCHAR InitialPath
)
{
    PCHAR newPath = NULL;
    NTSTATUS statusFunction = STATUS_SUCCESS;
    errno_t status = 0;

    if (strnlen_s(InitialPath, MAX_PATH) == MAX_PATH)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    newPath = (PCHAR)malloc(MAX_PATH * sizeof(CHAR));
    if (newPath == NULL)
    {
        printf_s("strncpy_s failed with status %d", status);
        statusFunction = STATUS_ALLOTTED_SPACE_EXCEEDED;
        goto cleanup;
    }

    status = strncpy_s(newPath, MAX_PATH, InitialPath, MAX_PATH);
    if (status)
    {
        printf_s("strncpy_s failed with status %d", status);
        statusFunction = STATUS_COPY_PROTECTION_FAILURE;
        goto cleanup;
    }

    *NewPath = newPath;
    return statusFunction;

cleanup:
    if (newPath)
    {
        free(newPath);
    }
    *NewPath = NULL;
    return statusFunction;
}

_Use_decl_annotations_
NTSTATUS
CopyString(
    _In_ PCHAR *NewString,
    _In_ PCHAR InitialString,
    _In_ DWORD InitialStringSize
)
{
    PCHAR newString = NULL;
    NTSTATUS statusFunction = STATUS_SUCCESS;
    errno_t status = 0;

    if (strnlen_s(InitialString, InitialStringSize + 1) != InitialStringSize)
        return STATUS_BUFFER_OVERFLOW;

    newString = (PCHAR)malloc((InitialStringSize + 1) * sizeof(CHAR));
    if (newString == NULL)
    {
        printf_s("strncpy_s failed with error %x\n", GetLastError());
        statusFunction = STATUS_ALLOTTED_SPACE_EXCEEDED;
        goto cleanup;
    }

    status = strncpy_s(newString, InitialStringSize + 1, InitialString, InitialStringSize);
    if (status)
    {
        printf_s("strncpy_s failed with status %x\n", GetLastError());
        statusFunction = STATUS_COPY_PROTECTION_FAILURE;
        goto cleanup;
    }

    *NewString = newString;
    return statusFunction;

cleanup:
    if (newString)
    {
        free(newString);
    }
    *NewString = NULL;
    return statusFunction;
}

_Use_decl_annotations_
NTSTATUS
GetCurrentWorkingDirectory(
    _Inout_ PCHAR *CurrentDirectory
)
{

    DWORD cwdSize = 0;
    PCHAR cwd = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    cwd = (PCHAR)malloc(MAX_PATH * sizeof(CHAR));
    if (cwd == NULL)
    {
        printf_s("currentDirectory malloc returned NULL!\n");
        status = STATUS_STACK_OVERFLOW;
        goto cleanup;

    }

    cwdSize = GetCurrentDirectoryA(
        MAX_PATH,
        cwd
    );
    if (cwdSize == 0)
    {
        printf_s("GetCurrentDirectory error!\n");
        status = STATUS_ALLOTTED_SPACE_EXCEEDED;
        goto cleanup;
    }

    *CurrentDirectory = cwd;
    return status;

cleanup:

    if (cwd)
    {
        free(cwd);
    }
    *CurrentDirectory = NULL;
    return status;
}

_Use_decl_annotations_
BOOL
DirectoryExists(
    _In_ PCHAR FilePath
)
{
    DWORD dwAttrib = GetFileAttributesA(FilePath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

_Use_decl_annotations_
NTSTATUS
GetFileDirectoryPath(
    _Inout_ PCHAR FilePath
)
{
    int file_size = (int)strnlen_s(FilePath, MAX_PATH);
    for (int idx = file_size; idx >= 0; idx--)
    {
        if (FilePath[idx] == '\\')
        {
            FilePath[idx + 1] = '\0';
            break;
        }
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
BOOL
FileExists(
    _In_ PCHAR FilePath
)
{
    DWORD dwAttrib = GetFileAttributesA(FilePath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        (!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)));
}

_Use_decl_annotations_
BOOL
AccessibleFile(
    _In_ PCHAR FilePath
)
{
    DWORD dwAttrib = GetFileAttributesA(FilePath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        (!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) &&
        (!(dwAttrib & FILE_ATTRIBUTE_DEVICE)) &&
        (!(dwAttrib & FILE_ATTRIBUTE_SYSTEM)) &&
        (!(dwAttrib & FILE_ATTRIBUTE_VIRTUAL)) &&
        (!(dwAttrib & FILE_ATTRIBUTE_REPARSE_POINT))

        );
}

_Use_decl_annotations_
NTSTATUS
ConcatenatePath(
    _In_ PCHAR PathBase,
    _In_ PCHAR PathExtension,
    _In_ BOOL AddSlashBefore
)
{
    errno_t status;

    if (AddSlashBefore)
    {
        status = strcat_s(PathBase, MAX_PATH, "\\");
        if (status != 0)
        {
            if (status == ERANGE)
                return STATUS_BUFFER_TOO_SMALL;
            if (status == EINVAL)
                return STATUS_INVALID_PARAMETER;
        }
    }

    status = strcat_s(PathBase, MAX_PATH, PathExtension);
    if (status != 0)
    {
        if (status == ERANGE)
            return STATUS_BUFFER_TOO_SMALL;
        if (status == EINVAL)
            return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
GetFileName(
    _In_ PCHAR FilePath,
    _Out_ PCHAR *FileName
)
{
    PCHAR fileName = FilePath;
    int file_size = (int)strnlen_s(FilePath, MAX_PATH);
    for (int idx = file_size; idx >= 0; idx--)
    {
        if (FilePath[idx] == '\\')
        {
            fileName += (idx + 1);
            break;
        }
    }

    *FileName = fileName;
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
void
StringToLower(
    _In_reads_bytes_opt_(UsernameSize) PCHAR Username,
    _In_ DWORD UsernameSize
)
{
    for (DWORD unameIdx = 0; unameIdx < UsernameSize; unameIdx++)
        if (isalpha(Username[unameIdx]))
            Username[unameIdx] = (CHAR)tolower(Username[unameIdx]);
}

_Use_decl_annotations_
NTSTATUS 
CreateDirectoryStructure(
    _In_ PCHAR destinationPath, 
    _In_ LPSECURITY_ATTRIBUTES directoryAttributes
)
{
    NTSTATUS status;
    PCHAR workPath = NULL;
    DWORD len;
    len = (DWORD)strnlen_s(destinationPath, MAX_PATH);

    if (len == 0 || len >= MAX_PATH)
    {
        printf("Invalid path size\n");
        return STATUS_INVALID_PARAMETER_1;
    }
    // retrieve test3 d:\master\test\test\merge\Prezentari.zip
    status = CopyPath(&workPath, destinationPath);
    if (!NT_SUCCESS(status))
    {
        printf("Error copying path\n");
        goto cleanup;
    }

    for (DWORD i = 1; i < len; i++)
    {
        if (workPath[i] == '\\')
        {
            char aux = workPath[i + 1];
            workPath[i + 1] = 0;
            if (!DirectoryExists(workPath))
            {
                BOOL internalStatus;
                internalStatus = CreateDirectoryA(
                    workPath,
                    directoryAttributes
                );
                if (!internalStatus)
                {
                    printf("CreateDirectoryA extited with error %x\n", GetLastError());
                    status = STATUS_UNSUCCESSFUL;
                    goto cleanup;
                }
            }
            workPath[i + 1] = aux;
        }
    }

    if (!DirectoryExists(destinationPath))
    {
        BOOL internalStatus;
        internalStatus = CreateDirectoryA(
            destinationPath,
            directoryAttributes
        );
        if (!internalStatus)
        {
            printf("CreateDirectoryA extited with error %x\n", GetLastError());
            status = STATUS_UNSUCCESSFUL;
            goto cleanup;
        }
    }

cleanup:
    if (workPath)
    {
        free(workPath);
    }

    return status;
}
