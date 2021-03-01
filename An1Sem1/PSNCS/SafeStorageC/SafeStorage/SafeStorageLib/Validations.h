#include "includes.h"

typedef union _PATH_VALIDATION_OPTIONS {
        struct {
            int ValidateLength : 1;
            int ValidateIsCurrentWorkingDirectory : 1;
            int ValidateIsCurrentWorkingDirectoryUserPath : 1;
            int ValidateDirectoryStructureExists : 1;
            int ValidateFileExists : 1;
            int ValidateFileIsNotSystemFile : 1;
            int ValidatePathDoesNotContainWildcards : 1;
            int IsFilePath : 1;
        } Options;
        int OptionsRaw;
} PATH_VALIDATION_OPTIONS;

typedef union _STRING_VALIDATION_OPTIONS {
    struct {
        int ValidateLength : 1;
        int ValidatePathDoesNotContainWildcards : 1;
    } Options;
    int OptionsRaw;
} STRING_VALIDATION_OPTIONS;

typedef union _PASSWORD_VALIDATION_OPTIONS {
    struct {
        int ValidateLength : 1;
        int ValidatePathDoesNotContainWildcards : 1;
        int ValidateLengthAndSpecialCharacters : 1;
    } Options;
    int OptionsRaw;
} PASSWORD_VALIDATION_OPTIONS;


_Use_decl_annotations_
NTSTATUS
ValidatePath(
    _In_ PCHAR Path,
    _In_opt_ DWORD PathSize,
    _In_ PATH_VALIDATION_OPTIONS ValidationOptions,
    _In_opt_ PCHAR UserPath,
    _Out_ PATH_VALIDATION_OPTIONS *ValidationResult
);

_Use_decl_annotations_
NTSTATUS
ValidateString(
    _In_ PCHAR String,
    _In_opt_ DWORD StringSize,
    _In_ STRING_VALIDATION_OPTIONS ValidationOptions,
    _Out_ STRING_VALIDATION_OPTIONS *ValidationResult
);

_Use_decl_annotations_
NTSTATUS
ValidatePassword(
    _In_ PCHAR Password,
    _In_opt_ DWORD PasswordSize,
    _In_ PASSWORD_VALIDATION_OPTIONS ValidationOptions,
    _Out_ PASSWORD_VALIDATION_OPTIONS *ValidationResult
);

_Use_decl_annotations_
BOOL
CheckUserCredentialsExist(
    _In_ FILE *PasswdFile,
    _In_ BOOL CheckPassword,
    _In_ DWORD UsernameHash,
    _In_ DWORD PasswordHash
);

_Use_decl_annotations_
NTSTATUS
HashCredentials(
    _In_  PCHAR Username,
    _In_  DWORD UsernameLength,
    _In_  PCHAR Password,
    _In_  DWORD PasswordLength,
    _Out_ DWORD *UsernameHash,
    _Out_ DWORD *PasswordHash
);
