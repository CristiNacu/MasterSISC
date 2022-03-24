/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_KMDFPingPong,
    0xfff6b300,0x35d7,0x441a,0xaf,0xfb,0x33,0x4b,0x8f,0x10,0x45,0x66);
// {fff6b300-35d7-441a-affb-334b8f104566}
