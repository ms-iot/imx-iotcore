/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    pbcrequests.cpp

Abstract:

   This file contains pbc request related functions

Environment:

    Kernel-mode Driver Framework

*/

#include "imxi2cinternal.h"
#include "imxi2cdevice.h"
#include "imxi2ccontroller.h"
#include <devpkey.h>
#include "pbcrequests.tmh"

// PBC functions.

/*++

  Routine Description:

    This routine populates the target's settings.

  Arguments:

    pDevice - a pointer to the PBC device context
    ConnectionParameters - a pointer to a blob containing the
        connection parameters
    Settings - a pointer the the target's settings

  Return Value:

    Status_success if all ok, error otherwise

--*/
_Use_decl_annotations_
NTSTATUS PbcTargetGetSettings(
    DEVICE_CONTEXT* pDevice,
    PVOID ConnectionParameters,
    PBC_TARGET_SETTINGS* pSettings
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    RH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER* connectionPtr;
    PNP_SERIAL_BUS_DESCRIPTOR* descriptorPtr;
    PNP_I2C_SERIAL_BUS_DESCRIPTOR* i2cDescriptorPtr;
    UNREFERENCED_PARAMETER(pDevice);

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_DEVICE, "++PbcTargetGetSettings()");

    NT_ASSERT(ConnectionParameters != nullptr);
    NT_ASSERT(pSettings != nullptr);

// get connection parameters from resourse hub/rhproxy and validate them

    connectionPtr = (RH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER*)ConnectionParameters;

    if (connectionPtr->PropertiesLength < sizeof(PNP_SERIAL_BUS_DESCRIPTOR)) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
                    "Invalid connection properties (length = %lu, expected = %Iu)",
                    connectionPtr->PropertiesLength,
                    sizeof(PNP_SERIAL_BUS_DESCRIPTOR));

        status = STATUS_INVALID_PARAMETER;
        goto EndGetTargetSet;
    }

    // check if connection is for proper bus type

    descriptorPtr = (PNP_SERIAL_BUS_DESCRIPTOR*)connectionPtr->ConnectionProperties;

    if (descriptorPtr->SerialBusType != I2C_SERIAL_BUS_TYPE) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
                    "ACPI Connnection descriptor is not an I2C. Bus type %c not supported",
                    descriptorPtr->SerialBusType);

        status = STATUS_NOT_SUPPORTED;
        goto EndGetTargetSet;
    }

    // ok bus type is of i2c type. Cast connection pointer so we can validate connection parameters

    i2cDescriptorPtr = (PNP_I2C_SERIAL_BUS_DESCRIPTOR*)connectionPtr->ConnectionProperties;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE,
                "I2C Connection Descriptor %p ConnectionSpeed:%lu Address:0x%hx",
                i2cDescriptorPtr,
                i2cDescriptorPtr->ConnectionSpeed,
                i2cDescriptorPtr->SlaveAddress);

    // check is master mode is selected

    if(i2cDescriptorPtr->SerialBusDescriptor.GeneralFlags & I2C_SLV_BIT) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
                    "iMX I2C controller slave mode not supported! Master mode only.");

        status = STATUS_NOT_SUPPORTED;
        goto EndGetTargetSet;
    }

    // validate Addressing mode
    // NOTE: iMX6 i2c does not support 10-bit addressing mode

    if(i2cDescriptorPtr->SerialBusDescriptor.TypeSpecificFlags &
        I2C_SERIAL_BUS_SPECIFIC_FLAG_10BIT_ADDRESS) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
                    "iMX I2C 10-bit addressing mode not supported! only 7-bit mode");

        status = STATUS_INVALID_PARAMETER;
        goto EndGetTargetSet;
    }
    pSettings->AddressMode = AddressMode7Bit;

    // validate Target address

    if(i2cDescriptorPtr->SlaveAddress > I2C_MAX_ADDRESS ) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
                    "Invalid slave I2C address %04Xh!", i2cDescriptorPtr->SlaveAddress);

        status = STATUS_INVALID_PARAMETER;
        goto EndGetTargetSet;
    }

    pSettings->Address = i2cDescriptorPtr->SlaveAddress;

    // validate Clock speed. Use this value {later} for clock divider

    if(i2cDescriptorPtr->ConnectionSpeed < IMX_I2C_MIN_CONNECTION_SPEED ||
        i2cDescriptorPtr->ConnectionSpeed > IMX_I2C_MAX_CONNECTION_SPEED ) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
                    "I2C connection speed %04Xh out of range. Must be between %ld and %ld",
                    i2cDescriptorPtr->ConnectionSpeed,
                    IMX_I2C_MIN_CONNECTION_SPEED,
                    IMX_I2C_MAX_CONNECTION_SPEED);

        status = STATUS_NOT_SUPPORTED;
        goto EndGetTargetSet;
    }

    pSettings->ConnectionSpeed = i2cDescriptorPtr->ConnectionSpeed;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE,
                "Connected to SPBTARGET. (SpbTarget = %p, "
                "targetPtr->Address = %04xh, targetPtr->ConnectionSpeed = %lu)",
                pSettings,
                pSettings->Address,
                pSettings->ConnectionSpeed);

EndGetTargetSet:
    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_DEVICE, "--PbcTargetGetSettings()=%Xh", status);
    return status;
}

/*++

  Routine Description:

    This routine validates pbc request.

  Arguments:

    pDevice - a pointer to the PBC device context

  Return Value:

    Status_success if all ok, error otherwise

--*/
_Use_decl_annotations_
NTSTATUS PbcRequestValidate(PPBC_REQUEST pRequest)
{
     TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_PBC, "++PbcRequestValidate()");

    SPB_TRANSFER_DESCRIPTOR descriptor;
    NTSTATUS status = STATUS_SUCCESS;

    // Validate each transfer descriptor.

    for (ULONG i = 0; i < pRequest->TransferCount; i++)
    {
        // Get transfer parameters for index.

        SPB_TRANSFER_DESCRIPTOR_INIT(&descriptor);

        SpbRequestGetTransferParameters(pRequest->SpbRequest,
                                        i,
                                        &descriptor,
                                        nullptr);

        // Validate the transfer length.

        if (descriptor.TransferLength > IMXI2C_MAX_TRANSFER_LENGTH) {

            status = STATUS_INVALID_PARAMETER;

            TraceEvents(TRACE_LEVEL_ERROR, TRACE_PBC,
                        "Transfer length %Iu is too large for controller driver, "
                        "max supported is %d (SPBREQUEST %p, index %lu) - %!STATUS!",
                        descriptor.TransferLength,
                        IMXI2C_MAX_TRANSFER_LENGTH,
                        pRequest->SpbRequest,
                        i,
                        status);

            break;
        }
    }

     TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_PBC, "--PbcRequestValidate()=%Xh",status);

    return status;
}

/*++

  Routine Description:

    This is a generic helper routine used to configure
    the request context and controller hardware for a non-
    sequence SPB request. It validates parameters and retrieves
    the transfer buffer as necessary.

  Arguments:

    pDevice - a pointer to the PBC device context
    pTarget - a pointer to the PBC target context
    pRequest - a pointer to the PBC request context
    Length - the number of bytes to read from the target

  Return Value:

    Status_success if all ok, error otherwise

--*/
_Use_decl_annotations_
VOID PbcRequestConfigureForNonSequence(
    WDFDEVICE SpbController,
    SPBTARGET SpbTarget,
    SPBREQUEST SpbRequest,
    size_t Length
    )
{
    PDEVICE_CONTEXT  pDevice  = GetDeviceContext(SpbController);
    PPBC_TARGET  pTarget  = GetTargetContext(SpbTarget);
    PPBC_REQUEST pRequest = GetRequestContext(SpbRequest);
    BOOLEAN completeRequest = FALSE;
    NTSTATUS status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(Length);

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_PBC, "++PbcRequestConfigureForNonSequence()");

    NT_ASSERT(pDevice  != NULL);
    NT_ASSERT(pTarget  != NULL);
    NT_ASSERT(pRequest != NULL);

    // Get the request parameters.

    SPB_REQUEST_PARAMETERS params;
    SPB_REQUEST_PARAMETERS_INIT(&params);
    SpbRequestGetParameters(SpbRequest, &params);

    // Initialize request context.

    pRequest->SpbRequest = SpbRequest;
    pRequest->Type = params.Type;
    pRequest->SequencePosition = params.Position;
    pRequest->TotalInformation = 0;
    pRequest->TransferCount = 1;
    pRequest->TransferIndex = 0;
    pRequest->bIoComplete = FALSE;

    // Validate the request before beginning the transfer.

    status = PbcRequestValidate(pRequest);

    if (!NT_SUCCESS(status)) {

        goto PbcRequestConfigureForNonSequenceEnd;
    }

    // Configure the request.

    status = PbcRequestConfigureForIndex(pRequest, pRequest->TransferIndex);

    if (!NT_SUCCESS(status)) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_PBC,
                    "Error configuring request context for SPBREQUEST %p (SPBTARGET %p)"
                    "- Err=%Xh",
                    pRequest->SpbRequest,
                    SpbTarget,
                    status);

        goto PbcRequestConfigureForNonSequenceEnd;
    }

    // Acquire the device lock.

    WdfSpinLockAcquire(pDevice->Lock);

    // Mark request cancellable (if cancellation supported).

    status = WdfRequestMarkCancelableEx(pRequest->SpbRequest, OnCancel);

    if (!NT_SUCCESS(status)) {

        // WdfRequestMarkCancelableEx should only fail if the request
        // has already been cancelled. If it does fail the request
        // must be completed with the corresponding status.

        NT_ASSERTMSG("WdfRequestMarkCancelableEx should only fail if the request has already been cancelled",
            status == STATUS_CANCELLED);

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_PBC,
                    "Failed to mark SPBREQUEST %p cancellable - %Xh",
                    pRequest->SpbRequest,
                    status);

        WdfSpinLockRelease(pDevice->Lock);
        goto PbcRequestConfigureForNonSequenceEnd;
    }

    // If sequence position is...
    //   - single:     ensure there is not a current target
    //   - not single: ensure that the current target is the
    //                 same as this target

    if (params.Position == SpbRequestSequencePositionSingle) {

        NT_ASSERT(pDevice->CurrentTargetPtr == NULL);

    } else {

        NT_ASSERT(pDevice->CurrentTargetPtr == pTarget);
    }

    // Ensure there is not a current request.

    NT_ASSERT(pTarget->CurrentRequestPtr == NULL);

    // Update the device and target contexts.

    if(pRequest->SequencePosition == SpbRequestSequencePositionSingle) {

        pDevice->CurrentTargetPtr = pTarget;
    }

    pTarget->CurrentRequestPtr = pRequest;

    // Configure controller and kick-off read or write.
    // Request will be completed asynchronously.

    PbcRequestDoTransfer(pDevice, pRequest);

    // complete the request synchronously. This must be done outside of
    // the locked code.

    if (pRequest->bIoComplete) {

        completeRequest = TRUE;
    }

    WdfSpinLockRelease(pDevice->Lock);

    // complete the request synchronously. This must be done outside of
    // the locked code.

    if (completeRequest) {

        PbcRequestComplete(pRequest);
    }

PbcRequestConfigureForNonSequenceEnd:

    if (!NT_SUCCESS(status)) {

        SpbRequestComplete(SpbRequest, status);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_PBC, "--PbcRequestConfigureForNonSequence()");
    return;
}

/*++

  Routine Description:

    This is a helper routine used to configure the request
    context and controller hardware for a transfer within a
    sequence. It validates parameters and retrieves
    the transfer buffer as necessary.

  Arguments:

    pRequest - a pointer to the PBC request context
    Index - index of the transfer within the sequence

  Return Value:

    Status_success if all ok, error otherwise

--*/
_Use_decl_annotations_
NTSTATUS PbcRequestConfigureForIndex(
    PPBC_REQUEST pRequest,
    ULONG Index
    )
{
    SPB_TRANSFER_DESCRIPTOR descriptor;
    PMDL pMdl;
    NTSTATUS status = STATUS_SUCCESS;

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_PBC, "++PbcRequestConfigureForIndex()");

    NT_ASSERT(pRequest != NULL);

    // Get transfer parameters for index.

    SPB_TRANSFER_DESCRIPTOR_INIT(&descriptor);

    SpbRequestGetTransferParameters(pRequest->SpbRequest,
                                    Index,
                                    &descriptor,
                                    &pMdl);

    NT_ASSERT(pMdl != NULL);

    // Configure request context.

    pRequest->pMdlChain = pMdl;
    pRequest->Length = descriptor.TransferLength;
    pRequest->Information = 0;
    pRequest->Direction = descriptor.Direction;
    pRequest->DelayInMicroseconds = descriptor.DelayInUs;

    // Update sequence position if request is type sequence.

    if (pRequest->Type == SpbRequestTypeSequence) {

        if (pRequest->TransferCount == 1) {

            pRequest->SequencePosition = SpbRequestSequencePositionSingle;
        }
        else if (Index == 0) {

            pRequest->SequencePosition = SpbRequestSequencePositionFirst;
        }
        else if (Index == (pRequest->TransferCount - 1)) {

            pRequest->SequencePosition = SpbRequestSequencePositionLast;

        } else {

            pRequest->SequencePosition = SpbRequestSequencePositionContinue;
        }
    }

    PPBC_TARGET pTarget = GetTargetContext(SpbRequestGetTarget(pRequest->SpbRequest));
    NT_ASSERT(pTarget != NULL);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_PBC,
                "Request context configured for %s (index %lu) to address 0x%lx (SPBTARGET %p)",
                pRequest->Direction == SpbTransferDirectionFromDevice ? "read" : "write",
                Index,
                pTarget->Settings.Address,
                pTarget->SpbTarget);

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_PBC, "--PbcRequestConfigureForIndex()");

    return status;
}

/*++

  Routine Description:

    This routine either starts the delay timer or
    kicks off the transfer depending on the request
    parameters.

  Arguments:

    pDevice - a pointer to the PBC device context
    pRequest - a pointer to the PBC request context

  Return Value:

    None. The request is completed elsewhere.

--*/
_Use_decl_annotations_
VOID PbcRequestDoTransfer(
    PDEVICE_CONTEXT pDevice,
    PPBC_REQUEST pRequest
    )
{
    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_PBC, "++PbcRequestDoTransfer()");

    NT_ASSERT(pDevice != NULL);
    NT_ASSERT(pRequest != NULL);

    // Start delay timer if necessary for this request,
    // otherwise continue transfer.

    // NOTE: Note using a timer to implement IO delay is only
    // applicable for sufficiently long delays (> 15ms).
    // For shorter delays, on the order of
    // microseconds, use different mechanism.

    if (pRequest->DelayInMicroseconds > 10000) {

        BOOLEAN bTimerAlreadyStarted;

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_PBC,
                    "Delaying %lu us before configuring transfer for WDFDEVICE %p",
                    pRequest->DelayInMicroseconds,
                    pDevice->WdfDevice);

        bTimerAlreadyStarted = WdfTimerStart(pDevice->DelayTimer,
                                             WDF_REL_TIMEOUT_IN_US(pRequest->DelayInMicroseconds));

        // There should never be another request
        // scheduled for delay.

        if (bTimerAlreadyStarted == TRUE) {

            TraceEvents(TRACE_LEVEL_ERROR, TRACE_PBC,
                        "The delay timer should not be started");
        }
    } else {

        ControllerConfigureForTransfer(pDevice, pRequest);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_PBC, "--PbcRequestDoTransfer()");
    return;
}

/*++

  Routine Description:

    This routine completes the SpbRequest associated with
    the PBC_REQUEST context.

  Arguments:

    pRequest - a pointer to the PBC request context

  Return Value:

    None.

--*/
_Use_decl_annotations_
VOID PbcRequestComplete(PPBC_REQUEST pRequest)
{
    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_PBC, "++PbcRequestComplete()");

    NT_ASSERT(pRequest != NULL);

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_PBC,
                "Completing SPBREQUEST %p with %Xh, transferred %Iu bytes",
                pRequest->SpbRequest,
                pRequest->Status,
                pRequest->TotalInformation);

    WdfRequestSetInformation(pRequest->SpbRequest,
                            pRequest->TotalInformation);

    SpbRequestComplete(pRequest->SpbRequest,
                        pRequest->Status);

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_PBC, "--PbcRequestComplete()");
    return;
}

/*++

  Routine Description:

    This is a helper routine used to retrieve the
    number of bytes remaining in the current transfer.

  Arguments:

    pRequest - a pointer to the PBC request context

  Return Value:

    Bytes remaining in request

--*/
_Use_decl_annotations_
size_t
PbcRequestGetInfoRemaining(PPBC_REQUEST pRequest)
{
    return (pRequest->Length - pRequest->Information);
}

/*++

  Routine Description:

    This is a helper routine used to retrieve the
    specified byte of the current transfer descriptor buffer.

  Arguments:

    pRequest - a pointer to the PBC request context

    Index - index of desired byte in current transfer descriptor buffer

    pByte - pointer to the location for the specified byte

  Return Value:

    STATUS_INFO_LENGTH_MISMATCH if invalid index,
    otherwise STATUS_SUCCESS

--*/
_Use_decl_annotations_
NTSTATUS
PbcRequestGetByte(
    PPBC_REQUEST pRequest,
    size_t Index,
    UCHAR* pByte)
{
    PMDL mdl = pRequest->pMdlChain;
    size_t mdlByteCount;
    size_t currentOffset = Index;
    PUCHAR pBuffer;
    NTSTATUS status = STATUS_INFO_LENGTH_MISMATCH;

    // Check for out-of-bounds index

    if (Index < pRequest->Length) {

        while (mdl != NULL) {

            mdlByteCount = MmGetMdlByteCount(mdl);

            if (currentOffset < mdlByteCount) {

                pBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(mdl,
                                                                NormalPagePriority |
                                                                MdlMappingNoExecute);

                if (pBuffer != NULL) {

                    // Byte found, mark successful

                    *pByte = pBuffer[currentOffset];
                    status = STATUS_SUCCESS;
                }

                break;
            }

            currentOffset -= mdlByteCount;
            mdl = mdl->Next;
        }

        // If after walking the MDL the byte hasn't been found,
        // status will still be STATUS_INFO_LENGTH_MISMATCH
    }

    return status;
}

/*++

  Routine Description:

    This is a helper routine used to set the
    specified byte of the current transfer descriptor buffer.

  Arguments:

    pRequest - a pointer to the PBC request context

    Index - index of desired byte in current transfer descriptor buffer

    Byte - the byte

  Return Value:

    STATUS_INFO_LENGTH_MISMATCH if invalid index,
    otherwise STATUS_SUCCESS

--*/
_Use_decl_annotations_
NTSTATUS
PbcRequestSetByte(
    PPBC_REQUEST pRequest,
    size_t Index,
    UCHAR Byte
   )
{
    PMDL mdl = pRequest->pMdlChain;
    size_t mdlByteCount;
    size_t currentOffset = Index;
    PUCHAR pBuffer;
    NTSTATUS status = STATUS_INFO_LENGTH_MISMATCH;

    // Check for out-of-bounds index

    if (Index < pRequest->Length) {

        while (mdl != NULL) {

            mdlByteCount = MmGetMdlByteCount(mdl);

            if (currentOffset < mdlByteCount) {

                pBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(mdl,
                                                                NormalPagePriority |
                                                                MdlMappingNoExecute);

                if (pBuffer != NULL) {

                    // Byte found, mark successful

                    pBuffer[currentOffset] = Byte;
                    status = STATUS_SUCCESS;
                }

                break;
            }

            currentOffset -= mdlByteCount;
            mdl = mdl->Next;
        }

        // If after walking the MDL the byte hasn't been found,
        // status will still be STATUS_INFO_LENGTH_MISMATCH
    }

    return status;
}