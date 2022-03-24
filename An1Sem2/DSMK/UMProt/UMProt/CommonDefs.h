#ifndef _COMMON_DEFS_
#define _COMMON_DEFS_

#define SIOCTL_TYPE 40000

#define PROTECT_PID		(CTL_CODE( SIOCTL_TYPE, 0x901, METHOD_IN_DIRECT, FILE_ANY_ACCESS  ))
#define UNPROTECT_PID	(CTL_CODE( SIOCTL_TYPE, 0x902, METHOD_IN_DIRECT, FILE_ANY_ACCESS  ))


#define DEVICE_NAME             L"UserModeAppProtecetion"

#define KERNEL_DEVICE_NAME      (L"\\Device\\" ## DEVICE_NAME       )
#define DOS_DEVICE_NAME         (L"\\DosDevices\\" ## DEVICE_NAME   )
#define UM_DEVICE_NAME          (L"\\\\.\\" ## DEVICE_NAME          )

#define PROCESS_TERMINATE   0x01

#endif