#include "globaldata.h"
#include "input_parser.h"
#include "commands.h"

APP_GLOBAL_DATA gApp;

int 
__cdecl
main(
    int argc,
    char *argv[]
)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    CommDriverPreinitialize();
    NTSTATUS status = CommDriverInitialize();
    if (status < 0)
    {
        return status;
    }

    UiHandleCommands();
    
    CommDriverUninitialize();
}