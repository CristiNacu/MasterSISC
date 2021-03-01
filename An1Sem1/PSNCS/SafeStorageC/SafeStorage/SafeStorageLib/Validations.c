#include "Validations.h"
#include "Common.h"

CHAR _RESTRICTED_CHARS_STRINGS[] = "\\/:*?\"<>|%";
CHAR _RESTRICTED_CHARS_PATH[] = "?*%";

#define     USERNAME_SEED   7
#define     PASSWD_SEED     13

#define     PASSWD_MIN_SIZE 8

_Use_decl_annotations_
BOOL 
_AreBasePathsEqual(
    _In_ PCHAR PathToValidate,
    _In_ PCHAR BasePath
)
{
    if (strnlen_s(BasePath, MAX_PATH) == MAX_PATH)
    {
        printf("[WARNING] Provided BasePath exceeds the MAX_PATH limit of %d characters\n", MAX_PATH);
        return FALSE;
    }

    if (strncmp(BasePath, PathToValidate, (strnlen_s(BasePath, MAX_PATH) - 1)) == 0)
        return TRUE;
    return FALSE;
}

_Use_decl_annotations_
BOOL 
_ValidateNoWildcards(
    _In_ PCHAR PathToValidate,
    _In_ PCHAR Wildcards
)
{
    DWORD length = (DWORD)strnlen_s(PathToValidate, MAX_PATH);
    if (length == MAX_PATH)
    {
        printf("[WARNING] Provided PathToValidate exceeds the MAX_PATH limit of %d characters\n", MAX_PATH);
        return FALSE;
    }

    for (DWORD chrIdx = 0; chrIdx < length; chrIdx++)
    {
        if (strchr(Wildcards, PathToValidate[chrIdx]))
            return FALSE;
        if (PathToValidate[chrIdx] == '.' && PathToValidate[chrIdx + 1] == '.')
            return FALSE;
    }
    return TRUE;
}

_Use_decl_annotations_
NTSTATUS
ValidatePath(
    _In_ PCHAR Path,
    _In_opt_ DWORD PathSize,
    _In_ PATH_VALIDATION_OPTIONS ValidationOptions,
    _In_opt_ PCHAR UserPath,
    _Out_ PATH_VALIDATION_OPTIONS *ValidationResult
)
{
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS internalStatus = STATUS_SUCCESS;

    DWORD pathLength = 0;
    PCHAR filePathCopy = NULL;
    PCHAR cwd = NULL;

    PATH_VALIDATION_OPTIONS returnOptions = { 0 };

    if (!Path)
        return STATUS_INVALID_PARAMETER_1;

    pathLength = (DWORD)strnlen_s(Path, MAX_PATH);

    if (pathLength == MAX_PATH || pathLength == 0)
    {
        returnOptions.Options.ValidateLength = FALSE;
        status = STATUS_ABANDONED;
        goto cleanup;
    }


    if (ValidationOptions.Options.ValidateDirectoryStructureExists ||
        ValidationOptions.Options.ValidateIsCurrentWorkingDirectory ||
        ValidationOptions.Options.ValidateIsCurrentWorkingDirectoryUserPath)
    {

        if (Path[0] > 'Z')
            Path[0] -= ('a' - 'A');

        internalStatus = GetCurrentWorkingDirectory(&cwd);
        if (!NT_SUCCESS(internalStatus))
        {
            printf("GetCurrentWorkingDirectory returned error %x\n", internalStatus);
        }
        
        if (ValidationOptions.Options.IsFilePath)
        {
            internalStatus = CopyPath(&filePathCopy, Path);
            if (!NT_SUCCESS(internalStatus))
            {
                printf("CopyPath returned error %x\n", internalStatus);
            }
            else
            {
                internalStatus = GetFileDirectoryPath(filePathCopy);
                if (!NT_SUCCESS(internalStatus))
                {
                    printf("GetFileDirectoryPath returned error %x\n", internalStatus);
                }
            }
        }
    }

    if (ValidationOptions.Options.ValidateLength && PathSize && pathLength == PathSize)
    {
        returnOptions.Options.ValidateLength = TRUE;
    }
    if (ValidationOptions.Options.ValidateDirectoryStructureExists)
    {
        if (!ValidationOptions.Options.IsFilePath)
        {
            returnOptions.Options.ValidateDirectoryStructureExists 
                = DirectoryExists(Path);
        }

        if (ValidationOptions.Options.IsFilePath && filePathCopy)
        {
            returnOptions.Options.ValidateDirectoryStructureExists 
                = DirectoryExists(filePathCopy);
        }
    }
    if (ValidationOptions.Options.ValidateIsCurrentWorkingDirectory)
    {
        if (!cwd)
        {
            printf("[WARNING] Unable to extract the current working directory, cannot validate base path\n");
        }
        else
        {
            if (!ValidationOptions.Options.IsFilePath)
            {
                returnOptions.Options.ValidateIsCurrentWorkingDirectory
                    = _AreBasePathsEqual(Path, cwd);
            }

            if (ValidationOptions.Options.IsFilePath && filePathCopy)
            {
                returnOptions.Options.ValidateIsCurrentWorkingDirectory
                    = _AreBasePathsEqual(filePathCopy, cwd);
            }
        }
    }
    if (ValidationOptions.Options.ValidateIsCurrentWorkingDirectoryUserPath)
    {
        if (!UserPath)
        {
            printf("[WARNING] No UserPath provided to validate ValidateIsCurrentWorkingDirectoryUserPath option\n");
        }
        else {
            if (!ValidationOptions.Options.IsFilePath)
            {
                returnOptions.Options.ValidateIsCurrentWorkingDirectoryUserPath =
                    _AreBasePathsEqual(Path, UserPath);
            }

            if (ValidationOptions.Options.IsFilePath && filePathCopy)
            {
                returnOptions.Options.ValidateIsCurrentWorkingDirectoryUserPath =
                    _AreBasePathsEqual(filePathCopy, UserPath);
            }
        }
    }
    if (ValidationOptions.Options.ValidateFileExists && ValidationOptions.Options.IsFilePath)
    {
        returnOptions.Options.ValidateFileExists = 
            FileExists(Path);
    }
    if (ValidationOptions.Options.ValidateFileIsNotSystemFile && ValidationOptions.Options.IsFilePath)
    {
        returnOptions.Options.ValidateFileIsNotSystemFile = 
            AccessibleFile(Path);
    }
    if (ValidationOptions.Options.ValidatePathDoesNotContainWildcards)
    {
        returnOptions.Options.ValidatePathDoesNotContainWildcards = 
            _ValidateNoWildcards(Path, _RESTRICTED_CHARS_PATH);
    }

cleanup:

    if (filePathCopy)
    {
        free(filePathCopy);
    }

    if (cwd)
    {
        free(cwd);
    }

    ValidationResult->OptionsRaw = returnOptions.OptionsRaw;
    
    return status;
}

_Use_decl_annotations_
NTSTATUS
ValidateString(
    _In_ PCHAR String,
    _In_opt_ DWORD StringSize,
    _In_ STRING_VALIDATION_OPTIONS ValidationOptions,
    _Out_ STRING_VALIDATION_OPTIONS *ValidationResult
)
{
    if (!String)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    STRING_VALIDATION_OPTIONS returnOptions = { 0 };
    DWORD size;

    if (ValidationOptions.Options.ValidateLength)
    {
        size = (DWORD)strnlen_s(String, MAX_PATH);
        if (size < MAX_PATH && size == StringSize)
        {
            returnOptions.Options.ValidateLength = TRUE;
        }
    }
    if (ValidationOptions.Options.ValidatePathDoesNotContainWildcards)
    {
        returnOptions.Options.ValidatePathDoesNotContainWildcards = 
            _ValidateNoWildcards(String, _RESTRICTED_CHARS_STRINGS);
    }

    ValidationResult->OptionsRaw = returnOptions.OptionsRaw;
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ValidatePassword(
    _In_ PCHAR Password,
    _In_opt_ DWORD PasswordSize,
    _In_ PASSWORD_VALIDATION_OPTIONS ValidationOptions,
    _Out_ PASSWORD_VALIDATION_OPTIONS *ValidationResult
)
{
    if (!Password)
        return STATUS_INVALID_PARAMETER_1;

    PASSWORD_VALIDATION_OPTIONS returnOptions = { 0 };
    DWORD size;
    size = (DWORD)strnlen_s(Password, MAX_PATH);

    if (size == 0 || size == MAX_PATH)
        return STATUS_ABANDONED;

    if (ValidationOptions.Options.ValidateLength)
    {
        if (size < MAX_PATH && size == PasswordSize)
        {
            returnOptions.Options.ValidateLength = TRUE;
        }
    }
    if (ValidationOptions.Options.ValidatePathDoesNotContainWildcards)
    {
        returnOptions.Options.ValidatePathDoesNotContainWildcards =
            _ValidateNoWildcards(Password, _RESTRICTED_CHARS_STRINGS);
    }
    if (ValidationOptions.Options.ValidateLengthAndSpecialCharacters)
    {
        BOOL hasChar = FALSE;
        BOOL hasDigit = FALSE;

        for (DWORD idx = 0; idx < size; idx++)
        {
            if (isalpha(Password[idx]))
                hasChar = TRUE;

            if (isdigit(Password[idx]))
                hasDigit = TRUE;
        }

        if (size >= PASSWD_MIN_SIZE && hasChar && hasDigit)
            returnOptions.Options.ValidateLengthAndSpecialCharacters = TRUE;
    }

    ValidationResult->OptionsRaw = returnOptions.OptionsRaw;
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
DWORD
_HashBuffer(
    _In_ PBYTE Buffer, 
    _In_ DWORD BufferSize, 
    _In_ BYTE Seed
)
{
    DWORD hash = 0;
    for (DWORD bufferIdx = 0; bufferIdx < BufferSize; bufferIdx++)
    {
        hash += ((DWORD)(Buffer[bufferIdx] ^ Seed));
        hash <<= (bufferIdx ^ Seed);
    }

    return hash;
}

_Use_decl_annotations_
BOOL
CheckUserCredentialsExist(
    _In_ FILE *PasswdFile,
    _In_ BOOL CheckPassword,
    _In_ DWORD UsernameHash,
    _In_ DWORD PasswordHash
)
{
    DWORD username = 0;
    DWORD password = 0;
    while (fscanf_s(PasswdFile, "%u %u\n", &username, &password) != EOF)
    {
        if ((!CheckPassword) && (username == UsernameHash))
            return TRUE;
        if (CheckPassword && (username == UsernameHash) && (password == PasswordHash))
            return TRUE;
    }
    return FALSE;

}

_Use_decl_annotations_
NTSTATUS
HashCredentials(
    _In_  PCHAR Username,
    _In_  DWORD UsernameLength,
    _In_  PCHAR Password,
    _In_  DWORD PasswordLength,
    _Out_ DWORD *UsernameHash,
    _Out_ DWORD *PasswordHash
)
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD passwordHash, usernameHash;

    StringToLower(Username, UsernameLength);

    usernameHash = _HashBuffer((PBYTE)Username, UsernameLength, USERNAME_SEED);
    passwordHash = _HashBuffer((PBYTE)Password, PasswordLength, PASSWD_SEED);
    *UsernameHash = usernameHash;
    *PasswordHash = passwordHash;

    return status;
}
