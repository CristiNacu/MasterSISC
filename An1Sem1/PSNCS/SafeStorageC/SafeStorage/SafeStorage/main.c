#include "includes.h"
#include "Commands.h"

/// You are not required to edit this file or this project (SafeStorage)
///    Any changes made to this file should be made only to validate 
///    your fixes for identified vulnerabilities.  
/// Your main task is to implement required functionalities in the SafeStorageLib project 
///    All new files and functions you need must be implemented in SafeStorageLib project.
///    All new functions must have proper SAL annotations
/// You can use NTSTATUS_FROM_WIN32 macro to convert the value returned from GetLastError() to a proper NTSTATUS
///
/// You need to guarantee proper resource and memory management (no memory leaks)
///
/// To run SAL on SafeStorageLib:
///    * Right click on SafeStorageLib in solution explorer in the right panel
///    * Go to Project only tab
///    * Click Run Code Analysis on only SafeStorageLib
///
/// To run SAL on entire solution:
///    * Right click on Solution 'SafeStorage' (2 projects):
///    * Go to Analyze tab
///    * Click on Run Code Analysis on Solution
/// 
/// You must identify the vulnerabilities present in this file
/// You have to submit a "vulnerabilities.txt" file 
///     containing the line with the vulnerability, the vulnerability, the fix
///     and the impact the vulnerability has.
/// There might be logic errors where you can not assign a specific line
///     in that case you only have to describe the error and the actual fix.


static void
PrintHelp()
{
    printf("Available commands:\n");
    printf("\t> register <username> <password>\n");
    printf("\t> login <username> <password>\n");
    printf("\t> logout\n");
    printf("\t> store <source file path> <submission name>\n");
    printf("\t> retrieve <submission name> <destination file path>\n");
    printf("\t> exit\n");
}

int main()
{
    char command[10];
    char arg1[MAX_PATH];
    char arg2[MAX_PATH];

    NTSTATUS status = GlobalDataInitialize();
    if (!NT_SUCCESS(status))
    {
        printf("GlobalDataInitialize failed with NTSTATUS = 0x%X\n", status);
        assert(FALSE);
        return -1;
    }

    PrintHelp();
    do
    {
        printf("Enter your command:\n");
        scanf("%s", command);

        if (memcmp(command, "register", sizeof("register")) == 0)
        {
            scanf("%s", arg1);    // username
            scanf("%s", arg2);    // password

            printf("register with username [%s] password [%s]\n", arg1, arg2);
            SafeStorageHandleRegister(arg1, (USHORT)strlen(arg1), arg2, (USHORT)strlen(arg2));
        }
        else if (memcmp(command, "login", sizeof("login")) == 0)
        {
            scanf("%s", arg1);    // username
            scanf("%s", arg2);    // password

            printf("login with username [%s] password [%s]\n", arg1, arg2);
            SafeStorageHandleLogin(arg1, (USHORT)strlen(arg1), arg2, (USHORT)strlen(arg2));
        }
        else if (memcmp(command, "logout", sizeof("logout")) == 0)
        {
            printf("logout\n");
            SafeStorageHandleLogout();
        }
        else if (memcmp(command, "store", sizeof("store")) == 0)
        {
            scanf("%s", arg1);    // source file path
            scanf("%s", arg2);    // submission name

            printf("store with source file path [%s] submission name [%s]\n", arg1, arg2);
            SafeStorageHandleStore(arg2, (USHORT)strlen(arg2), arg1, (USHORT)strlen(arg1));
        }
        else if (memcmp(command, "retrieve", sizeof("retrieve")) == 0)
        {
            scanf("%s", arg1);    // submission name 
            scanf("%s", arg2);    // destination file path

            printf("retrieve with submission name [%s] destination file path [%s]\n", arg1, arg2);
            SafeStorageHandleRetrieve(arg1, (USHORT)strlen(arg1), arg2, (USHORT)strlen(arg2));
        }
        else if (memcmp(command, "exit", sizeof("exit")) == 0)
        {
            SafeStorageHandleLogout();
            printf("Bye Bye!\n");
            break;
        }
        else
        {
            printf("Unknown command. Try again!\n");
        }
    } while (TRUE);

    return 0;
}