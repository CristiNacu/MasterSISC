#include <fwpmtypes.h>
#include <ntddk.h>
#include <windef.h>
#include <ntddk.h>
#include <wdm.h>
#include <fwpsk.h>
#include <fwpmk.h>
#pragma comment(lib, "Fwpuclnt.lib")
// link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")


#define UINT_MAX 0xffffffff

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD MyUnload;
UINT32 deviceCalloutId = 0;
UINT32 engineCalloutId = 0;
UINT32 filterId1 = 0;
UINT32 filterId2 = 0;
HANDLE engineHandle = NULL;
PDEVICE_OBJECT device = NULL;

void MyUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING uniWin32NameString;

    if (filterId1)
    {
        FwpmFilterDeleteById(engineHandle, filterId1);
    }
    if (filterId2)
    {
        FwpmFilterDeleteById(engineHandle, filterId2);
    }
    if (engineHandle)
    {
        FwpmCalloutDeleteById(engineHandle, engineCalloutId);
    }
    if (deviceCalloutId)
    {
        FwpsCalloutUnregisterById(deviceCalloutId);
    }
    if (engineHandle)
    {
        FwpmEngineClose(engineHandle);
    }
    if (device)
    {
        IoDeleteDevice(device);
    }
    if (deviceObject != NULL)
    {
        IoDeleteDevice(deviceObject);
    }

}

void GetNetwork5TupleIndexesForLayer(
    _In_ UINT16 layerId,
    _Out_ UINT* appId,
    _Out_ UINT* localAddressIndex,
    _Out_ UINT* remoteAddressIndex,
    _Out_ UINT* localPortIndex,
    _Out_ UINT* remotePortIndex,
    _Out_ UINT* protocolIndex,
    _Out_ UINT* icmpIndex

);

NTSTATUS
MyCreate(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

const char* PRINT_STRINGS_FOR_FIELDS[] = {
        "appId",
        "localAddressIndex",
        "remoteAddressIndex",
        "localPortIndex",
        "remotePortIndex",
        "protocolIndex",
        "icmpIndex"
};

VOID NTAPI
ClassifyFn(
    IN const FWPS_INCOMING_VALUES0* inFixedValues,
    IN const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
    IN OUT VOID* layerData,
    IN const FWPS_FILTER0* filter,
    IN UINT64  flowContext,
    IN OUT FWPS_CLASSIFY_OUT0* classifyOut
)
{
    UINT ids[7];

    GetNetwork5TupleIndexesForLayer(
        inFixedValues->layerId,
        &ids[0],
        &ids[1],
        &ids[2],
        &ids[3],
        &ids[4],
        &ids[5],
        &ids[6]
    );

    for (int i = 0; i < 7; i++)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%s value: ", PRINT_STRINGS_FOR_FIELDS[i]);
        if (inFixedValues->incomingValue[ids[i]].value.type == FWP_EMPTY)
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NULL\n");
        else if (inFixedValues->incomingValue[ids[i]].value.type == FWP_UINT8)
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%ud\n", inFixedValues->incomingValue[ids[i]].value.uint8);
        else if (inFixedValues->incomingValue[ids[i]].value.type == FWP_UINT16)
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%ud\n", inFixedValues->incomingValue[ids[i]].value.uint16);
        else if (inFixedValues->incomingValue[ids[i]].value.type == FWP_UINT32)
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%ud\n", inFixedValues->incomingValue[ids[i]].value.uint32);
        else if (inFixedValues->incomingValue[ids[i]].value.type == FWP_UINT64)
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%ud\n", inFixedValues->incomingValue[ids[i]].value.uint64);
        else if (inFixedValues->incomingValue[ids[i]].value.type == FWP_INT8)
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%d\n", inFixedValues->incomingValue[ids[i]].value.int8);
        else if (inFixedValues->incomingValue[ids[i]].value.type == FWP_INT16)
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%d\n", inFixedValues->incomingValue[ids[i]].value.int16);
        else if (inFixedValues->incomingValue[ids[i]].value.type == FWP_INT32)
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%d\n", inFixedValues->incomingValue[ids[i]].value.int32);
        else if (inFixedValues->incomingValue[ids[i]].value.type == FWP_INT64)
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%d\n", inFixedValues->incomingValue[ids[i]].value.int64);
        else if (inFixedValues->incomingValue[ids[i]].value.type == FWP_UNICODE_STRING_TYPE)
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%S\n", inFixedValues->incomingValue[ids[i]].value.unicodeString);
    }
    return;
}

NTSTATUS NTAPI
NotifyFn(
    IN FWPS_CALLOUT_NOTIFY_TYPE notifyType,
    IN const GUID* filterKey,
    IN const FWPS_FILTER0* filter
)
{
    return;
}



NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS ntstatus;
    UNICODE_STRING deviceName;
    UNICODE_STRING dosDeviceName;
    UNICODE_STRING secondaryDriverName;
    FWPM_SUBLAYER0    fwpFilterSubLayer = {0};
    RPC_STATUS rpcStatus = RPC_S_OK;
    DWORD  result = ERROR_SUCCESS;
    FWPM_FILTER0    fwpFilter;
    NTSTATUS status;
    GUID guid;

    FWPS_CALLOUT0 calloutStructure = {
        .calloutKey = {0},
        .flags = 0,
        .notifyFn = NotifyFn,
        .classifyFn = ClassifyFn,
        .flowDeleteFn = NULL
    };
    

    FWPM_FILTER0 filterStructure1 = {
        .filterKey = {0},
        .displayData.name = L"MyFilter1",
        .flags = FWPM_FILTER_FLAG_NONE,
        .weight.type = FWP_EMPTY,
        .action.type = FWP_ACTION_CALLOUT_INSPECTION,
        .numFilterConditions = 0,
        .filterCondition = NULL,
        .layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4
    };


    FWPM_FILTER0 filterStructure2 = {
        .filterKey = {0},
        .displayData.name = L"MyFilter2",
        .flags = FWPM_FILTER_FLAG_NONE,
        .weight.type = FWP_EMPTY,
        .action.type = FWP_ACTION_CALLOUT_INSPECTION,
        .numFilterConditions = 0,
        .filterCondition = NULL,
        .layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4
    };

    __debugbreak();


    ntstatus = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &device);
    if (!NT_SUCCESS(ntstatus))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IoCreateDevice returned status %d\n", ntstatus);
        return ntstatus;
    }

    //To add a filter that references a callout, invoke the functions in the following order.

    //    Call FwpsCalloutRegister(documented in the Windows Driver Kit(WDK)), to register the callout with the filter engine.
    //    Call FwpmCalloutAdd0 to add the callout to the system.
    //    Call FwpmFilterAdd0 to add the filter that references the callout to the system.
    //    By default filters that reference callouts that have been added but have not yet registered with the filter engine are treated as Block filters.

    rpcStatus = UuidCreate(&guid);
    if (RPC_S_OK != rpcStatus)
    {
        printf("UuidCreate failed (%d).\n", rpcStatus);
        return;
    }

    memcpy(&fwpFilterSubLayer.subLayerKey, &guid, sizeof(GUID));
    memcpy(&filterStructure1.subLayerKey, &guid, sizeof(GUID));
    memcpy(&filterStructure2.subLayerKey, &guid, sizeof(GUID));

    rpcStatus = UuidCreate(&guid);
    if (RPC_S_OK != rpcStatus)
    {
        printf("UuidCreate failed (%d).\n", rpcStatus);
        return;
    }

    memcpy(&calloutStructure.calloutKey, &guid, sizeof(GUID));
    memcpy(&filterStructure1.filterKey, &guid, sizeof(GUID));
    memcpy(&filterStructure2.filterKey, &guid, sizeof(GUID));

    FwpmEngineOpen0(
        NULL,
        RPC_C_AUTHN_WINNT,
        NULL,
        NULL,
        &engineHandle
    );

    fwpFilterSubLayer.displayData.name = L"MyFilterSublayer";
    fwpFilterSubLayer.displayData.description = L"My filter sublayer";
    fwpFilterSubLayer.flags = 0;
    fwpFilterSubLayer.weight = 0x100;

    result = FwpmSubLayerAdd0(engineHandle, &fwpFilterSubLayer, NULL);
    if (result != ERROR_SUCCESS)
    {
        printf("FwpmSubLayerAdd0 failed (%d).\n", result);
        goto cleanup;
    }

    status = FwpsCalloutRegister0(
        device,
        &calloutStructure,
        &deviceCalloutId
    );
    if (!NT_SUCCESS(status))
    {
        printf("FwpsCalloutRegister0 failed (%x).\n", status);
        goto cleanup;
    }

    status = FwpmCalloutAdd0(
        engineHandle,
        &calloutStructure,
        NULL,
        &engineCalloutId
    );
    if (!NT_SUCCESS(status))
    {
        printf("FwpsCalloutRegister0 failed (%x).\n", status);
        goto cleanup;
    }

    result = FwpmFilterAdd0(
        engineHandle,
        &filterStructure1,
        NULL,
        &filterId1
    );

    result = FwpmFilterAdd0(
        engineHandle,
        &filterStructure2,
        NULL,
        &filterId2
    );


    DriverObject->DriverUnload = MyUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = MyCreate;

    return STATUS_SUCCESS;

cleanup:
    MyUnload(DriverObject);
    return status;
}


void GetNetwork5TupleIndexesForLayer(
    _In_ UINT16 layerId,
    _Out_ UINT* appId,
    _Out_ UINT* localAddressIndex,
    _Out_ UINT* remoteAddressIndex,
    _Out_ UINT* localPortIndex,
    _Out_ UINT* remotePortIndex,
    _Out_ UINT* protocolIndex,
    _Out_ UINT* icmpIndex

)
{
    switch (layerId)
    {
    case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
        *appId = FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_APP_ID;
        *localAddressIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS;
        *remoteAddressIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS;
        *localPortIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_PORT;
        *remotePortIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT;
        *protocolIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL;
        *icmpIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V4_ICMP_TYPE;
        break;
    case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
        *appId = FWPS_FIELD_ALE_AUTH_CONNECT_V6_ALE_APP_ID;
        *localAddressIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_ADDRESS;
        *remoteAddressIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_ADDRESS;
        *localPortIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_PORT;
        *remotePortIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_PORT;
        *protocolIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_PROTOCOL;
        *icmpIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V6_ICMP_TYPE;
        break;
    case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
        *appId = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_APP_ID;
        *localAddressIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS;
        *remoteAddressIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS;
        *localPortIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_PORT;
        *remotePortIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_PORT;
        *protocolIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL;
        *icmpIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ICMP_TYPE;
        break;
    case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
        *appId = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_ALE_APP_ID;
        *localAddressIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_ADDRESS;
        *remoteAddressIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_ADDRESS;
        *localPortIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_PORT;
        *remotePortIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_PORT;
        *protocolIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_PROTOCOL;
        *icmpIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_ICMP_TYPE;
        break;
    default:
        *appId = UINT_MAX;
        *localAddressIndex = UINT_MAX;
        *remoteAddressIndex = UINT_MAX;
        *localPortIndex = UINT_MAX;
        *remotePortIndex = UINT_MAX;
        *protocolIndex = UINT_MAX;
        *icmpIndex = UINT_MAX;
        NT_ASSERT(0);
    }
}