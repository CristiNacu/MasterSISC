#ifndef _GLOBALS_H_
#define _GLOBALS_H_ 

#include "includes.h"
#include "ThreadPool.h"

/// This file contains definition for all global data you might need
/// You can change the GLOBAL_DATA structure to fit your needs
/// You must NOT change the signature of the GlobalDataInitialize and GlobalDataUninitialize APIs
///
/// For example you will need a ThreadPool to perform the file transfer logic
/// You might also need some details regarding the current state of the application
///     whether a user is logged in or not, the directory of the logged in user, etc
/// You can use this struct to store what you need (a logging mechanism, users directory, ...)

#pragma pack(push, 16)
typedef struct _GLOBAL_DATA
{
    THREAD_POOL FileTransferThreadPool;
    BOOL        UserAuthenticated;
    PCHAR       UserPath;
    BYTE        Retries;
    time_t      Timeout;

}GLOBAL_DATA, *PGLOBAL_DATA;
#pragma pack(pop)

extern GLOBAL_DATA gGlobalData;


// This API is guaranteed to be called before any other command
// It is used to initialize the state of the application
// It returns STATUS_SUCCESS if everything succeeded or a proper error status otherwise.
_Must_inspect_result_
_No_competing_thread_
NTSTATUS
GlobalDataInitialize();

// This API must be called when the application exits
// It performs proper cleanup and resets the state of the application.
_No_competing_thread_
VOID
GlobalDataUninitialize();


#endif //_GLOBALS_H_