#define _WINSOCK_DEPRECATED_NO_WARNINGS
#undef UNICODE

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <windows.h>
#include <fwpmu.h>
#include <stdio.h>
#include <fwpmtypes.h>
#pragma comment(lib, "Fwpuclnt.lib")
// link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

int __cdecl main(int argc, char** argv)
{

    //-----------------------------------------
    // Declare and initialize variables
    WSADATA wsaData;
    int iResult;

    DWORD dwRetval;

    int i = 1;

    struct addrinfo* result = NULL;
    struct addrinfo* ptr = NULL;
    struct addrinfo hints;
    struct sockaddr_in* sockaddr_ipv4;
    DWORD ipbufferlength = 46;
    DWORD condCount = 0;
    HANDLE engineHandle;

    //FWPM_FILTER_CONDITION0 *filterCondition[4];
    //FWP_V4_ADDR_AND_MASK *ipMasks[4];
    UINT64 filterIds[4];
    int filterIdCount = 0;

    // Validate the parameters
    if (argc != 3) {
        printf("Invalid argument count\n");
        return 1;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    //--------------------------------
    // Setup the hints address info structure
    // which is passed to the getaddrinfo() function
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    dwRetval = getaddrinfo(
        argv[1], 
        "0", 
        &hints, 
        &result
    );
    if (dwRetval != 0) {
        printf("getaddrinfo failed with error: %d\n", dwRetval);
        WSACleanup();
        return 1;
    }

    dwRetval = FwpmEngineOpen0(
        NULL,
        RPC_C_AUTHN_WINNT,
        NULL,
        NULL,
        &engineHandle
    );
    if (dwRetval != ERROR_SUCCESS)
    {
        printf("FwpmEngineOpen0 error %X", dwRetval);
        goto cleanup;
    }


    // Retrieve each address and print out the hex bytes
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        if (ptr->ai_family == AF_INET) 
        {
            sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
            printf("\tIPv4 address %s\n", inet_ntoa(sockaddr_ipv4->sin_addr));

            FWPM_FILTER0 filter = {0};

            filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
            filter.action.type = FWP_ACTION_BLOCK; // echivalent FWP_ACTION_BLOCK
            filter.displayData.name = L"Filtru de yahu";
            filter.weight.type = FWP_EMPTY;
            filter.flags = FWPM_FILTER_FLAG_NONE;

            FWP_V4_ADDR_AND_MASK *ipmask = malloc(sizeof(FWP_V4_ADDR_AND_MASK));
            RtlZeroMemory(ipmask, sizeof(FWP_V4_ADDR_AND_MASK));

            ipmask->addr = ntohl(sockaddr_ipv4->sin_addr.S_un.S_addr);
            ipmask->mask = 0xFFFFFFFF;

            FWPM_FILTER_CONDITION0* pCondition = (FWPM_FILTER_CONDITION0*)malloc(sizeof(FWPM_FILTER_CONDITION0));
            RtlZeroMemory(pCondition, sizeof(FWPM_FILTER_CONDITION0));

            pCondition->fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
            pCondition->conditionValue.type = FWP_V4_ADDR_MASK;
            pCondition->conditionValue.v4AddrMask = ipmask;
            pCondition->matchType = FWP_MATCH_EQUAL;

            filter.numFilterConditions = 1;
            filter.filterCondition = pCondition;

            dwRetval = FwpmFilterAdd0(
                engineHandle,
                &filter,
                NULL,
                &filterIds[filterIdCount]
            );
            filterIdCount++;

        }
    }

    printf("Pres key");
    char c = getchar();

    for (int i = 0; i < filterIdCount; i++)
    {
        FwpmFilterDeleteById0(engineHandle, filterIds[i]);
    }

    FwpmEngineClose0(engineHandle);
    //dwRetval = FwpmFilterAdd0(engineHandle, &fwpFilter, NULL, NULL);


cleanup:
    freeaddrinfo(result);
    WSACleanup();

    return dwRetval;
}