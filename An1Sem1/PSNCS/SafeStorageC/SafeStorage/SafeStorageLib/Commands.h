#ifndef _COMMANDS_H_
#define _COMMANDS_H_ 

#include "includes.h"
#include "Globals.h"

// Handles the "register" command
// Creates the user with Username and Password specified
//      * Creates the user's subdirectory in the %APPDIR%\users directory
// Fails if the user is already registered
// This command is available only if no user is currently logged in (it must fail otherwise)
// It returns STATUS_SUCCESS if everything succeeded or a proper error status otherwise.
_Must_inspect_result_
NTSTATUS
SafeStorageHandleRegister(
    _In_reads_bytes_opt_(UsernameLength) PCHAR Username,
    _In_ USHORT UsernameLength,
    _In_reads_bytes_opt_(PasswordLength) PCHAR Password,
    _In_ USHORT PasswordLength
);

// Handles the "login" command
// Logins the user with Username and Password provided
// Fails if the user doesn't exists or the password is incorrect
// This command is available only if no user is currently logged in (it must fail otherwise)
// It returns STATUS_SUCCESS if everything succeeded or a proper error status otherwise.
_Must_inspect_result_
NTSTATUS
SafeStorageHandleLogin(
    _In_reads_bytes_opt_(UsernameLength) PCHAR Username,
    _In_ USHORT UsernameLength,
    _In_reads_bytes_opt_(PasswordLength) PCHAR Password,
    _In_ USHORT PasswordLength
);

// Handles the "logout" command
// Fails if no user is currently logged in
// It returns STATUS_SUCCESS if everything succeeded or a proper error status otherwise.
_Must_inspect_result_
NTSTATUS
SafeStorageHandleLogout();

// Handles the "store" command
// Fails if no user is currently logged in
//      * Copies the file from SourceFile to %APPDIR%\users\<current_logged_user_username>\SubmissionName
// It returns STATUS_SUCCESS if everything succeeded or a proper error status otherwise.
_Must_inspect_result_
NTSTATUS
SafeStorageHandleStore(
    _In_reads_bytes_opt_(SubmissionNameLength) PCHAR SubmissionName,
    _In_ USHORT SubmissionNameLength,
    _In_reads_bytes_opt_(SourceFileLength) PCHAR SourceFile,
    _In_ USHORT SourceFileLength
);

// Handles the "retrieve" command
// Fails if no user is currently logged in
//      * Copies the file from %APPDIR%\users\<current_logged_user_username>\SubmissionName to DestinationFile
// It returns STATUS_SUCCESS if everything succeeded or a proper error status otherwise.
_Must_inspect_result_
NTSTATUS
SafeStorageHandleRetrieve(
    _In_reads_bytes_opt_(SubmissionNameLength) PCHAR SubmissionName,
    _In_ USHORT SubmissionNameLength,
    _In_reads_bytes_opt_(DestinationFileLength) PCHAR DestinationFile,
    _In_ USHORT DestinationFileLength
);

#endif //_COMMANDS_H_