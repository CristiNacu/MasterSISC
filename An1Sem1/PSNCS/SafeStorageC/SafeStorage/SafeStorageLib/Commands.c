#include "Commands.h"
#include "Common.h"
#include "FileProcessing.h"
#include "Validations.h"
#include "includes.h"
#include <windows.h>
#include <time.h> 

#define TIMEOUT_IN_SECONDS 5

CHAR _RELATIVE_FOLDER[] = "users";
CHAR _RELATIVE_LOGIN_FILE[] = "login";

SECURITY_ATTRIBUTES directoryAttributes = {
    .nLength = sizeof(SECURITY_ATTRIBUTES),
    .lpSecurityDescriptor = NULL,
    .bInheritHandle = FALSE
};


NTSTATUS
_GetWorkingPath(
    _Out_ PCHAR *CurrentDirectory
) {

    NTSTATUS status = STATUS_SUCCESS;
    PCHAR currentDirectory = NULL;


    status = GetCurrentWorkingDirectory(&currentDirectory);
    if (!NT_SUCCESS(status))
    {
        printf_s("_DiscoverCurrentWorkingDirectory returned error\n");
        goto cleanup;
    }

    status = ConcatenatePath(currentDirectory, _RELATIVE_FOLDER, TRUE);
    if (!NT_SUCCESS(status))
    {
        printf_s("ConcatenatePath returned error\n");
        goto cleanup;
    }

    if (!DirectoryExists(currentDirectory))
    {
        BOOL createdSuccessful = CreateDirectoryA(
            currentDirectory,
            &directoryAttributes
        );
        if (!createdSuccessful)
        {
            status = STATUS_FILE_HANDLE_REVOKED;
            goto cleanup;
        }
    }

    *CurrentDirectory = currentDirectory;
    return status;

cleanup:

    if (currentDirectory)
    {
        free(currentDirectory);
    }

    *CurrentDirectory = NULL;
    return status;
}

NTSTATUS
_CreateUserPath(
    _Inout_ PCHAR *CurrentDirectory,
    _In_ PCHAR Username,
    _In_ BOOL WarningIfPathDoesntExist
)
{
    NTSTATUS status;

    status = ConcatenatePath(*CurrentDirectory, Username, TRUE);
    if (!NT_SUCCESS(status))
    {
        printf_s("ConcatenatePath returned error\n");
        goto cleanup;
    }

    if (!DirectoryExists(*CurrentDirectory))
    {
        if (WarningIfPathDoesntExist)
        {
            printf_s("[WARNING] Path for the current user has been removed. All data was lost!\n");
        }

        BOOL createdSuccessful = CreateDirectoryA(
            *CurrentDirectory,
            &directoryAttributes
        );
        if (!createdSuccessful)
        {
            status = STATUS_FILE_HANDLE_REVOKED;
            goto cleanup;
        }
    }

cleanup:
    return status;
}

_Use_decl_annotations_
NTSTATUS
SafeStorageHandleRegister(
    _In_reads_bytes_opt_(UsernameLength) PCHAR Username,
    _In_ USHORT UsernameLength,
    _In_reads_bytes_opt_(PasswordLength) PCHAR Password,
    _In_ USHORT PasswordLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PCHAR currentDirectory = NULL;
    PCHAR passwordFile = NULL;
    
    DWORD passwordHash = 0;
    DWORD usernameHash = 0;

    PCHAR passwordCopy = NULL;
    PCHAR usernameCopy = NULL;
    
    USHORT usernameLength = UsernameLength;
    USHORT passwordLength = PasswordLength;

    FILE *passwdFile = NULL;
    BOOL userExists = FALSE;

    PASSWORD_VALIDATION_OPTIONS passwordValidationOptions = { 0 };
    STRING_VALIDATION_OPTIONS stringValidationOptions = { 0 };
         
    if (gGlobalData.UserAuthenticated)
    {
        printf_s("An user is already authenticated\n");
        return STATUS_ALREADY_COMMITTED;
    }

    status = ValidateString(
        Username,
        usernameLength,
        (STRING_VALIDATION_OPTIONS) {
            .Options.ValidateLength = TRUE,
            .Options.ValidatePathDoesNotContainWildcards = TRUE
        },
        &stringValidationOptions
    );
    if (!NT_SUCCESS(status))
    {
        printf("Invalid username, try again\n");
        status = STATUS_INVALID_PARAMETER_1;
        goto cleanup;
    }
    if (!stringValidationOptions.Options.ValidateLength)
    {
        printf("Invalid username size. Please try again.");
        status = STATUS_INVALID_PARAMETER_1;
        goto cleanup;
    }
    if (!stringValidationOptions.Options.ValidatePathDoesNotContainWildcards)
    {
        printf("Username contains wildcard characters. Hacky.");
        status = STATUS_INVALID_PARAMETER_1;
        goto cleanup;
    }

    status = ValidatePassword(
        Password,
        passwordLength,
        (PASSWORD_VALIDATION_OPTIONS) {
            .Options.ValidateLength = TRUE,
            .Options.ValidatePathDoesNotContainWildcards = FALSE,
            .Options.ValidateLengthAndSpecialCharacters = TRUE
        },
        &passwordValidationOptions
    );
    if (!NT_SUCCESS(status))
    {
        printf("Invalid password, try again\n");
        status = STATUS_INVALID_PARAMETER_3;
        goto cleanup;
    }
    if (!passwordValidationOptions.Options.ValidateLength)
    {
        printf("Invalid password size. Please try again.");
        status = STATUS_INVALID_PARAMETER_3;
        goto cleanup;
    }
    if (!passwordValidationOptions.Options.ValidateLengthAndSpecialCharacters)
    {
        printf("Password must contain at least one character and one digit and must be at least 8 characters long.\n");
        status = STATUS_INVALID_PARAMETER_3;
        goto cleanup;
    }

    status = CopyString(&usernameCopy, Username, usernameLength);
    if (!NT_SUCCESS(status))
    {
        printf("CopyString exited with status %x\n", status);
        goto cleanup;
    }

    status = CopyString(&passwordCopy, Password, passwordLength);
    if (!NT_SUCCESS(status))
    {
        printf("CopyString exited with status %x\n", status);
        goto cleanup;
    }

    status = HashCredentials(
        usernameCopy,
        usernameLength,
        passwordCopy,
        passwordLength,
        &usernameHash,
        &passwordHash
    );
    if (!NT_SUCCESS(status))
    {
        printf_s("HashCredentials returned error\n");
        goto cleanup;
    }

    status = _GetWorkingPath(
        &currentDirectory
    );
    if (!NT_SUCCESS(status))
    {
        printf_s("_GetWorkingPath returned error\n");
        goto cleanup;
    }

    status = CopyPath(&passwordFile, currentDirectory);
    if (!NT_SUCCESS(status))
    {
        printf_s("ConcatenatePath returned error\n");
        goto cleanup;
    }

    status = ConcatenatePath(passwordFile, _RELATIVE_LOGIN_FILE, TRUE);
    if (!NT_SUCCESS(status))
    {
        printf_s("ConcatenatePath returned error\n");
        goto cleanup;
    }

    if (!FileExists(passwordFile))
    {
        HANDLE fileHandle = CreateFileA(
            passwordFile,
            (GENERIC_READ | GENERIC_WRITE),
            0,
            NULL,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN,
            NULL
        );
        if (fileHandle == INVALID_HANDLE_VALUE)
        {
            status = STATUS_FILE_INVALID;
            goto cleanup;
        }

        CloseHandle(fileHandle);
    }
    else
    {
        passwdFile = fopen(passwordFile, "r+");
        if (passwdFile == NULL)
        {
            printf_s("fopen returned error\n");
            goto cleanup;
        }

        userExists = CheckUserCredentialsExist(
            passwdFile,
            FALSE,
            usernameHash,
            0
        );

        fclose(passwdFile);
    }
    
    if (userExists)
    {
        printf_s("Username already present in the database. Use another\n");
        status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    passwdFile = fopen(passwordFile, "a+");
    if (passwdFile == NULL)
    {
        printf_s("fopen returned error\n");
        goto cleanup;
    }

    fprintf_s(passwdFile, "%u %u\n", usernameHash, passwordHash);

    fclose(passwdFile);
    passwdFile = NULL;

    status = _CreateUserPath(&currentDirectory, Username, FALSE);
    if (!NT_SUCCESS(status))
    {
        printf_s("_CreateUserPath failed!\n");
        goto cleanup;
    }

cleanup:
    if (usernameCopy)
    {
        free(usernameCopy);
    }
    if (passwordCopy)
    {
        free(passwordCopy);
    }
    if (currentDirectory)
    {
        free(currentDirectory);
    }
    if (passwordFile)
    {
        free(passwordFile);
    }
    if (passwdFile)
    {
        fclose(passwdFile);
    }

    return status;
}

_Use_decl_annotations_
NTSTATUS
SafeStorageHandleLogin(
    _In_reads_bytes_opt_(UsernameLength) PCHAR Username,
    _In_ USHORT UsernameLength,
    _In_reads_bytes_opt_(PasswordLength) PCHAR Password,
    _In_ USHORT PasswordLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PCHAR currentDirectory = NULL;
    PCHAR passwordFile = NULL;

    FILE *passwdFile = NULL;
    BOOL userExists = FALSE;

    DWORD passwordHash = 0;
    DWORD usernameHash = 0;

    USHORT usernameLength = UsernameLength;
    USHORT passwordLength = PasswordLength;

    PCHAR usernameCopy = NULL;
    PCHAR passwordCopy = NULL;

    PASSWORD_VALIDATION_OPTIONS passwordValidationOptions = { 0 };
    STRING_VALIDATION_OPTIONS stringValidationOptions = { 0 };

    if (gGlobalData.UserAuthenticated)
    {
        printf_s("An user is already authenticated\n");
        return STATUS_ALREADY_COMMITTED;
    }

    if (gGlobalData.Timeout != 0)
    {
        if (time(NULL) - gGlobalData.Timeout > TIMEOUT_IN_SECONDS)
            gGlobalData.Timeout = 0;
        else
        {
            printf("You are timeout-ed. %lld seconds left", TIMEOUT_IN_SECONDS - time(NULL) + gGlobalData.Timeout);
            return STATUS_TIMEOUT;
        }
    }

    status = ValidateString(
        Username,
        usernameLength,
        (STRING_VALIDATION_OPTIONS) {
            .Options.ValidateLength = TRUE,
            .Options.ValidatePathDoesNotContainWildcards = FALSE
        },
        &stringValidationOptions
    );
    if (!NT_SUCCESS(status))
    {
        printf("Invalid username, try again\n");
        status = STATUS_INVALID_PARAMETER_1;
        goto cleanup;
    }
    if (!stringValidationOptions.Options.ValidateLength)
    {
        printf("Invalid username size. Please try again.");
        status = STATUS_INVALID_PARAMETER_1;
        goto cleanup;
    }

    status = ValidatePassword(
        Password,
        passwordLength,
        (PASSWORD_VALIDATION_OPTIONS) {
            .Options.ValidateLength = TRUE,
            .Options.ValidatePathDoesNotContainWildcards = FALSE,
            .Options.ValidateLengthAndSpecialCharacters = FALSE
        },
        &passwordValidationOptions
    );
    if (!NT_SUCCESS(status))
    {
        printf("Invalid password, try again\n");
        status = STATUS_INVALID_PARAMETER_3;
        goto cleanup;
    }
    if (!passwordValidationOptions.Options.ValidateLength)
    {
        printf("Invalid password size. Please try again.");
        status = STATUS_INVALID_PARAMETER_3;
        goto cleanup;
    }

    status = CopyString(&usernameCopy, Username, usernameLength);
    if (!NT_SUCCESS(status))
    {
        printf("CopyString exited with status %x\n", status);
        goto cleanup;
    }

    status = CopyString(&passwordCopy, Password, passwordLength);
    if (!NT_SUCCESS(status))
    {
        printf("CopyString exited with status %x\n", status);
        goto cleanup;
    }

    status = HashCredentials(
        usernameCopy,
        usernameLength,
        passwordCopy,
        passwordLength,
        &usernameHash,
        &passwordHash
    );
    if (!NT_SUCCESS(status))
    {
        printf_s("HashCredentials returned error\n");
        goto cleanup;
    }

    status = _GetWorkingPath(
        &currentDirectory
    );
    if (!NT_SUCCESS(status))
    {
        printf_s("_GetWorkingPath returned error\n");
        goto cleanup;
    }

    status = CopyPath(&passwordFile, currentDirectory);
    if (!NT_SUCCESS(status))
    {
        printf_s("CopyPath returned error\n");
        goto cleanup;
    }

    status = ConcatenatePath(passwordFile, _RELATIVE_LOGIN_FILE, TRUE);
    if (!NT_SUCCESS(status))
    {
        printf_s("ConcatenatePath returned error\n");
        goto cleanup;
    }

    if (!FileExists(passwordFile))
    {
        status = STATUS_FILE_NOT_AVAILABLE;
        goto cleanup;
    }

    passwdFile = fopen(passwordFile, "r+");
    if (passwdFile == NULL)
    {
        printf_s("fopen returned error\n");
        goto cleanup;
    }

    userExists = CheckUserCredentialsExist(
        passwdFile,
        TRUE,
        usernameHash,
        passwordHash
    );

    fclose(passwdFile);
    passwdFile = NULL;

    if (userExists)
    {
        status = _CreateUserPath(&currentDirectory, usernameCopy, TRUE);
        if (!NT_SUCCESS(status))
        {
            printf_s("_CreateUserPath returned error\n");
            goto cleanup;
        }

        CopyPath(&gGlobalData.UserPath, currentDirectory);
        gGlobalData.UserAuthenticated = TRUE;

        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

cleanup:
    if (!NT_SUCCESS(status))
    {
        printf("Unkown username or password. Please try again\n");
    }
    else
    {
        printf_s("User authenticated ok.\n");
    }
    if (usernameCopy)
    {
        free(usernameCopy);
    }

    if (passwordCopy)
    {
        free(passwordCopy);
    }

    if (currentDirectory)
    {
        free(currentDirectory);
    }

    if (passwordFile)
    {
        free(passwordFile);
    }
    
    if (passwdFile)
    {
        fclose(passwdFile);
    }

    if (!NT_SUCCESS(status))
    {
        gGlobalData.UserAuthenticated = FALSE;
        if (gGlobalData.UserPath)
        {
            free(gGlobalData.UserPath);
            gGlobalData.UserPath = NULL;
        }

        gGlobalData.Retries++;
        if (gGlobalData.Retries == 3)
        {
            printf("Too many failed login trials. You are timeout-ed for %d seconds\n", TIMEOUT_IN_SECONDS);
            gGlobalData.Retries = 0;
            gGlobalData.Timeout = time(NULL);
        }

    }

    return status;
}

_Use_decl_annotations_
NTSTATUS
SafeStorageHandleLogout()
{
    if (!gGlobalData.UserAuthenticated)
    {
        printf_s("No authenticated user\n");
        return STATUS_NOTHING_TO_TERMINATE;
    }

    gGlobalData.UserAuthenticated = FALSE;
    if (!gGlobalData.UserPath)
    {
        printf("[WARNING] No username available, odd\n");
    }
    else
    {
        free(gGlobalData.UserPath);
        gGlobalData.UserPath = NULL;
    }
    printf_s("Logout successful\n");

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
SafeStorageHandleStore(
    _In_reads_bytes_opt_(SubmissionNameLength) PCHAR SubmissionName,
    _In_ USHORT SubmissionNameLength,
    _In_reads_bytes_opt_(SourceFileLength) PCHAR SourceFile,
    _In_ USHORT SourceFileLength
)
{
    NTSTATUS status;
    PCHAR currentDirectory = NULL;
    PCHAR sourceFile = NULL;
    PCHAR submissionName = NULL;
    PCHAR fileName = NULL;
    PATH_VALIDATION_OPTIONS outputValidations;
    STRING_VALIDATION_OPTIONS stringValidation;

    if (!gGlobalData.UserAuthenticated)
    {
        printf_s("No user authenticated. Use login to authenticate\n");
        return STATUS_ACCESS_DENIED;
    }
    if (!gGlobalData.UserPath)
    {
        printf_s("An error occured during login. Please logout and login again.\n");
        return STATUS_ACCESS_DENIED;
    }

    status = ValidateString(
        SubmissionName,
        SubmissionNameLength,
        (STRING_VALIDATION_OPTIONS) {
            .Options.ValidateLength = TRUE,
            .Options.ValidatePathDoesNotContainWildcards = TRUE
        },
        &stringValidation
    );
    if (!NT_SUCCESS(status))
    {
        printf("ValidateString returned status %x\n", status);
        goto cleanup;
    }
    if (!stringValidation.Options.ValidateLength)
    {
        printf("The submission name has invalid length.\n");
        return STATUS_INVALID_PARAMETER_3;
    }
    if (!stringValidation.Options.ValidatePathDoesNotContainWildcards)
    {
        printf("Submission name contains unacceptable wildcard characters.\n");
        return STATUS_INVALID_PARAMETER_3;
    }

    status = ValidatePath(
        SourceFile,
        SourceFileLength,
        (PATH_VALIDATION_OPTIONS){
            .Options.ValidateLength = TRUE,
            .Options.ValidateIsCurrentWorkingDirectory = TRUE,
            .Options.ValidateIsCurrentWorkingDirectoryUserPath = TRUE,
            .Options.ValidateDirectoryStructureExists = FALSE,
            .Options.ValidateFileExists = TRUE,
            .Options.ValidateFileIsNotSystemFile = TRUE,
            .Options.ValidatePathDoesNotContainWildcards = TRUE,
            .Options.IsFilePath = TRUE
        },
        gGlobalData.UserPath,
        &outputValidations
    );
    if (!NT_SUCCESS(status))
    {
        printf("ValidatePath returned status %x\n", status);
        goto cleanup;
    }
    if (!outputValidations.Options.ValidateLength)
    {
        printf("The path has invalid length. Please provide a path with at most %d characters.\n", MAX_PATH);
        return STATUS_INVALID_PARAMETER_3;
    }
    if (!outputValidations.Options.ValidateFileExists)
    {
        printf("The file doesn't exist. Please provide a path to an existing file.\n");
        return STATUS_INVALID_PARAMETER_3;
    }
    if (!outputValidations.Options.ValidateFileIsNotSystemFile)
    {
        printf("The path points to a restricted file. Please provide a path to a non-system file.\n");
        return STATUS_INVALID_PARAMETER_3;
    }
    if (outputValidations.Options.ValidateIsCurrentWorkingDirectory && 
        (!outputValidations.Options.ValidateIsCurrentWorkingDirectoryUserPath))
    {
        printf("The path points to another user's files. Please provide your own files / files in your computer.\n");
        return STATUS_INVALID_PARAMETER_3;
    }
    if (!outputValidations.Options.ValidatePathDoesNotContainWildcards)
    {
        printf("The path contains wildcards. This seems hacky.\n");
        return STATUS_INVALID_PARAMETER_3;
    }

    status = CopyString(&submissionName, SubmissionName, SubmissionNameLength);
    if (!NT_SUCCESS(status))
    {
        printf_s("CopyPath returned error\n");
        goto cleanup;
    }

    status = CopyPath(&sourceFile, SourceFile);
    if (!NT_SUCCESS(status))
    {
        printf_s("CopyPath returned error\n");
        goto cleanup;
    }

    StringToLower(submissionName, SubmissionNameLength);

    status = CopyPath(&currentDirectory, gGlobalData.UserPath);
    if (!NT_SUCCESS(status))
    {
        printf_s("CopyPath returned error\n");
        goto cleanup;
    }

    status = ConcatenatePath(currentDirectory, submissionName, TRUE);
    if (!NT_SUCCESS(status))
    {
        printf_s("ConcatenatePath returned error\n");
        goto cleanup;
    }

    if (!DirectoryExists(currentDirectory))
    {
        status = CreateDirectoryStructure(
            currentDirectory,
            &directoryAttributes
        );
        if (!NT_SUCCESS(status))
        {
            status = STATUS_FILE_HANDLE_REVOKED;
            goto cleanup;
        }
    }

    status = GetFileName(sourceFile, &fileName);
    if (!NT_SUCCESS(status))
    {
        printf_s("GetFileName failed");
        goto cleanup;
    }

    status = ConcatenatePath(currentDirectory, fileName, TRUE);
    if (!NT_SUCCESS(status))
    {
        printf_s("ConcatenatePath returned error\n");
        goto cleanup;
    }

    status = AddFileProcessingJob(
        sourceFile,
        currentDirectory
    );
    if (!NT_SUCCESS(status))
    {
        printf_s("AddFileProcessingJob returned error %x\n", status);
        goto cleanup;
    }
    
cleanup:
    if (sourceFile)
    {
        free(sourceFile);
        sourceFile = NULL;
    }
    if (currentDirectory)
    {
        free(currentDirectory);
        currentDirectory = NULL;
    }
    if (submissionName)
    {
        free(submissionName);
        submissionName = NULL;
    }
    return status;
}

_Use_decl_annotations_
NTSTATUS
SafeStorageHandleRetrieve(
    _In_reads_bytes_opt_(SubmissionNameLength) PCHAR SubmissionName,
    _In_ USHORT SubmissionNameLength,
    _In_reads_bytes_opt_(DestinationFileLength) PCHAR DestinationFile,
    _In_ USHORT DestinationFileLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PCHAR currentDirectory = NULL;
    PCHAR fileName = NULL;
    PCHAR destinationPath = NULL;
    PCHAR submissionName = NULL;
    PATH_VALIDATION_OPTIONS outputValidations;
    STRING_VALIDATION_OPTIONS stringOutputValidation;

    if (!gGlobalData.UserAuthenticated)
    {
        printf_s("No user authenticated. Use login to authenticate\n");
        return STATUS_ACCESS_DENIED;
    }
    if (!gGlobalData.UserPath)
    {
        printf_s("An error occured during login. Please logout and login again.\n");
        return STATUS_ACCESS_DENIED;
    }

    status = ValidateString(
        SubmissionName,
        SubmissionNameLength,
        (STRING_VALIDATION_OPTIONS) {
        .Options.ValidateLength = TRUE,
        .Options.ValidatePathDoesNotContainWildcards = TRUE
        },
        &stringOutputValidation
    );
    if (!NT_SUCCESS(status))
    {
        printf("Invalid submission name. Please provide a valid submission name.\n");
        goto cleanup;
    }
    if (!stringOutputValidation.Options.ValidateLength)
    {
        printf("Invalid submission length. Please provide a valid submission name.\n");
        status = STATUS_INVALID_PARAMETER_1;
        goto cleanup;
    }
    if (!stringOutputValidation.Options.ValidatePathDoesNotContainWildcards)
    {
        printf("Submission name contains wildcards. Hacky.\n");
        status = STATUS_INVALID_PARAMETER_1;
        goto cleanup;
    }

    status = ValidatePath(
        DestinationFile,
        DestinationFileLength,
        (PATH_VALIDATION_OPTIONS) {
            .Options.ValidateLength = TRUE,
            .Options.ValidateIsCurrentWorkingDirectory = TRUE,
            .Options.ValidateIsCurrentWorkingDirectoryUserPath = TRUE,
            .Options.ValidateDirectoryStructureExists = TRUE,
            .Options.ValidateFileExists = FALSE,
            .Options.ValidateFileIsNotSystemFile = FALSE,
            .Options.ValidatePathDoesNotContainWildcards = TRUE,
            .Options.IsFilePath = TRUE
            },
            gGlobalData.UserPath,
            &outputValidations
    );
    if (!NT_SUCCESS(status))
    {
        printf("Please provide a valid path.\n");
        goto cleanup;
    }
    if (!outputValidations.Options.ValidateLength)
    {
        printf("The path has invalid length. Please provide a path with at most %d characters.\n", MAX_PATH);
        status = STATUS_INVALID_PARAMETER_3;
        goto cleanup;
    }
    if (outputValidations.Options.ValidateIsCurrentWorkingDirectory)
    {
        printf("Due to security concerns you cannot copy the file inside the application's working directory!\n");
        status = STATUS_INVALID_PARAMETER_3;
        goto cleanup;
    }
    if (!outputValidations.Options.ValidatePathDoesNotContainWildcards)
    {
        printf("The path contains wildcards. This seems hacky.\n");
        status = STATUS_INVALID_PARAMETER_3;
        goto cleanup;
    }
    if (!outputValidations.Options.ValidateDirectoryStructureExists)
    {
        status = CopyPath(&destinationPath, DestinationFile);
        if (!NT_SUCCESS(status))
        {
            printf_s("CopyPath returned error\n");
            goto cleanup;
        }

        status = GetFileDirectoryPath(destinationPath);
        if (!NT_SUCCESS(status))
        {
            printf_s("GetFileDirectoryPath returned error\n");
            goto cleanup;
        }

        status = CreateDirectoryStructure(
            destinationPath,
            &directoryAttributes
        );
        if (!NT_SUCCESS(status))
        {
            printf("Failed to create directory path %s with error %x\n", destinationPath, GetLastError());
            status = STATUS_FILE_HANDLE_REVOKED;
            goto cleanup;
        }

        free(destinationPath);
        destinationPath = NULL;
    }
    
    status = CopyString(
        &submissionName, 
        SubmissionName, 
        SubmissionNameLength
    );
    if (!NT_SUCCESS(status))
    {
        printf_s("CopyString returned error\n");
        goto cleanup;
    }

    StringToLower(submissionName, SubmissionNameLength);

    status = CopyPath(&destinationPath, DestinationFile);
    if (!NT_SUCCESS(status))
    {
        printf_s("CopyPath returned error\n");
        goto cleanup;
    }
    status = CopyPath(&currentDirectory, gGlobalData.UserPath);
    if (!NT_SUCCESS(status))
    {
        printf_s("CopyPath returned error\n");
        goto cleanup;
    }

    status = ConcatenatePath(currentDirectory, submissionName, TRUE);
    if (!NT_SUCCESS(status))
    {
        printf_s("ConcatenatePath returned error\n");
        goto cleanup;
    }

    status = GetFileName(destinationPath, &fileName);
    if (!NT_SUCCESS(status))
    {
        printf_s("GetFileName failed\n");
        goto cleanup;
    }

    status = ConcatenatePath(currentDirectory, fileName, TRUE);
    if (!NT_SUCCESS(status))
    {
        printf_s("ConcatenatePath returned error\n");
        goto cleanup;
    }

    if (!AccessibleFile(currentDirectory))
    {
        printf("File %s does not exist for submision %s!\n", fileName, submissionName);
        goto cleanup;
    }

    status = AddFileProcessingJob(
        currentDirectory,
        destinationPath
    );
    if (!NT_SUCCESS(status))
    {
        printf_s("AddFileProcessingJob returned error %x\n", status);
        goto cleanup;
    }

cleanup:
    if (currentDirectory)
    {
        free(currentDirectory);
        currentDirectory = NULL;
    }
    if (destinationPath)
    {
        free(destinationPath);
        destinationPath = NULL;
    }
    if (submissionName)
    {
        free(submissionName);
        submissionName = NULL;
    }
    return status;
}