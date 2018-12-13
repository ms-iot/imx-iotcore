/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    imxcontroller.cpp

Abstract:

    This module contains the controller-specific functions
    for handling transfers.

Environment:

    kernel-mode only

Revision History:

*/

#include "imxi2cinternal.h"
#include "imxi2ccontroller.h"
#include "imxi2cdevice.h"

#include "imxcontroller.tmh"

const PBC_TRANSFER_SETTINGS g_TransferSettings[] =
{
    // Bus condition        IsStart  IsEnd
    {BusConditionDontCare,  FALSE,   FALSE}, // SpbRequestTypeInvalid
    {BusConditionFree,      TRUE,    TRUE},  // SpbRequestTypeSingle
    {BusConditionFree,      TRUE,    FALSE}, // SpbRequestTypeFirst
    {BusConditionBusy,      FALSE,   FALSE}, // SpbRequestTypeContinue
    {BusConditionBusy,      FALSE,   TRUE}   // SpbRequestTypeLast
};

// fixed clock divider ratios as per Freescale IMX6 programming manual
// note: only selected fixed I2C clock frequecnies are supported.

static const USHORT I2C_Clock_Rate_Dividers_Table[] =
{
    30,  32,  36,  42,  48,  52,  60,  72,   80,   88,  104,  128,  144,  160,  192,  240,
    288, 320, 384, 480, 576, 640, 768, 960, 1152, 1280, 1536, 1920, 2304, 2560, 3072, 3840,
    22,  24,  26,  28,  32,  36,  40,  44,   48,   56,   64,   72,   80,   96,  112,  128,
    160, 192, 224, 256, 320, 384, 448, 512,  640,  768,  896, 1024, 1280, 1536, 1792, 2048
};

#define I2C_DIV_TAB_SIZE (sizeof(I2C_Clock_Rate_Dividers_Table)/sizeof(I2C_Clock_Rate_Dividers_Table[0]))
#define I2C_MAXDIVIDER 3840
#define I2C_MINDIVIDER 22

/*++

  Routine Description:

    This routine initializes the controller hardware.

  Arguments:

    pDevice - a pointer to the PBC device context

  Return Value:

    None.

--*/
_Use_decl_annotations_
NTSTATUS ControllerInitialize(PDEVICE_CONTEXT DeviceCtxPtr)
{
    NTSTATUS status = STATUS_SUCCESS;
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR, "++ControllerInitialize()");

    // Disable I2C Module Initially

    DeviceCtxPtr->RegistersPtr->ControlReg = (USHORT)0x00;
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "ControllerInitialize() i2c block is disabled");

    // Set I2C clock divider to default conenction frequency.

    SetControllerClockDiv(DeviceCtxPtr, IMX_I2C_MIN_CONNECTION_SPEED);

    // reset controller. Function will enable controller

    if(STATUS_SUCCESS != ResetController(DeviceCtxPtr, FALSE)) {

       TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerInitialize() i2c block reset failed!");
    } else {

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                    "ControllerInitialize() i2c block is ready.");

        DeviceCtxPtr->RegistersPtr->ControlReg = DeviceCtxPtr->RegistersPtr->ControlReg & ~IMX_I2C_CTRL_REG_IEN_MASK;

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                    "ControllerInitialize() CTRL reg=%04Xh",
                    DeviceCtxPtr->RegistersPtr->ControlReg);

        // clear out interrupt bit if any were set

        DeviceCtxPtr->RegistersPtr->StatusReg = DeviceCtxPtr->RegistersPtr->StatusReg & ~IMX_I2C_STA_REG_IIF_MASK;

        // disable controller

        DeviceCtxPtr->RegistersPtr->ControlReg = (USHORT)0x0;

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                    "ControllerInitialize() Sts reg=%04Xh",
                    DeviceCtxPtr->RegistersPtr->StatusReg);
    };

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR, "--ControllerInitialize()=%Xh",status);
    return status;
}

/*++

  Routine Description:

    This routine finds best fit clock divider for given i2c clock speed in kHz

  Arguments:

    DeviceCtxPtr - a pointer to device context
    DesiredClockFrequencyHz - desired bus clock frequency in kHz

  Return Value:

    status_success if it was possible to find close enough divider in the table
    error if not

--*/
_Use_decl_annotations_
NTSTATUS SetControllerClockDiv(
    PDEVICE_CONTEXT DeviceCtxPtr,
    ULONG DesiredClockFrequencyHz
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT estimatedDivider = 0;
    int iFirstRd = 0;
    int iSecondRd = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "++SetControllerClockDiv(%lu Hz)",
                DesiredClockFrequencyHz);

    // perform validation

    if(DesiredClockFrequencyHz > IMX_I2C_MAX_CONNECTION_SPEED ||
                        DesiredClockFrequencyHz < IMX_I2C_MIN_CONNECTION_SPEED) {


        // error - out of range

        status = STATUS_NOT_SUPPORTED;
        goto SetClockFreqDone;
    }

    estimatedDivider=(USHORT)((DeviceCtxPtr->ModuleClock_kHz*1000) / DesiredClockFrequencyHz);

    // round divider up

    if((DeviceCtxPtr->ModuleClock_kHz * 1000 -
        (DesiredClockFrequencyHz * estimatedDivider)) * 100 >
            DeviceCtxPtr->ModuleClock_kHz * 1000) {

        estimatedDivider += 1;
    }

    // now perform a search in imx6 table of dividers
    // use left part of the table first

    for(iFirstRd = 0; iFirstRd < (I2C_DIV_TAB_SIZE/2 - 1); iFirstRd++) {
        // Once found a divider greater than the estimate, stop

        if (estimatedDivider <= I2C_Clock_Rate_Dividers_Table[iFirstRd])
            break;
    }

    if(estimatedDivider == I2C_Clock_Rate_Dividers_Table[iFirstRd]) {


        // If the estimated divider matched one of the array entries, no need
        // to search further

        estimatedDivider = (USHORT)iFirstRd;
    } else {
        // look int the right part of the table

        for (iSecondRd = (I2C_DIV_TAB_SIZE/2); iSecondRd < (I2C_DIV_TAB_SIZE-1); iSecondRd++)
        {
            // Again, if a greater entry is found, stop

            if (estimatedDivider <= I2C_Clock_Rate_Dividers_Table[iSecondRd])
                break;
        }

        if (estimatedDivider == I2C_Clock_Rate_Dividers_Table[iSecondRd]) {

            // If the estimated divider is found in the second round, stop

            estimatedDivider = (USHORT)iSecondRd;

        } else {
            // Search for the nearest divider among the 2 portion of the array

            if ((I2C_Clock_Rate_Dividers_Table[iFirstRd] > estimatedDivider) &&
                            (I2C_Clock_Rate_Dividers_Table[iSecondRd] > estimatedDivider)) {

                if ((I2C_Clock_Rate_Dividers_Table[iFirstRd] - estimatedDivider) <
                            (I2C_Clock_Rate_Dividers_Table[iSecondRd] - estimatedDivider)) {

                    estimatedDivider = (USHORT)iFirstRd;

                    } else {

                    estimatedDivider = (USHORT)iSecondRd;
                    }
            } else {
                if (I2C_Clock_Rate_Dividers_Table[iSecondRd] > estimatedDivider) {

                    estimatedDivider = (USHORT)iSecondRd;
                } else {
                    // Less than setting, use I2CClockRateDividersTable[31] as default

                    estimatedDivider = (USHORT)iFirstRd;
                }
            }
        }
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "SetControllerClockDiv() divider: idx=%u, value=%u",
                estimatedDivider,
                I2C_Clock_Rate_Dividers_Table[estimatedDivider]);

    DeviceCtxPtr->RegistersPtr->FreqDivReg = estimatedDivider;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "SetControllerClockDiv() ReqDiv Reg=%04Xh",
                DeviceCtxPtr->RegistersPtr->FreqDivReg);

SetClockFreqDone:

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "--SetControllerClockDiv()=%Xh",
                status);
    return status;
}

/*++

  Routine Description:

    This routine resets iMX i2c controller

  Arguments:

    DeviceCtxPtr - a pointer to device context
    DisableFirst - boolean to specify should controller be disabled first

  Return Value:

    None.

--*/
_Use_decl_annotations_
NTSTATUS ResetController(
    PDEVICE_CONTEXT DeviceCtxPtr,
    BOOLEAN DisableFirst
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    const int nMaxResetIter = 999;
    int n = 0;
    LARGE_INTEGER liTimeout = {0};

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR, "++ResetController()");

    liTimeout.QuadPart = (LONGLONG)(-10); // 1us in 100ns units

    // enable and reset given i2c controller hw

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "ResetController() CTRL reg=%04Xh upon entry",
                DeviceCtxPtr->RegistersPtr->ControlReg);

    if(TRUE == DisableFirst) {


        // disable I2C Module Initially. clear bit 7.
        // Note: WinCE and iMX data flow example clears entire i2cr register

        DeviceCtxPtr->RegistersPtr->ControlReg = (USHORT)0x0;
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                    "ResetController() i2c block is disabled");

        // wait a bit so that hardware has time to complete disable operation

        KeDelayExecutionThread(KernelMode, FALSE, &liTimeout);
    }

    // now enable it back (set bit 7) and

    DeviceCtxPtr->RegistersPtr->ControlReg = DeviceCtxPtr->RegistersPtr->ControlReg | IMX_I2C_CTRL_REG_IEN_MASK;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "ResetController() i2c block starting reset");

    // wait until i2c controller completes reset sequence

    for(n=0; n < nMaxResetIter; n++)
    {
        KeDelayExecutionThread(KernelMode, FALSE, &liTimeout);

        if(DeviceCtxPtr->RegistersPtr->ControlReg & IMX_I2C_CTRL_REG_IEN_MASK) {

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                        "ResetController() i2c controller reset complete. Iter=%ld",
                        n);
            break;
        }
    }

    // check if resetting controller hardware timed out

    if((DeviceCtxPtr->RegistersPtr->ControlReg & IMX_I2C_CTRL_REG_IEN_MASK) == 0) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ResetController() i2c controller reset timeout! n=%ld",n);

        status = STATUS_DEVICE_HARDWARE_ERROR;
    }

    // cleart out controller status

    DeviceCtxPtr->RegistersPtr->StatusReg = (USHORT)0;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "--ResetController()=%Xh",
                status);

    return status;
}

/*++

  Routine Description:

    This routine uninitializes the controller hardware.

  Arguments:

    DeviceCtxPtr - a pointer to device context

  Return Value:

    Ntstatus of operation.

--*/
_Use_decl_annotations_
NTSTATUS ControllerUninitialize(PDEVICE_CONTEXT DeviceCtxPtr)
{
    NTSTATUS status = STATUS_SUCCESS;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "++ControllerUninitialize()");

    NT_ASSERT(DeviceCtxPtr != NULL);

    // Uninitialize controller hardware via the pDevice->RegistersPtr->* register interface

    DeviceCtxPtr->RegistersPtr->ControlReg = (USHORT)0x0;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "--ControllerUninitialize()=%Xh",
                status);

    return status;
}

/*++

  Routine Description:

    This routine configures and starts the controller
    for a transfer.

  Arguments:

    pDevice - a pointer to the PBC device context
    pRequest - a pointer to the PBC request context

  Return Value:

    None. The request is completed asynchronously.

--*/
_Use_decl_annotations_
VOID ControllerConfigureForTransfer(
    PDEVICE_CONTEXT DeviceCtxPtr,
    PPBC_REQUEST RequestPtr
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR, "++ControllerConfigureForTransfer()");

    NT_ASSERT(DeviceCtxPtr  != NULL);
    NT_ASSERT(RequestPtr != NULL);

    // Initialize request context for transfer.

    RequestPtr->Settings = g_TransferSettings[RequestPtr->SequencePosition];
    RequestPtr->Status = STATUS_SUCCESS;

    // Configure hardware for transfer.

    // Initialize controller hardware for a general
    // transfer via the pDevice->RegistersPtr->* register interface.

    if(RequestPtr->SequencePosition == SpbRequestSequencePositionFirst  ||
        RequestPtr->SequencePosition == SpbRequestSequencePositionSingle) {

        // disable i2c block

        DeviceCtxPtr->RegistersPtr->ControlReg = (USHORT)0;

        // set data sampling rate SCL frequency

        SetControllerClockDiv(DeviceCtxPtr,
                            DeviceCtxPtr->CurrentTargetPtr->Settings.ConnectionSpeed);

        // clear out i2c status

        DeviceCtxPtr->RegistersPtr->ControlReg =
            DeviceCtxPtr->RegistersPtr->StatusReg = (USHORT)0;

        // enable i2c block

        DeviceCtxPtr->RegistersPtr->ControlReg = DeviceCtxPtr->RegistersPtr->ControlReg | IMX_I2C_CTRL_REG_IEN_MASK;

#if (defined(DBG) || defined(DEBUG))
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                    "C AdrReg=%04Xh FDReg=%04Xh CtlReg=%04Xh StaReg=%04Xh",
                    DeviceCtxPtr->RegistersPtr->AddressReg,
                    DeviceCtxPtr->RegistersPtr->FreqDivReg,
                    DeviceCtxPtr->RegistersPtr->ControlReg,
                    DeviceCtxPtr->RegistersPtr->StatusReg);
#endif
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "Controller configured for %s of %Iu bytes %s address 0x%lx "
                "(SPBREQUEST %p, WDFDEVICE %p)",
                RequestPtr->Direction == SpbTransferDirectionFromDevice ? "read" : "write",
                RequestPtr->Length,
                RequestPtr->Direction == SpbTransferDirectionFromDevice ? "from" : "to",
                DeviceCtxPtr->CurrentTargetPtr->Settings.Address,
                RequestPtr->SpbRequest,
                DeviceCtxPtr->WdfDevice);

    // Perform necessary action to begin transfer.

#if (defined(DBG) || defined(DEBUG))
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "M AdrReg=%04Xh FDReg=%04Xh CtlReg=%04Xh StaReg=%04Xh",
                DeviceCtxPtr->RegistersPtr->AddressReg,
                DeviceCtxPtr->RegistersPtr->FreqDivReg,
                DeviceCtxPtr->RegistersPtr->ControlReg,
                DeviceCtxPtr->RegistersPtr->StatusReg);
#endif

    status = ControllerTransferDataMultp(DeviceCtxPtr, RequestPtr);

    // complete the request synchronously.
    // partial operations return success per PBC framework
    // do not proceed with next transfer after partial transfer

    if(RequestPtr->Information != RequestPtr->Length || status != STATUS_SUCCESS) {

        ControllerCompleteTransfer(DeviceCtxPtr, RequestPtr, TRUE);
    } else {

        ControllerCompleteTransfer(DeviceCtxPtr, RequestPtr, FALSE);
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR, "--ControllerConfigureForTransfer()");
    return;
}

/*++

  Routine Description:

    This routine transfers data to or from the device.

  Arguments:

    DeviceCtxPtr - a pointer to device context
    RequestPtr - a pointer to the PBC request context

  Return Value:

    None.
 --*/

_Use_decl_annotations_
NTSTATUS ControllerTransferDataMultp(
    PDEVICE_CONTEXT DeviceCtxPtr,
    PPBC_REQUEST RequestPtr
    )
{
    int timeoutMax = 25;
    int n = 0;
    UCHAR UchOneByteToWrite = 0x00;
    UCHAR UchOneByteRead = 0x00;
    size_t bytesToTransfer = 0;
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS transferStatus = STATUS_SUCCESS;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR, "++ControllerTransferDataMultp()");

    NT_ASSERT(DeviceCtxPtr  != NULL);
    NT_ASSERT(RequestPtr != NULL);

    // the timeout value depends on i2c clock frequency
    // formula : timeout = 10/Fscl
    // the minimum timeout for polling IIF bit is 25us at 400 kHz (period 2.5us x 10 )

    timeoutMax = ( 10 * 1000000 ) / DeviceCtxPtr->CurrentTargetPtr->Settings.ConnectionSpeed;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "ControllerTransferDataMultp() timeout value=%lu us",
                timeoutMax);


    if(RequestPtr->SequencePosition == SpbRequestSequencePositionSingle ||
        RequestPtr->SequencePosition == SpbRequestSequencePositionFirst) {

        status = ControllerGenerateStart(DeviceCtxPtr, RequestPtr);

        if(STATUS_SUCCESS != status) {

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                        "ControllerTransferDataMultp() Start failed!");

            goto ControllerTransferDataMultpEnd;
        }
    } else {
        status = ControllerGenerateRepeatedStart(DeviceCtxPtr, RequestPtr);

        if(STATUS_SUCCESS != status) {

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                        "ControllerTransferDataMultp() Repeated Start failed!");

            goto ControllerTransferDataMultpEnd;
        }
    }

    // switch into receive mode if needed prior to loop start

    if(RequestPtr->Direction == SpbTransferDirectionFromDevice) {

        DeviceCtxPtr->RegistersPtr->ControlReg = DeviceCtxPtr->RegistersPtr->ControlReg & ~IMX_I2C_CTRL_REG_MTX_MASK;

        // nxp application note AN4481

        if(RequestPtr->Length==1)
        {
            DeviceCtxPtr->RegistersPtr->ControlReg = DeviceCtxPtr->RegistersPtr->ControlReg | IMX_I2C_CTRL_REG_TXAK_MASK;
             TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                    "ControllerTransferDataMultp(R) set TXAK bit since reading only 1 byte");
        }

        // make a dummy read to kick off actual read

        UchOneByteRead = (UCHAR)DeviceCtxPtr->RegistersPtr->DataIOReg;

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                    "ControllerTransferDataMultp(R) Dummy read[0] %02Xh",
                    UchOneByteRead);

        // wait for dummy read to complete - expect ICF bit to become set

        for(n = 0; n < timeoutMax &&
                (DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_ICF_MASK) == 0;
                n++)
        {
            KeStallExecutionProcessor(1);
        }

        // check if hardware timed out

        if((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_ICF_MASK) == 0) {

            TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                        "ControllerTransferDataMultp(R) dummy read i2c ICF timeout! n=%ld",n);

            status = RequestPtr->Status = STATUS_IO_TIMEOUT;
            goto ControllerTransferDataMultpEnd;
        }
    }

    // perform all reads or writes in current request

    for(size_t j = 0; j < RequestPtr->Length; j++)
    {
        if (RequestPtr->Direction == SpbTransferDirectionToDevice) {

            // Write

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                        "Ready to write byte for address %lXh",
                        DeviceCtxPtr->CurrentTargetPtr->Settings.Address);

            // Obtain data byte

            status = PbcRequestGetByte(RequestPtr, j, &UchOneByteToWrite);

            if(STATUS_SUCCESS != status) {

                // error - cannot obtain data byte from request

                TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                            "ControllerTransferDataMultp(W) error %Xh from request get byte",
                            status);
                break;
            }

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                        "ControllerTransferDataMultp(W) Will write byte[%Iu] %02Xh",
                        j,
                        UchOneByteToWrite);

            // have one data byte.
            // now write it to DATAIO register to actually send it out

            DeviceCtxPtr->RegistersPtr->DataIOReg = (USHORT)UchOneByteToWrite;

            // wait for transfer complete - expect IIF bit to become set

            for(n = 0; n < timeoutMax &&
                (DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IIF_MASK) == 0;
                n++)
            {
                KeStallExecutionProcessor(1);
            }

            // check if hardware timed out

            if((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IIF_MASK) == 0) {

                TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                            "ControllerTransferDataMultp(W) i2c IIF timeout! n=%ld", n);

                status = RequestPtr->Status = STATUS_IO_TIMEOUT;
                break;
            }

            // evaluate post tranfser condition
            // once write is complete an interrupt IIF bit will be set as well as ICF data complete

            DeviceCtxPtr->RegistersPtr->StatusReg = DeviceCtxPtr->RegistersPtr->StatusReg & ~IMX_I2C_STA_REG_IIF_MASK;

            // now check IAL

            if(DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IAL_MASK) {

                TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                            "ControllerTransferDataMultp(W) i2c data write arbitration lost");

                // clear IAL bit

                DeviceCtxPtr->RegistersPtr->StatusReg = DeviceCtxPtr->RegistersPtr->StatusReg & ~IMX_I2C_STA_REG_IAL_MASK;

                DeviceCtxPtr->RegistersPtr->StatusReg = DeviceCtxPtr->RegistersPtr->StatusReg & ~IMX_I2C_STA_REG_IIF_MASK;

                status = RequestPtr->Status=STATUS_NO_SUCH_DEVICE;
                break;
            }

            // check for no ack

            if(DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_RXAK_MASK) {

                TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                            "ControllerTransferDataMultp(W) i2c data write not acknowledged!");

                status = STATUS_NO_SUCH_DEVICE;
                RequestPtr->Status = STATUS_SUCCESS; // to satisfy TAEFF partial write test
                break;
            }

        } else {
            // Read

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                        "Ready to read byte[%Iu] from address %Xh",
                        j,
                        DeviceCtxPtr->CurrentTargetPtr->Settings.Address);

            // do not generate ACK for the last-2 byte

            if(RequestPtr->Length >= 2 && j == RequestPtr->Length-2) {

                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                            "ControllerTransferDataMultp(R) 2 bytes left to Read - set TXAK=1");

                DeviceCtxPtr->RegistersPtr->ControlReg = DeviceCtxPtr->RegistersPtr->ControlReg | IMX_I2C_CTRL_REG_TXAK_MASK;

            } else {

                // when reading last byte leave TXAK bit as is

                if(j != RequestPtr->Length-1) {

                    DeviceCtxPtr->RegistersPtr->ControlReg = DeviceCtxPtr->RegistersPtr->ControlReg & ~IMX_I2C_CTRL_REG_TXAK_MASK;
                }
            }

            // nxp application note AN4481

            if(j == RequestPtr->Length-1)
            {
                DeviceCtxPtr->RegistersPtr->ControlReg=DeviceCtxPtr->RegistersPtr->ControlReg & ~IMX_I2C_CTRL_REG_MSTA_MASK;
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                            "ControllerTransferDataMultp(R) Last byte to Read - set STOP=1");
            }

            // read one byte from i2c data register

            UchOneByteRead = (UCHAR)DeviceCtxPtr->RegistersPtr->DataIOReg;

            // let read operation complete

            for(n = 0; n < timeoutMax &&
                (DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_ICF_MASK) == 0;
                n++) {
                KeStallExecutionProcessor(1);
            }

            // check if read timed out

            if((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_ICF_MASK) == 0) {

                TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                            "ControllerTransferDataMultp(R) i2c ICF timeout! n=%ld", n);

                status = RequestPtr->Status = STATUS_IO_TIMEOUT;
                break;
            }

            // so we have ICF, check for error conditions such as IAL

            if(DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IAL_MASK) {

                TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                            "ControllerTransferDataMultp(R) i2c data read arbitration lost");

                // clear IAL bit

                DeviceCtxPtr->RegistersPtr->StatusReg = DeviceCtxPtr->RegistersPtr->StatusReg & ~IMX_I2C_STA_REG_IAL_MASK;

                DeviceCtxPtr->RegistersPtr->StatusReg = DeviceCtxPtr->RegistersPtr->StatusReg & ~IMX_I2C_STA_REG_IIF_MASK;

                status = RequestPtr->Status = STATUS_NO_SUCH_DEVICE;
                break;
            }

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                        "ControllerTransferDataMultp(R) Data[%Iu] read %02Xh",
                        j,
                        UchOneByteRead);

            status = PbcRequestSetByte(RequestPtr, j, UchOneByteRead);

            if(STATUS_SUCCESS != status) {

                TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                            "ControllerTransferDataMultp(R) error %Xh from request set byte",
                            status);
                break;
            }

        } // end IF Read or Write

        bytesToTransfer += 1;

    }  // end FOR loop reads or write

    transferStatus = status;

    if(RequestPtr->SequencePosition == SpbRequestSequencePositionSingle ||
            RequestPtr->SequencePosition == SpbRequestSequencePositionLast ||
            STATUS_SUCCESS != status ) {

            // for reads - Stop is already made prior to last byte except when imcomplete read occurred due to an error

            if(RequestPtr->Direction == SpbTransferDirectionFromDevice) {
                // wait for Stop condition to complete.

                for(n = 0;
                    n < timeoutMax && ((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IBB_MASK)!=0);
                    n++) {
                    KeStallExecutionProcessor(1);
                }

                // check if Stop condition timed out

                if((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IBB_MASK) != 0) {
                    TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                        "ControllerTransferDataMultp(R) i2c IBB timeout! n=%ld", n);

                    status = RequestPtr->Status = STATUS_IO_TIMEOUT;
                }
            }

            // for writes - after last byte OR when imcomplete write occurred due to an error

            else if(RequestPtr->Direction == SpbTransferDirectionToDevice ||
                STATUS_SUCCESS != status ) {

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                        "ControllerTransferDataMultp() last in sequence, will stop");

            // send Stop after last byte in sequence.

            status = ControllerGenerateStop(DeviceCtxPtr);

            if(STATUS_SUCCESS != status) {

                TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                           "ControllerTransferDataMultp(W) error %Xh from stop", status);
            }
        }
        //  disable i2c block

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                    "ControllerTransferDataMultp() last i/o in sequence completed, disable i2c");

        DeviceCtxPtr->RegistersPtr->ControlReg = (USHORT)0;
    }

    // Update request context with bytes transferred.

    RequestPtr->Information += bytesToTransfer;
    status = transferStatus;

    // make note of partial transfer

    if(transferStatus != STATUS_SUCCESS && bytesToTransfer != 0) {

            TraceEvents(TRACE_LEVEL_WARNING, TRACE_CTRLR,
                    "ControllerTransferDataMultp(WARN) partial transfer! only %Id of %Id bytes succeeded.",
                        bytesToTransfer, RequestPtr->Length);
    }

ControllerTransferDataMultpEnd:

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "--ControllerTransferDataMultp()=%Xh",
                status);

    return status;
}

/*++

  Routine Description:

    This routine completes a data transfer. Unless there are
    more transfers remaining in the sequence, the request is
    completed.

  Arguments:

    DeviceCtxPtr - a pointer to the device context
    RequestPtr - a pointer to the PBC request context
    AbortSequence - specifies whether the driver should abort the
        ongoing sequence or begin the next transfer

  Return Value:

    None. The request is completed asynchronously.

--*/
_Use_decl_annotations_
VOID ControllerCompleteTransfer(
    PDEVICE_CONTEXT DeviceCtxPtr,
    PPBC_REQUEST RequestPtr,
    BOOLEAN AbortSequence)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "++ControllerCompleteTransfer()");

    NT_ASSERT(DeviceCtxPtr != NULL);
    NT_ASSERT(RequestPtr != NULL);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
        "Transfer (index %lu) %s with %Iu bytes for address 0x%lx "
        "(SPBREQUEST %p)",
        RequestPtr->TransferIndex,
        NT_SUCCESS(RequestPtr->Status) ? "complete" : "error",
        RequestPtr->Information,
        DeviceCtxPtr->CurrentTargetPtr->Settings.Address,
        RequestPtr->SpbRequest);

    // Update request context with information from this transfer.

    RequestPtr->TotalInformation += RequestPtr->Information;
    RequestPtr->Information = 0;

    // Check if there are more transfers in the sequence.

    if(FALSE==AbortSequence) {

        RequestPtr->TransferIndex++;

        if (RequestPtr->TransferIndex < RequestPtr->TransferCount) {

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                        "Configure request for the next transfer %u",
                        RequestPtr->TransferIndex);

            // Configure the request for the next transfer.

            RequestPtr->Status = PbcRequestConfigureForIndex(RequestPtr,
                                                            RequestPtr->TransferIndex);

            if (NT_SUCCESS(RequestPtr->Status)) {

                // Configure controller and kick-off next read or write

                PbcRequestDoTransfer(DeviceCtxPtr, RequestPtr);
                goto exit;
            }

        } else {

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                        "No more requests in sequence.");
        }
    }

    // If not already cancelled, unmark request cancellable.

    if (RequestPtr->Status != STATUS_CANCELLED) {

        NTSTATUS cancelStatus;
        cancelStatus = WdfRequestUnmarkCancelable(RequestPtr->SpbRequest);

        if (!NT_SUCCESS(cancelStatus)) {

            // WdfRequestUnmarkCancelable should only fail if the request
            // has already been or is about to be cancelled. If it does fail
            // the request must NOT be completed - the cancel callback will do this.

            NT_ASSERTMSG("WdfRequestUnmarkCancelable should only fail if"
                        " the request has already been or is about to be cancelled",
                        cancelStatus == STATUS_CANCELLED);

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                        "Failed to unmark SPBREQUEST %p as cancelable - %Xh",
                        RequestPtr->SpbRequest,
                        cancelStatus);

            goto exit;
        }
    }

    // Clear the target's current request. This will prevent
    // the request context from being accessed once the request
    // is completed (and the context is invalid).

    DeviceCtxPtr->CurrentTargetPtr->CurrentRequestPtr = NULL;

    // Clear the controller's current target if any of
    //   1. request is type sequence
    //   2. request position is single
    //      (did not come between lock/unlock)
    // Otherwise wait until unlock.

    if ((RequestPtr->Type == SpbRequestTypeSequence) ||
        (RequestPtr->SequencePosition == SpbRequestSequencePositionSingle)) {

        DeviceCtxPtr->CurrentTargetPtr = NULL;
    }

    // Mark the IO complete. Request not completed here.

    RequestPtr->bIoComplete = TRUE;

exit:

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR, "--ControllerCompleteTransfer()");
}

/*++

  Routine Description:

    This routine is invoked whenever the controller is to
    be locked for a single target. The request is only completed
    if there is an error configuring the transfer.

  Arguments:

    SpbController - a handle to the framework device object
        representing an SPB controller
    SpbTarget - a handle to the SPBTARGET object
    SpbRequest - a handle to the SPBREQUEST object

  Return Value:

    None.  The request is completed synchronously.

--*/
_Use_decl_annotations_
VOID OnControllerLock(
    WDFDEVICE SpbController,
    SPBTARGET SpbTarget,
    SPBREQUEST SpbRequest
    )
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "++OnControllerLock()");

    PDEVICE_CONTEXT deviceCtxPtr = GetDeviceContext(SpbController);
    PPBC_TARGET targetPtr = GetTargetContext(SpbTarget);

    NT_ASSERT(deviceCtxPtr  != NULL);
    NT_ASSERT(targetPtr  != NULL);

    // Acquire the device lock.

    WdfSpinLockAcquire(deviceCtxPtr->Lock);

    // Assign current target.

    NT_ASSERT(deviceCtxPtr->CurrentTargetPtr == NULL);

    deviceCtxPtr->CurrentTargetPtr = targetPtr;

    WdfSpinLockRelease(deviceCtxPtr->Lock);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE,
                "Controller locked for SPBTARGET %p at address 0x%lx (WDFDEVICE %p)",
                targetPtr->SpbTarget,
                targetPtr->Settings.Address,
                deviceCtxPtr->WdfDevice);

    // Complete lock request.

    SpbRequestComplete(SpbRequest, STATUS_SUCCESS);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "--OnControllerLock()");
    return;
}

/*++

  Routine Description:

    This routine is invoked whenever the controller is to
    be unlocked for a single target. The request is only completed
    if there is an error configuring the transfer.

  Arguments:

    SpbController - a handle to the framework device object
        representing an SPB controller
    SpbTarget - a handle to the SPBTARGET object
    SpbRequest - a handle to the SPBREQUEST object

  Return Value:

    None.  The request is completed asynchronously.

--*/
_Use_decl_annotations_
VOID OnControllerUnlock(
    WDFDEVICE SpbController,
    SPBTARGET SpbTarget,
    SPBREQUEST SpbRequest
    )
{
    PDEVICE_CONTEXT deviceCtxPtr = GetDeviceContext(SpbController);
    PPBC_TARGET targetPtr = GetTargetContext(SpbTarget);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "++OnControllerUnlock()");

    NT_ASSERT(deviceCtxPtr != NULL);
    NT_ASSERT(targetPtr != NULL);

    // Acquire the device lock.

    WdfSpinLockAcquire(deviceCtxPtr->Lock);

    // Check if there is an active sequence: check bit 7 ICF, 0 means transfer in progress
    // and if so perform any action necessary to stop the transfer in process.

    // Remove current target.

    NT_ASSERT(deviceCtxPtr->CurrentTargetPtr == targetPtr);

    deviceCtxPtr->CurrentTargetPtr = NULL;

    WdfSpinLockRelease(deviceCtxPtr->Lock);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
                "Controller unlocked for SPBTARGET %p at address 0x%lx (WDFDEVICE %p)",
                targetPtr->SpbTarget,
                targetPtr->Settings.Address,
                deviceCtxPtr->WdfDevice);

    // Complete unlock request.

    SpbRequestComplete(SpbRequest, STATUS_SUCCESS);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "--OnControllerUnlock()");
    return;
}

/*++

  Routine Description:

    This routine generates i2c START and sends out slave address.

  Arguments:

    pDevice - device object
    pRequest - a handle to the SPBREQUEST object

  Return Value:

    Success if all okay, otherwise error code

--*/
_Use_decl_annotations_
NTSTATUS ControllerGenerateStart(
    PDEVICE_CONTEXT DeviceCtxPtr,
    PPBC_REQUEST RequestPtr
    )
{
    int n = 0;
    int timeoutMax;
    NTSTATUS status = STATUS_SUCCESS;
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR, "++ControllerGenerateStart()");

    timeoutMax = ( 10 * 1000000 ) / DeviceCtxPtr->CurrentTargetPtr->Settings.ConnectionSpeed; // 100us at 100 kHz

    // expect i2c bus be Not busy.
    // check if i2c bus is busy - wait until i2c bus becomes not busy

    for(n = 0; n < timeoutMax &&
        (DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IBB_MASK) != 0;
        n++) {
        KeStallExecutionProcessor(1);
    }

    // check if hardware timed out

    if((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IBB_MASK) != 0) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateStart(ERR) bus still busy, cannot proceed! n=%ld", n);

        status = STATUS_DEVICE_BUSY;
        goto ControllerGenerateStart;
    }

    DeviceCtxPtr->RegistersPtr->StatusReg = (USHORT)0;

    // select bus master mode

    DeviceCtxPtr->RegistersPtr->ControlReg = DeviceCtxPtr->RegistersPtr->ControlReg | IMX_I2C_CTRL_REG_MSTA_MASK;

    // select transmit mode

    DeviceCtxPtr->RegistersPtr->ControlReg = DeviceCtxPtr->RegistersPtr->ControlReg | IMX_I2C_CTRL_REG_MTX_MASK;

#if (defined(DBG) || defined(DEBUG))
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "S AdrReg=%04Xh FDReg=%04Xh CtlReg=%04Xh StaReg=%04Xh",
                DeviceCtxPtr->RegistersPtr->AddressReg,
                DeviceCtxPtr->RegistersPtr->FreqDivReg,
                DeviceCtxPtr->RegistersPtr->ControlReg,
                DeviceCtxPtr->RegistersPtr->StatusReg);
#endif

    // wait until bus becomes busy - then we gained bus master state, i.e. start is succssessful
    // Note: WinCE waits longer

    for(n = 0; n < timeoutMax * 10 &&
        (DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IBB_MASK) == 0;
        n++) {
        KeStallExecutionProcessor(1);
    };

    // check if hardware timed out

    if((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IBB_MASK) == 0) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateStart(ERR) START timed out. n=%ld", n);

        status=STATUS_IO_TIMEOUT;
        goto ControllerGenerateStart;
    }

    // check if master mode was not lost

    if((DeviceCtxPtr->RegistersPtr->ControlReg & IMX_I2C_CTRL_REG_MSTA_MASK) == 0) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateStart(ERR) after Start master mode is lost!");

        status = STATUS_DEVICE_DATA_ERROR;
        goto ControllerGenerateStart;
    }

    // check if IIF==1. IIF should be set after byte is trasnmitted, not after start is signalled.

    if((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IIF_MASK) != 0) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateStart(ERR) after START IIF bit is set");

        status = STATUS_DEVICE_DATA_ERROR;
        goto ControllerGenerateStart;
    }

    // all clear to proceed sending out slave address

    UCHAR uchSlaveAddress = (UCHAR)(DeviceCtxPtr->CurrentTargetPtr->Settings.Address << 1);
    uchSlaveAddress = uchSlaveAddress |
            ( RequestPtr->Direction == SpbTransferDirectionToDevice ? (UCHAR)0 : (UCHAR)1 );

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "ControllerGenerateStart() i2c slave addr %02Xh",
                uchSlaveAddress);

    // send out first byte - slave address
    // write calling address to data register. ICF will now become 0

    DeviceCtxPtr->RegistersPtr->DataIOReg = uchSlaveAddress;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "ControllerGenerateStart() slave address written, awaiting IIF");

    for(n = 0; n < timeoutMax &&
        (DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IIF_MASK) == 0;
        n++) {
        KeStallExecutionProcessor(1);
    }

    // check if hardware timed out

    if((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IIF_MASK) == 0) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateStart() i2c IIF timeout! n=%ld", n);

        RequestPtr->Status = status = STATUS_IO_TIMEOUT;
        goto ControllerGenerateStart;
    }

    DeviceCtxPtr->RegistersPtr->StatusReg = (USHORT)0;

    // IIF is now set.
    // check if slave acknowledged this address

    if(DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_RXAK_MASK) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateStart() slave address was not acknowledged");

        RequestPtr->Status = status = STATUS_NO_SUCH_DEVICE;
        goto ControllerGenerateStart;
    }

ControllerGenerateStart:

    // clean up error condition if occurred

    if(STATUS_SUCCESS != status) {

        DeviceCtxPtr->RegistersPtr->StatusReg = (USHORT)0;

        if(DeviceCtxPtr->RegistersPtr->ControlReg & IMX_I2C_CTRL_REG_MSTA_MASK) {

            // this is error condition clean up. Therefore we don't care much if stop will fail or not

            ControllerGenerateStop(DeviceCtxPtr);
        }
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "--ControllerGenerateStart()=%Xh",
                status);
    return status;
}

/*++

  Routine Description:

    This routine generates i2c Repeated START and sends out slave address.

  Arguments:

    pDevice - device object
    pRequest - a handle to the SPBREQUEST object

  Return Value:

    Success if all okay, otherwise error code

--*/
_Use_decl_annotations_
NTSTATUS ControllerGenerateRepeatedStart(
    PDEVICE_CONTEXT DeviceCtxPtr,
    PPBC_REQUEST RequestPtr
    )
{
    int n = 0;
    int timeoutMax = 25;
    NTSTATUS status = STATUS_SUCCESS;
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR, "++ControllerGenerateRepeatedStart()");

    timeoutMax = ( 10 * 1000000 ) / DeviceCtxPtr->CurrentTargetPtr->Settings.ConnectionSpeed;

    // set RSTA bit

    DeviceCtxPtr->RegistersPtr->ControlReg = DeviceCtxPtr->RegistersPtr->ControlReg | IMX_I2C_CTRL_REG_RSTA_MASK;

    // Delay for 2 pg_clk_patref clocks.
    // Since only 156ns delay is needed, use minimum possible duration of 1us

    KeStallExecutionProcessor(1);

    // switch to transmit mode

    DeviceCtxPtr->RegistersPtr->ControlReg = DeviceCtxPtr->RegistersPtr->ControlReg | IMX_I2C_CTRL_REG_MTX_MASK;

    // Delay for 2 SCL clocks

    KeStallExecutionProcessor((timeoutMax*2)/10);

    // check repeated start succeeded

    if((DeviceCtxPtr->RegistersPtr->ControlReg & IMX_I2C_CTRL_REG_MSTA_MASK) == 0) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateRepeatedStart(ERR) master status lost!");

        status=STATUS_DEVICE_BUSY;
        goto ControllerGenerateRepeatedStartErr;
    }

    DeviceCtxPtr->RegistersPtr->StatusReg = (USHORT)0;

    if((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IIF_MASK) != 0) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateRepeatedStart(ERR) IIF was set");

        status = STATUS_DEVICE_BUSY;
        goto ControllerGenerateRepeatedStartErr;
    }

    // all clear to proceed sending out slave address

    UCHAR uchSlaveAddress = (UCHAR)(DeviceCtxPtr->CurrentTargetPtr->Settings.Address << 1);

    uchSlaveAddress = uchSlaveAddress |
        ( RequestPtr->Direction == SpbTransferDirectionToDevice ? 0 : 1 );

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "ControllerGenerateRepeatedStart() i2c slave addr %02Xh", uchSlaveAddress);

    // send out first byte - slave address
    // write calling address to data register. ICF will now become 0

    DeviceCtxPtr->RegistersPtr->DataIOReg = uchSlaveAddress;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "ControllerGenerateRepeatedStart() slave address written, awaiting IIF");

    // wait for IIF to be set

    for(n = 0; n < timeoutMax &&
        (DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IIF_MASK)==0;
        n++) {
        KeStallExecutionProcessor(1);
    }

    // check if hardware timed out

    if((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IIF_MASK)==0) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateRepeatedStart() i2c IIF timeout! n=%ld", n);

        status = STATUS_IO_TIMEOUT;
        goto ControllerGenerateRepeatedStartErr;
    };

    DeviceCtxPtr->RegistersPtr->StatusReg = (USHORT)0;

    // IIF is set. check for error conditions

    if(DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_RXAK_MASK) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateRepeatedStart() slave address was not acknoledged");

        RequestPtr->Status = STATUS_NO_SUCH_DEVICE;
        goto ControllerGenerateRepeatedStartErr;
    }

ControllerGenerateRepeatedStartErr:

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR,
                "--ControllerGenerateRepeatedStart()=%Xh", status);
    return status;
}

/*++

  Routine Description:

    This routine generates i2c STOP.

  Arguments:

    pDevice - device object
    pRequest - a handle to the SPBREQUEST object

  Return Value:

    Success if all okay, otherwise error code

--*/
_Use_decl_annotations_
NTSTATUS ControllerGenerateStop(PDEVICE_CONTEXT DeviceCtxPtr)
{
    int n = 0;
    int timeoutMax = 25;
    NTSTATUS status = STATUS_SUCCESS;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR, "++ControllerGenerateStop()");

    // Note: our default is ten*(10) clock cycles (e.g. 100us at 100 kHz).

    timeoutMax = ( 10 * 1000000 ) / DeviceCtxPtr->CurrentTargetPtr->Settings.ConnectionSpeed;

    // bus must be busy to generate stop

    if((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IBB_MASK)==0) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateStop(ERR) bus not busy - cannot proceed");

        status=STATUS_DEVICE_BUSY;
        goto ControllerGenerateStopEnd;
    }

    if(DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IAL_MASK) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateStop(ERR) IAL detected");
    }

    if((DeviceCtxPtr->RegistersPtr->ControlReg & IMX_I2C_CTRL_REG_MSTA_MASK)==0) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateStop(ERR) is not Master, cannot proceed");

        status = STATUS_DEVICE_BUSY;
        goto ControllerGenerateStopEnd;
    }

    // remove master status

    DeviceCtxPtr->RegistersPtr->ControlReg=DeviceCtxPtr->RegistersPtr->ControlReg & ~IMX_I2C_CTRL_REG_MSTA_MASK;

    // wait for stop condition to complete - bus must become free

    for(n = 0; n < timeoutMax &&
        ((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IBB_MASK)!=0);
        n++) {
        KeStallExecutionProcessor(1);
    }

    if((DeviceCtxPtr->RegistersPtr->StatusReg & IMX_I2C_STA_REG_IBB_MASK)!=0) {

        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CTRLR,
                    "ControllerGenerateStop(ERROR) Stop bit timed out! n=%ld", n);

        status = STATUS_DEVICE_BUSY;

        goto ControllerGenerateStopEnd;
    }

ControllerGenerateStopEnd:

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CTRLR, "--ControllerGenerateStop()=%Xh",status);
    return status;
}