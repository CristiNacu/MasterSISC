#include <stdio.h>
#include <stdlib.h>
#include "Threadpool.h"
#include "Implementation.h"

#include "trace.h"
#include "Driver01.tmh"
int main()
{
    NTSTATUS status;
    THREAD_POOL threadPool;
    status = ThreadPoolInit(&threadPool);
    if (!NT_SUCCESS(status))
    {
        printf("ThreadPoolInit returned error %X", status);
        return;
    }

    char command[50];
    FltfLogTrace("Merge\n");
    
    while (strcmp(command, "exit")) 
    {
        scanf_s("%s", command, (unsigned)sizeof(command) - 1);

        if(!strcmp(command, "help"))
            printf("help requested. help given.\n");
        else if (!strcmp(command, "processes"))
        {
            printf_s("Listing processes\n");
            PROCESS_LIST *processList = ListAllProcesses();
            PrintProcessList(processList);
            FreeProcessList(&processList);
        }
        else
            printf("unknown command: %s\n", command);
    }

    ThreadPoolUninit(&threadPool);
    return;

}