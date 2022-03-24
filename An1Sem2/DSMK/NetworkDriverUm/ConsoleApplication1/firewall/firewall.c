//
//   Copyright (C) 2018 BitDefender S.R.L.
//   Author(s)    : Radu PORTASE(rportase@bitdefender.com)
//
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <Ws2tcpip.h>
#include <fwpmu.h>
#include <malloc.h>


#pragma comment (lib, "Fwpuclnt.lib")
#pragma comment (lib, "Ws2_32.lib")

#define OPT_INAVLID	0
#define OPT_LIST	1
#define OPT_ADD		2
#define OPT_DELETE	3
#define OPT_DEBUG	4
#define OPT_HELP	128

/* aac2f163-0c0b-41f6-b676-6d2403aa0d59 */
#define GUID_FILTER_DEBUG_LOG_RECV_V4 {					\
    0xaac2f163,											\
    0x0c0b,												\
    0x41f6,												\
    { 0xb6, 0x76, 0x6d, 0x24, 0x03, 0xaa, 0x0d, 0x59 }	\
 }

/* 2b5c0201-a763-4a0f-8428-5d63f7bd919c */
#define GUID_FILTER_DEBUG_LOG_CONNECT_V4 {												\
    0x2b5c0201,											\
    0xa763,												\
    0x4a0f,												\
    { 0x84, 0x28, 0x5d, 0x63, 0xf7, 0xbd, 0x91, 0x9c }	\
}

/* 582ab445-bf75-4c54-880f-db8f32f86fab */
#define GUID_CALLOUT_DEBUG_LOG_RECV_V4 {				\
    0x582ab445,											\
    0xbf75,												\
    0x4c54,												\
    { 0x88, 0x0f, 0xdb, 0x8f, 0x32, 0xf8, 0x6f, 0xab }	\
}

/* 005ae48f-5a02-4cf2-a52f-eee77c3d7b80 */
#define GUID_CALLOUT_DEBUG_LOG_CONNECT_V4 {				\
    0x005ae48f,											\
    0x5a02,												\
    0x4cf2,												\
    { 0xa5, 0x2f, 0xee, 0xe7, 0x7c, 0x3d, 0x7b, 0x80 }	\
}

void PrintHelp(char* ExecutableName)
{
    printf("%s help|list|add|del\n", ExecutableName);
    printf("\n");
    printf("	help\n");
    printf("	list\n");
    printf("	add <process>|* <domain>|*\n");
    printf("	del <filter-id>\n");
    printf("	debug-log on|off");
}

DWORD AddRule(HANDLE hEngine, wchar_t* Process, wchar_t* Domain)
{
    DWORD error;
    FWPM_FILTER filter;
    FWPM_FILTER_CONDITION conditions[4];
    FWP_V4_ADDR_AND_MASK addrAndMask;
    FWP_BYTE_BLOB* pAppId;
    ADDRINFOW getAddrHints;
    ADDRINFOW* plIpAddrInfo;
    struct sockaddr_in* pSockAddr;
    DWORD ipAddress;

    error = ERROR_SUCCESS;
    pAppId = NULL;
    plIpAddrInfo = NULL;

    error = FwpmGetAppIdFromFileName(Process, &pAppId);
    if (ERROR_SUCCESS != error)
    {
        printf("FwpmGetAppIdFromFileName returned: %d!\n", error);
        wprintf(L"Application name: %s\n", Process);
        goto cleanup;
    }

    memset(&getAddrHints, 0, sizeof(ADDRINFOW));
    getAddrHints.ai_family = AF_INET;
    error = GetAddrInfoW(
        Domain,
        NULL,
        &getAddrHints,
        &plIpAddrInfo
    );
    if (0 != error || AF_INET != plIpAddrInfo->ai_family)
    {
        printf("GetAddrInfoW returned: %d!\n", error);
        wprintf(L"Domain name:%s\n", Domain);
        goto cleanup;
    }
    pSockAddr = (struct sockaddr_in*)plIpAddrInfo->ai_addr;
    ipAddress = ntohl(pSockAddr->sin_addr.S_un.S_addr);
    printf("IP: %d.%d.%d.%d\n",
        ipAddress >> 24,
        (ipAddress & 0x00FF0000) >> 16,
        (ipAddress & 0x0000FF00) >> 8,
        (ipAddress & 0x000000FF)
    );

    ZeroMemory(&conditions[0], sizeof(FWPM_FILTER_CONDITION));
    conditions[0].fieldKey = FWPM_CONDITION_ALE_APP_ID;
    conditions[0].matchType = FWP_MATCH_EQUAL;
    conditions[0].conditionValue.type = FWP_BYTE_BLOB_TYPE;
    conditions[0].conditionValue.byteBlob = pAppId;

    ZeroMemory(&conditions[1], sizeof(FWPM_FILTER_CONDITION));
    conditions[1].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
    conditions[1].matchType = FWP_MATCH_EQUAL;
    conditions[1].conditionValue.type = FWP_V4_ADDR_MASK;
    addrAndMask.addr = ipAddress;
    addrAndMask.mask = 0xFFFFFFFF;
    conditions[1].conditionValue.v4AddrMask = &addrAndMask;

    ZeroMemory(&filter, sizeof(FWPM_FILTER));
    filter.displayData.name = (wchar_t*)malloc(sizeof(wchar_t) * 2);
    filter.displayData.name = Domain;
    filter.displayData.description = NULL;
    //filter.flags = FWPM_FILTER_FLAG_NONE;
    //filter.providerKey = NULL;
    //filter.providerData.size = 0;
    //filter.providerData.data = NULL;
    filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    //filter.subLayerKey = IID_NULL;
    filter.weight.type = FWP_EMPTY;
    filter.numFilterConditions = 2;
    //filter.numFilterConditions = 1;
    filter.filterCondition = conditions;
    filter.action.type = FWP_ACTION_BLOCK;
    //filter.effectiveWeight.type = FWP_EMPTY;

    error = FwpmFilterAdd(hEngine, &filter, NULL, NULL);
    if (ERROR_SUCCESS != error)
    {
        printf("FwpmFilterAdd returned: %d!\n", error);
    }
cleanup:
    if (NULL != pAppId)
    {
        FwpmFreeMemory((void**)&pAppId);
    }
    if (NULL != plIpAddrInfo)
    {
        FreeAddrInfoW(plIpAddrInfo);
        plIpAddrInfo = NULL;
    }
    return error;
}

DWORD DeleteRule(HANDLE hEngine, UINT64 FilterId)
{
    DWORD error;

    error = FwpmFilterDeleteById(hEngine, FilterId);
    if (ERROR_SUCCESS != error)
    {
        printf("FwpmFilterDeleteById returned: %d!\n", error);
    }

    return error;
}

DWORD ListRules(HANDLE hEngine)
{
    DWORD error;
    HANDLE hEnum;
    FWPM_FILTER** plEntries;
    UINT32 entriesReturned;

    error = ERROR_SUCCESS;
    hEnum = NULL;
    plEntries = NULL;
    entriesReturned = 0;

    error = FwpmFilterCreateEnumHandle(
        hEngine,
        NULL,
        &hEnum
    );
    if (ERROR_SUCCESS != error)
    {
        printf("FwpmFilterCreateEnumHandler returned: %d!\n", error);
        goto cleanup;
    }

    for (;;)
    {
        error = FwpmFilterEnum(
            hEngine,
            hEnum,
            1,
            &plEntries,
            &entriesReturned
        );
        if (ERROR_SUCCESS != error)
        {
            printf("FwpmFilterEnum returned: %d!\n", error);
            goto cleanup;
        }

        if (entriesReturned < 1 || NULL == plEntries)
        {
            break;
        }
        wprintf(L"%llu: %s\n", plEntries[0]->filterId, plEntries[0]->displayData.name);
        FwpmFreeMemory((void**)&plEntries);
    }

cleanup:
    if (NULL != hEnum)
    {
        FwpmFilterDestroyEnumHandle(hEngine, hEnum);
        hEnum = NULL;
    }
    return error;
}

DWORD DebugLogOn(HANDLE hEngine)
{
    DWORD error, ret;
    FWPM_FILTER filter;
    FWPM_CALLOUT0 callout;
    UINT32 idCalloutRecv4, idCalloutConnect4;
    UINT64 idFilterRecv4, idFilterConnect4;

    ret = ERROR_SUCCESS;
    error = ERROR_SUCCESS;
    idCalloutRecv4 = idCalloutConnect4 = 0;
    idFilterRecv4 = idFilterConnect4 = 0;

    ZeroMemory(&callout, sizeof(FWPM_CALLOUT0));
    callout.calloutKey = GUID_CALLOUT_DEBUG_LOG_RECV_V4;
    callout.displayData.name = L"Debug Log Callout Recv4";
    callout.displayData.description = NULL;
    callout.flags = 0;
    callout.providerKey = NULL;
    callout.providerData.size = 0;
    callout.providerData.data = NULL;
    callout.applicableLayer = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
    callout.calloutId = 0;
    error = FwpmCalloutAdd0(hEngine, &callout, NULL, &idCalloutRecv4);
    if (ERROR_SUCCESS != error)
    {
        printf("FwpmCalloutAdd failed for DebugLogCallout-Recv4: %d\n", error);
        goto cleanup;
    }

    callout.calloutKey = GUID_CALLOUT_DEBUG_LOG_CONNECT_V4;
    callout.displayData.name = L"Debug Log Callout Connect4";
    callout.displayData.description = NULL;
    callout.flags = 0;
    callout.providerKey = NULL;
    callout.providerData.size = 0;
    callout.providerData.data = NULL;
    callout.applicableLayer = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    callout.calloutId = 0;
    error = FwpmCalloutAdd0(hEngine, &callout, NULL, &idCalloutConnect4);
    if (ERROR_SUCCESS != error)
    {
        printf("FwpmCalloutAdd failed for DebugLogCallout-Connect4: %d\n", error);
        goto cleanup;
    }

    ZeroMemory(&filter, sizeof(FWPM_FILTER));
    filter.filterKey = GUID_FILTER_DEBUG_LOG_RECV_V4;
    filter.displayData.name = L"DebugLog-Recv4";
    filter.displayData.description = NULL;
    filter.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
    filter.weight.type = FWP_EMPTY;
    filter.numFilterConditions = 0;
    filter.filterCondition = NULL;
    filter.action.type = FWP_ACTION_CALLOUT_INSPECTION;
    filter.action.calloutKey = GUID_CALLOUT_DEBUG_LOG_RECV_V4;

    error = FwpmFilterAdd(hEngine, &filter, NULL, &idFilterRecv4);
    if (ERROR_SUCCESS != error)
    {
        printf("FwpmFilterAdd failed for DebugLog-Recv4: %d!\n", error);
        goto cleanup;
    }

    ZeroMemory(&filter, sizeof(FWPM_FILTER));
    filter.filterKey = GUID_FILTER_DEBUG_LOG_CONNECT_V4;
    filter.displayData.name = L"DebugLog-Connect4";
    filter.displayData.description = NULL;
    filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    filter.weight.type = FWP_EMPTY;
    filter.numFilterConditions = 0;
    filter.filterCondition = NULL;
    filter.action.type = FWP_ACTION_CALLOUT_INSPECTION;
    filter.action.calloutKey = GUID_CALLOUT_DEBUG_LOG_CONNECT_V4;

    error = FwpmFilterAdd(hEngine, &filter, NULL, &idFilterConnect4);
    if (ERROR_SUCCESS != error)
    {
        printf("FwpmFilterAdd failed for DebugLog-Connect4: %d!\n", error);
        goto cleanup;
    }

    ret = error;
    if (ERROR_SUCCESS == ret)
    {
        goto exit;
    }

cleanup:
    if (0 != idFilterConnect4)
    {
        error = FwpmFilterDeleteById(hEngine, idFilterRecv4);
        if (ERROR_SUCCESS != error)
        {
            printf("FwpmFilterDeleteById failed for DebugLog-Recv4: %d!\n", error);
        }
    }
    if (0 != idFilterRecv4)
    {
        error = FwpmFilterDeleteById(hEngine, idFilterConnect4);
        if (ERROR_SUCCESS != error)
        {
            printf("FwpmFilterDeleteById failed for DebugLog-Connect4: %d!\n", error);
        }
    }

    if (0 != idCalloutRecv4)
    {
        error = FwpmCalloutDeleteById(hEngine, idCalloutRecv4);
        if (ERROR_SUCCESS != error)
        {
            printf("FwpmCalloutDeleteById failed for DebugLog-Recv4: %d!\n", error);
        }
    }
    if (0 != idCalloutConnect4)
    {
        error = FwpmCalloutDeleteById(hEngine, idCalloutConnect4);
        if (ERROR_SUCCESS != error)
        {
            printf("FwpmCalloutDeleteById failed for DebugLog-Connect4: %d!\n", error);
        }
    }

exit:
    return ret;
}

DWORD DebugLogOff(HANDLE hEngine)
{
    DWORD error;
    GUID keyCalloutRecv4 = GUID_CALLOUT_DEBUG_LOG_RECV_V4;
    GUID keyCalloutConnect4 = GUID_CALLOUT_DEBUG_LOG_CONNECT_V4;
    GUID keyFilterRecv4 = GUID_FILTER_DEBUG_LOG_RECV_V4;
    GUID keyFilterConnect4 = GUID_FILTER_DEBUG_LOG_CONNECT_V4;

    error = FwpmFilterDeleteByKey(hEngine, &keyFilterRecv4);
    if (error != ERROR_SUCCESS)
    {
        printf("FwpmFilterDeleteByKey failed for DebugLog-Recv4: %d!\n", error);
    }

    error = FwpmFilterDeleteByKey(hEngine, &keyFilterConnect4);
    if (error != ERROR_SUCCESS)
    {
        printf("FwpmFilterDeleteByKey failed for DebugLog-Connect4: %d!\n", error);
    }

    error = FwpmCalloutDeleteByKey(hEngine, &keyCalloutRecv4);
    if (error != ERROR_SUCCESS)
    {
        printf("FwpmCalloutDeleteByKey failed for DebugLog-Recv4: %d!\n", error);
    }

    error = FwpmCalloutDeleteByKey(hEngine, &keyCalloutConnect4);
    if (error != ERROR_SUCCESS)
    {
        printf("FwpmCalloutDeleteByKey failed for DebugLog-Connect4: %d!\n", error);
    }

    return error;
}

int __cdecl main(int argc, char** argv)
{
    DWORD error, option;
    WORD wWsaVersion;
    WSADATA wsaData;
    HANDLE hEngine;
    BOOL wsaInitialized;

    error = ERROR_SUCCESS;
    option = OPT_INAVLID;
    hEngine = NULL;
    wWsaVersion = MAKEWORD(2, 2);
    wsaInitialized = FALSE;

    // Get option
    do
    {
        if (argc < 2 || 0 == strcmp("help", argv[1]))
        {
            option = OPT_HELP;
            break;
        }
        if (0 == strcmp("list", argv[1]))
        {
            option = OPT_LIST;
            break;
        }
        if (0 == strcmp("add", argv[1]))
        {
            option = OPT_ADD;
            break;
        }
        if (0 == strcmp("del", argv[1]))
        {
            option = OPT_DELETE;
            break;
        }
        if (0 == strcmp("debug-log", argv[1]))
        {
            option = OPT_DEBUG;
            break;
        }
    } while (0);

    if (OPT_INAVLID == option)
    {
        printf("Unknown option: %s!\n", argv[1]);
        PrintHelp(argv[0]);
        return 1;
    }

    // Check input
    if (OPT_ADD == option && argc != 4)
    {
        printf("Not enough arguments for add!\n");
        return 1;
    }
    if (OPT_DELETE == option && argc != 3)
    {
        printf("Not enough arguments for delete!\n");
        return 1;
    }
    if (OPT_DEBUG == option && argc != 3)
    {
        printf("Not enough parameters for debug!\n");
        return 1;
    }

    // Initialize WSA environment
    error = WSAStartup(wWsaVersion, &wsaData);
    if (0 != error)
    {
        printf("failed WSAStartup: %d!\n", error);
        goto cleanup;
    }
    wsaInitialized = TRUE;

    // Open engine
    error = FwpmEngineOpen(
        NULL,
        RPC_C_AUTHN_DEFAULT,
        NULL,
        NULL,
        &hEngine
    );

    if (ERROR_SUCCESS != error)
    {
        printf("Failed opening engine: %d!\n", error);
        goto cleanup;
    }

    switch (option)
    {
    case OPT_HELP:
    {
        PrintHelp(argv[0]);
        break;
    }
    case OPT_ADD:
    {
        wchar_t* process, * domain;
        size_t lenProcess, lenDomain;

        lenProcess = strlen(argv[2]);
        lenDomain = strlen(argv[3]);
        process = (wchar_t*)malloc((lenProcess + 1) * sizeof(wchar_t*));
        domain = (wchar_t*)malloc((lenDomain + 1) * sizeof(wchar_t*));
        mbstowcs(process, argv[2], lenProcess);
        process[lenProcess] = L'\x00';
        mbstowcs(domain, argv[3], lenDomain);
        domain[lenDomain] = L'\x00';

        error = AddRule(hEngine, process, domain);
        if (error)
        {
            printf("An error has occurred!\n");
        }

        free(domain);
        free(process);
        break;
    }
    case OPT_DELETE:
    {
        UINT64 filterId;

        filterId = atoll(argv[2]);
        error = DeleteRule(hEngine, filterId);
        if (error)
        {
            printf("An error has occured!\n");
        }

        break;
    }
    case OPT_DEBUG:
    {
        if (0 == strcmp("on", argv[2]))
        {
            error = DebugLogOn(hEngine);
        }
        else if (0 == strcmp("off", argv[2]))
        {
            error = DebugLogOff(hEngine);
        }
        else
        {
            printf("Invalid argument to debug-log: Use debug-log with on|off commands!");
        }
        if (0 != error)
        {
            printf("An error has occurred: %d!\n", error);
        }
        break;
    }
    case OPT_LIST:
    {
        error = ListRules(hEngine);
        if (error)
        {
            printf("An error has occured!\n");
        }
        break;
    }
    default:
        printf("Command not yet implemented!\n");
    }

cleanup:
    if (NULL != hEngine)
    {
        FwpmEngineClose(hEngine);
        hEngine = NULL;
    }

    if (wsaInitialized)
    {
        WSACleanup();
    }

    return error;
}