#ifndef _COMMON_DEFS_
#define _COMMON_DEFS_

#define PRIMARY_SIOCTL_TYPE 40000

#define PRIMARY_IOCTL_COMMAND_1  (CTL_CODE( PRIMARY_SIOCTL_TYPE, 0x901, METHOD_IN_DIRECT, FILE_ANY_ACCESS  ))
#define PRIMARY_IOCTL_COMMAND_2  (CTL_CODE( PRIMARY_SIOCTL_TYPE, 0x902, METHOD_IN_DIRECT, FILE_ANY_ACCESS  ))


#define PRIMARY_DEVICE_NAME             L"PrimaryDriver"

#define PRIMARY_KERNEL_DEVICE_NAME      (L"\\Device\\" ## PRIMARY_DEVICE_NAME       )
#define PRIMARY_DOS_DEVICE_NAME         (L"\\DosDevices\\" ## PRIMARY_DEVICE_NAME   )
#define PRIMARY_UM_DEVICE_NAME          (L"\\\\.\\" ## PRIMARY_DEVICE_NAME          )

//Redefinition of the secondary driver name due to VS bug :/
#define SECONDARY_DEVICE_NAME             L"SecondaryDriver"
#define SECONDARY_KERNEL_DEVICE_NAME      (L"\\Device\\" ## SECONDARY_DEVICE_NAME)

#endif