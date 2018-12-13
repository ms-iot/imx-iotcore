// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//    ECSPIdriver.h
//
// Abstract:
//
//    This module contains the IMX ECSPI controller driver entry functions.
//    This controller driver uses the SPB WDF class extension (SpbCx).
//
// Environment:
//
//    kernel-mode only
//

#ifndef _ECSPI_DRIVER_H_
#define _ECSPI_DRIVER_H_

WDF_EXTERN_C_START

//
// Driver configuration parameters registry value names
//
#define REGSTR_VAL_REFERENCE_CLOCK_HZ L"ReferenceClockHz"
#define REGSTR_VAL_REFERENCE_MAX_SPEED_HZ L"MaxSpeedHz"
#define REGSTR_VAL_FLAGS L"Flags"


//
// ECSPI_DRIVER_FLAGS.
//
enum ECSPI_DRIVER_FLAGS : ULONG {
    
    ENABLE_LOOPBACK = 0x00000001,
};


//
// ECSPI_DRIVER_EXTENSION.
//  Contains all The IMXECSPI driver configuration parameters.
//  It is associated with the WDFDRIVER object.
//
typedef struct _ECSPI_DRIVER_EXTENSION
{
    //
    // Driver log handle
    //
    RECORDER_LOG DriverLogHandle;

    //
    // Driver configuration parameters
    // We read these parameters from registry,
    // since three is no standard way to get it
    // from UEFI.
    //
    //  The configuration parameters reside at:
    //  HKLM\System\CurrentControlSet\Services\imxecspi\Parameters
    //

    //
    // Reference clock.
    //
    ULONG ReferenceClockHz;

    //
    // Max connection speed.
    // Optional, if not given ReferenceClockHz is used.
    //
    ULONG MaxConnectionSpeedHz;

    //
    // Driver flags
    //
    ULONG Flags;

} ECSPI_DRIVER_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(ECSPI_DRIVER_EXTENSION, ECSPIDriverGetExtension);


//
// ECSPI driver WDF Driver event handlers
//
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD ECSPIEvtDriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD ECSPIEvtDeviceAdd;


ECSPI_DRIVER_EXTENSION*
ECSPIDriverGetDriverExtension ();

//
// Driver trace log handle
//
#define DRIVER_LOG_HANDLE ECSPIDriverGetDriverExtension()->DriverLogHandle

//
// Routine Description:
//
//  ECSPIDriverGetReferenceClock returns the reference clock in Hz.
//
// Arguments:
//
// Return Value:
//
//  Reference clock in Hz.
//
__forceinline
ULONG
ECSPIDriverGetReferenceClock ()
{
    return ECSPIDriverGetDriverExtension()->ReferenceClockHz;
}

//
// Routine Description:
//
//  ECSPIDriverGetMaxSpeed returns the maximum connection speed in Hz.
//
// Arguments:
//
// Return Value:
//
//  The maximum connection speed in Hz.
//
__forceinline
ULONG
ECSPIDriverGetMaxSpeed ()
{
    ULONG maxSpeed = ECSPIDriverGetDriverExtension()->MaxConnectionSpeedHz;
    if (maxSpeed == 0) {

        return ECSPIDriverGetReferenceClock();
    }
    return maxSpeed;
}

//
// Routine Description:
//
//  ECSPIDriverGetFlags returns the driver flags.
//
// Arguments:
//
// Return Value:
//
// The driver 32bit flags
//
__forceinline
ULONG
ECSPIDriverGetFlags ()
{
    return ECSPIDriverGetDriverExtension()->Flags;
}

//
// Routine Description:
//
//  ECSPIHwIsLoopback returns TRUE loopback is enabled.
//
// Arguments:
//
//  ECSPIRegsPtr - ECSPI registers base address
//
// Return Value:
//
//  TRUE if  loopback is enabled, otherwise FALSE.
//
__forceinline
BOOLEAN
ECSPIDriverIsLoopback ()
{
    return (ECSPIDriverGetFlags() & ECSPI_DRIVER_FLAGS::ENABLE_LOOPBACK) != 0;
}


//
// ECSPI driver private methods
//
#ifdef _ECSPI_DRIVER_CPP_

    _IRQL_requires_max_(PASSIVE_LEVEL)
    static NTSTATUS
    ECSPIpDriverReadConfig();

#endif //_ECSPI_DRIVER_CPP_


WDF_EXTERN_C_END

#endif // !_ECSPI_DRIVER_H_
