#ifndef _COMMON_DEFS_
#define _COMMON_DEFS_

#define SIOCTL_TYPE 40000

#define IOCTL_COMMAND_1  (CTL_CODE( SIOCTL_TYPE, 0x901, METHOD_IN_DIRECT, FILE_ANY_ACCESS  ))
#define IOCTL_COMMAND_2  (CTL_CODE( SIOCTL_TYPE, 0x902, METHOD_IN_DIRECT, FILE_ANY_ACCESS  ))


#define DEVICE_NAME             L"MyDriverDevice634423"

#define KERNEL_DEVICE_NAME      L"\\Device\\" ## DEVICE_NAME
#define DOS_DEVICE_NAME         L"\\DosDevices\\" ## DEVICE_NAME
#define UM_DEVICE_NAME          L"\\\\.\\" ## DEVICE_NAME
#endif