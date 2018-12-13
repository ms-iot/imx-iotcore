/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    imxi2ccontroller.h

Abstract:

    This module contains the controller-specific function
    declarations.

Environment:

    kernel-mode only

*/

#ifndef _IMXI2CCONTROLLER_H_
#define _IMXI2CCONTROLLER_H_

//
// Controller specific function prototypes.
//

NTSTATUS ControllerInitialize(_In_ PDEVICE_CONTEXT DeviceCtxPtr);

NTSTATUS ControllerUninitialize(_In_ PDEVICE_CONTEXT DeviceCtxPtr);

NTSTATUS ResetController(
    _In_ PDEVICE_CONTEXT DeviceCtxPtr,
    _In_ BOOLEAN DisableFisrt);

NTSTATUS SetControllerClockDiv(
    _In_ PDEVICE_CONTEXT DeviceCtxPtr,
    _In_ ULONG ClockFrequencyHz);

NTSTATUS ControllerGenerateStart(
    _In_ PDEVICE_CONTEXT DeviceCtxPtr,
    _In_  PPBC_REQUEST RequestPtr);

NTSTATUS ControllerGenerateRepeatedStart(
    _In_ PDEVICE_CONTEXT DeviceCtxPtr,
    _In_  PPBC_REQUEST RequestPtr);

NTSTATUS ControllerGenerateStop(_In_ PDEVICE_CONTEXT DeviceCtxPtr);

VOID
ControllerConfigureForTransfer(
    _In_ PDEVICE_CONTEXT DeviceCtxPtr,
    _In_ PPBC_REQUEST pRequest);

NTSTATUS
ControllerTransferDataMultp(
    _In_ PDEVICE_CONTEXT DeviceCtxPtr,
    _In_ PPBC_REQUEST pRequest);

VOID
ControllerCompleteTransfer(
    _In_ PDEVICE_CONTEXT DeviceCtxPtr,
    _In_ PPBC_REQUEST pRequest,
    _In_ BOOLEAN AbortSequence);

#endif // of _IMXI2CCONTROLLER_H_

