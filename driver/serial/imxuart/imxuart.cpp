// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//    imxuart.cpp
//
// Abstract:
//
//    This module contains the iMX UART driver implementation which uses
//    the SerCx2 framework.
//
#include "precomp.h"
#include "imxuarthw.h"
#include "imxuart.h"
#include "HalExtiMXDmaCfg.h"

#include "trace.h"
#include "imxuart.tmh"

IMX_UART_NONPAGED_SEGMENT_BEGIN; //==============================================

//
// Placement new and delete operators
//

_Use_decl_annotations_
void* operator new ( size_t, void* Ptr ) throw ()
{
    return Ptr;
}

void operator delete ( void*, void* ) throw ()
{}

_Use_decl_annotations_
void* operator new[] ( size_t, void* Ptr ) throw ()
{
    return Ptr;
}

void operator delete[] ( void*, void* ) throw ()
{}

_Use_decl_annotations_
ULONG
IMX_UART_RING_BUFFER::EnqueueBytes (
    const UCHAR* InputBufferPtr,
    ULONG InputBufferSize
    )
{
    if (InputBufferSize == 0) return 0;

    const ULONG tail = ReadULongAcquire(&this->TailIndex);
    const ULONG head = this->HeadIndex;
    const ULONG size = this->Size;

    ULONG bytesCopied;
    if (((head + 1) % size) == tail) {
        return 0;
    } else if (head >= tail) {
        // copy from head to the end
        ULONG maxCount = (tail == 0) ? (size - 1) : size;
        ULONG count = min(maxCount - head, InputBufferSize);
        memcpy(
            &this->BufferPtr[head],
            &InputBufferPtr[0],
            count);

        bytesCopied = count;

        // copy from beginning to tail
        if (tail != 0) {
            count = min(tail - 1, InputBufferSize - bytesCopied);
            memcpy(
                &this->BufferPtr[0],
                &InputBufferPtr[bytesCopied],
                count);

            bytesCopied += count;
        }
    } else {
        // copy from head to tail
        ULONG count = min(tail - head - 1, InputBufferSize);
        memcpy(
            &this->BufferPtr[head],
            &InputBufferPtr[0],
            count);

        bytesCopied = count;
    }

    // Update head index
    const ULONG newHeadIndex = (head + bytesCopied) % size;
    WriteULongRelease(&this->HeadIndex, newHeadIndex);
    return bytesCopied;
}

_Use_decl_annotations_
ULONG
IMX_UART_RING_BUFFER::DequeueBytes (
    UCHAR* OutputBufferPtr,
    ULONG OutputBufferSize
    )
{
    const ULONG head = ReadULongAcquire(&this->HeadIndex);
    const ULONG tail = this->TailIndex;
    const ULONG size = this->Size;

    ULONG bytesCopied;
    if (head == tail) {
        return 0;
    } else if (head > tail) {
        // copy from tail to head
        ULONG count = min(head - tail, OutputBufferSize);
        memcpy(
            &OutputBufferPtr[0],
            &this->BufferPtr[tail],
            count);

        bytesCopied = count;
    } else {
        // copy from tail to end
        ULONG count = min(size - tail, OutputBufferSize);
        memcpy(
            &OutputBufferPtr[0],
            &this->BufferPtr[tail],
            count);

        bytesCopied = count;

        // copy from beginning to head
        count = min(head, OutputBufferSize - bytesCopied);
        memcpy(
            &OutputBufferPtr[bytesCopied],
            &this->BufferPtr[0],
            count);

        bytesCopied += count;
    }

    // update tail pointer
    const ULONG newTailIndex = (tail + bytesCopied) % size;
    WriteULongRelease(&this->TailIndex, newTailIndex);
    return bytesCopied;
}

_Use_decl_annotations_
BOOLEAN
IMXUartEvtInterruptIsr (
    WDFINTERRUPT WdfInterrupt,
    ULONG /*MessageID*/
    )
{
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            IMXUartGetInterruptContext(WdfInterrupt);

    IMX_UART_REGISTERS* registersPtr = interruptContextPtr->RegistersPtr;

    const ULONG usr1 = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Usr1);
    const ULONG usr2 = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Usr2);

    const ULONG usr1Masked = usr1 & interruptContextPtr->Usr1EnabledInterruptsMask;
    const ULONG usr2Masked = usr2 & interruptContextPtr->Usr2EnabledInterruptsMask;

    if ((usr1Masked == 0) && (usr2Masked == 0)) {
        IMX_UART_LOG_TRACE(
            "Not claiming interrupt. (usr1 = 0x%lx, usr2 = 0x%lx, Usr1EnabledInterruptsMask = 0x%lx, Usr2EnabledInterruptsMask = 0x%lx)",
            usr1,
            usr2,
            interruptContextPtr->Usr1EnabledInterruptsMask,
            interruptContextPtr->Usr2EnabledInterruptsMask);

        return FALSE;
    }

    bool queueDpc = false;
    const ULONG waitMask = interruptContextPtr->WaitMask;
    ULONG waitEvents = interruptContextPtr->WaitEvents;

    //
    // If DMA RX transfer is not active, service the RX buffer
    //
    if (!IMXUartIsRxDmaActive(interruptContextPtr) &&
        ((usr1Masked & (IMX_UART_USR1_AGTIM | IMX_UART_USR1_RRDY)) != 0)) {

        IMX_UART_RING_BUFFER* rxBufferPtr = &interruptContextPtr->RxBuffer;
        const ULONG tail = ReadULongAcquire(&rxBufferPtr->TailIndex);
        const ULONG oldHead = rxBufferPtr->HeadIndex;
        const ULONG size = rxBufferPtr->Size;

        ULONG head = oldHead;
        ULONG nextHead = (head + 1) % size;
        ULONG bytesRead = 0;
        while (nextHead != tail) {
            const ULONG rxd = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Rxd);
            if ((rxd & IMX_UART_RXD_CHARRDY) == 0) {
                break;
            }

            if ((rxd & IMX_UART_RXD_ERR) != 0) {
                IMX_UART_LOG_ERROR("RX FIFO reported error. (rxd = 0x%lx)", rxd);

                if ((rxd & IMX_UART_RXD_OVRRUN) != 0) {
                    interruptContextPtr->CommStatusErrors |= SERIAL_ERROR_OVERRUN;
                }

                if ((rxd & IMX_UART_RXD_FRMERR) != 0) {
                    interruptContextPtr->CommStatusErrors |= SERIAL_ERROR_FRAMING;
                }

                if ((rxd & IMX_UART_RXD_BRK) != 0) {
                    interruptContextPtr->CommStatusErrors |= SERIAL_ERROR_BREAK;
                }

                if ((rxd & IMX_UART_RXD_PRERR) != 0) {
                    interruptContextPtr->CommStatusErrors |= SERIAL_ERROR_PARITY;
                }

                waitEvents |= (waitMask & SERIAL_EV_ERR);
            }

            rxBufferPtr->BufferPtr[head] = static_cast<UCHAR>(rxd);

            ++bytesRead;
            head = nextHead;
            nextHead = (nextHead + 1) % size;
        }

        WriteULongRelease(&rxBufferPtr->HeadIndex, head);

        IMX_UART_LOG_TRACE("Read %lu bytes from FIFO.", bytesRead);

        //
        // If the intermediate buffer is full, disable the RRDY and AGTIM
        // interrupts so they do not continue asserting
        //
        if (nextHead == tail) {
            IMX_UART_LOG_WARNING("Intermediate receive buffer overflowed, disabling RRDY and AGTIM.");

            interruptContextPtr->CommStatusErrors |= SERIAL_ERROR_QUEUEOVERRUN;
            waitEvents |= (waitMask & SERIAL_EV_ERR);

            interruptContextPtr->Ucr1Copy &= ~IMX_UART_UCR1_RRDYEN;
            interruptContextPtr->Usr1EnabledInterruptsMask &= ~IMX_UART_USR1_RRDY;
            WRITE_REGISTER_NOFENCE_ULONG(
                &registersPtr->Ucr1,
                interruptContextPtr->Ucr1Copy);

            //
            // Disable AGTIM interrupt
            //
            interruptContextPtr->Ucr2Copy &= ~IMX_UART_UCR2_ATEN;
            interruptContextPtr->Usr1EnabledInterruptsMask &= ~IMX_UART_USR1_AGTIM;
            WRITE_REGISTER_NOFENCE_ULONG(
                &registersPtr->Ucr2,
                interruptContextPtr->Ucr2Copy);

            WRITE_REGISTER_NOFENCE_ULONG(
                &registersPtr->Usr1,
                IMX_UART_USR1_AGTIM);
        }

        //
        // If there are bytes available and RX notifications are enabled,
        // queue the receive ready notification
        //
        if ((head != tail) &&
            (interruptContextPtr->RxState ==
             IMX_UART_STATE::WAITING_FOR_INTERRUPT)) {

            IMX_UART_LOG_TRACE("Bytes available in RX buffer, going to WAITING_FOR_DPC state.");

            interruptContextPtr->RxState = IMX_UART_STATE::WAITING_FOR_DPC;

            queueDpc = true;
        }

        //
        // Acknowledge AGTIM interrupt
        //
        if ((usr1Masked & IMX_UART_USR1_AGTIM) != 0) {
            IMX_UART_LOG_TRACE("Acknowledging AGTIM interrupt.");
            WRITE_REGISTER_NOFENCE_ULONG(
                &registersPtr->Usr1,
                IMX_UART_USR1_AGTIM);
        }
    }

    if ((usr2 & IMX_UART_USR2_RDR) != 0) {
        waitEvents |= (waitMask & SERIAL_EV_RXCHAR);
    }

    //
    // Take bytes from the intermediate buffer and put them in the TX FIFO
    //
    if (!IMXUartIsTxDmaActive(interruptContextPtr)) {
        IMX_UART_RING_BUFFER* txBufferPtr = &interruptContextPtr->TxBuffer;
        const ULONG head = ReadULongAcquire(&txBufferPtr->HeadIndex);
        ULONG tail = txBufferPtr->TailIndex;
        const ULONG size = txBufferPtr->Size;

        if (tail != head) {
            do {
                const ULONG uts = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Uts);
                if ((uts & IMX_UART_UTS_TXFULL) != 0) {
                    break;
                }

                WRITE_REGISTER_NOFENCE_ULONG(
                    &registersPtr->Txd,
                    txBufferPtr->BufferPtr[tail]);

                tail = (tail + 1) % size;
            } while (tail != head);

            WriteULongRelease(&txBufferPtr->TailIndex, tail);
        }

        //
        // If we drained the intermediate buffer, mask the TX ready interrupt
        // so it does not cause a storm. The interrupt will be reenabled
        // by WriteBuffer when more bytes are put in the intermediate buffer.
        //
        if ((tail == head) && ((usr1Masked & IMX_UART_USR1_TRDY) != 0)) {
            IMX_UART_LOG_TRACE("TX intermediate buffer was emptied, masking TRDY interrupt.");
            interruptContextPtr->Ucr1Copy &= ~IMX_UART_UCR1_TRDYEN;
            interruptContextPtr->Usr1EnabledInterruptsMask &= ~IMX_UART_USR1_TRDY;

            WRITE_REGISTER_NOFENCE_ULONG(
                &registersPtr->Ucr1,
                interruptContextPtr->Ucr1Copy);
        }

        //
        // If the TX ready notification is enabled and the intermediate buffer
        // contains fewer bytes than the threshold, queue a DPC to request
        // more bytes.
        //
        if (interruptContextPtr->TxState ==
            IMX_UART_STATE::WAITING_FOR_INTERRUPT) {

            const ULONG count = IMX_UART_RING_BUFFER::Count(head, tail, size);
            if (count <= interruptContextPtr->TxDpcThreshold) {
                IMX_UART_LOG_TRACE(
                    "Intermediate TX buffer is below threshold and notifications are enabled, firing ready notification. (count = %lu, interruptContextPtr->TxDpcThreshold = %lu)",
                    count,
                    interruptContextPtr->TxDpcThreshold);

                interruptContextPtr->TxState = IMX_UART_STATE::WAITING_FOR_DPC;

                queueDpc = true;
            }
        }
    }

    //
    // If FIFO empty request is pending and enabled, queue DPC
    //
    if ((usr2Masked & IMX_UART_USR2_TXFE) != 0) {
        IMX_UART_LOG_TRACE(
            "TX FIFO empty interrupt occurred (usr2Masked = 0x%lx)",
            usr2Masked);

        if (interruptContextPtr->TxDrainState ==
            IMX_UART_STATE::WAITING_FOR_INTERRUPT) {

            IMX_UART_LOG_TRACE(
                "Drain complete notification is enabled - queuing DPC. (usr2Masked = 0x%lx)",
                usr2Masked);

            interruptContextPtr->TxDrainState = IMX_UART_STATE::WAITING_FOR_DPC;

            queueDpc = true;
        }

        if (interruptContextPtr->TxPurgeState ==
            IMX_UART_STATE::WAITING_FOR_INTERRUPT) {

            IMX_UART_LOG_TRACE(
                "Purge complete notification is enabled - queuing DPC. (usr2Masked = 0x%lx)",
                usr2Masked);

            interruptContextPtr->TxPurgeState = IMX_UART_STATE::WAITING_FOR_DPC;

            queueDpc = true;
        }

        if (interruptContextPtr->TxDmaDrainState ==
            IMX_UART_STATE::WAITING_FOR_INTERRUPT) {

            IMX_UART_LOG_TRACE(
                "TX DMA: Drain complete notification is enabled - queuing DPC. "
                "(usr2Masked = 0x%lx)",
                usr2Masked);

            interruptContextPtr->TxDmaDrainState = IMX_UART_STATE::WAITING_FOR_DPC;

            queueDpc = true;
        }

        waitEvents |= (waitMask & SERIAL_EV_TXEMPTY);

        //
        // Disable TXFE interrupt
        //
        interruptContextPtr->Ucr1Copy &= ~IMX_UART_UCR1_TXMPTYEN;
        interruptContextPtr->Usr2EnabledInterruptsMask &= ~IMX_UART_USR2_TXFE;
        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Ucr1,
            interruptContextPtr->Ucr1Copy);
    }

    //
    // Is a break active?
    //
    if ((usr2Masked & IMX_UART_USR2_BRCD) != 0) {
        waitEvents |= (waitMask & SERIAL_EV_BREAK);

        //
        // Clear BRCD bit.
        //
        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Usr2,
            IMX_UART_USR2_BRCD);
    }

    //
    // Did the CTS line change state?
    //
    if ((usr2Masked & IMX_UART_USR2_RTSF) != 0) {
        waitEvents |= (waitMask & SERIAL_EV_CTS);

        //
        // Clear RTSF bit
        //
        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Usr2,
            IMX_UART_USR2_RTSF);
    }

    interruptContextPtr->WaitEvents = waitEvents;
    if (waitEvents != 0) {
        queueDpc = true;
    }

    if (queueDpc) {
        WdfInterruptQueueDpcForIsr(WdfInterrupt);
    }

    return TRUE;
}

_Use_decl_annotations_
VOID
IMXUartEvtInterruptDpc (
    WDFINTERRUPT WdfInterrupt,
    WDFOBJECT /*AssociatedObject*/
    )
{
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            IMXUartGetInterruptContext(WdfInterrupt);

    IMX_UART_LOG_TRACE(
        "DPC was fired. (RxState = %d, RxDmaState = %d, "
        "TxState = %d, TxDrainState = %d, TxPurgeState = %d, "
        "TxDmaState = %d, TxDmaDrainState = %d, WaitEvents = 0x%lx)",
        int(interruptContextPtr->RxState),
        int(interruptContextPtr->RxDmaState),
        int(interruptContextPtr->TxState),
        int(interruptContextPtr->TxDrainState),
        int(interruptContextPtr->TxPurgeState),
        int(interruptContextPtr->TxDmaState),
        int(interruptContextPtr->TxDmaDrainState),
        interruptContextPtr->WaitEvents);

    if (interruptContextPtr->RxState ==
        IMX_UART_STATE::WAITING_FOR_DPC) {

        interruptContextPtr->RxState = IMX_UART_STATE::COMPLETE;

        IMX_UART_LOG_TRACE("Calling SerCx2PioReceiveReady()");
        SerCx2PioReceiveReady(interruptContextPtr->SerCx2PioReceive);
    }

    //
    // If the TX buffer is below the threshold and TX notifications are
    // enabled, call SerCx2PioTransmitReady() to request more bytes
    //
    if (interruptContextPtr->TxState ==
        IMX_UART_STATE::WAITING_FOR_DPC) {

        interruptContextPtr->TxState = IMX_UART_STATE::COMPLETE;

        IMX_UART_LOG_TRACE("Calling SerCx2PioTransmitReady()");
        SerCx2PioTransmitReady(interruptContextPtr->SerCx2PioTransmit);
    }

    if (interruptContextPtr->TxDrainState ==
        IMX_UART_STATE::WAITING_FOR_DPC) {

        interruptContextPtr->TxDrainState = IMX_UART_STATE::COMPLETE;

        IMX_UART_LOG_TRACE("Notifying TX FIFO drain complete");
        SerCx2PioTransmitDrainFifoComplete(interruptContextPtr->SerCx2PioTransmit);
    }

    if (interruptContextPtr->TxPurgeState ==
        IMX_UART_STATE::WAITING_FOR_DPC) {

        interruptContextPtr->TxPurgeState = IMX_UART_STATE::STOPPED;

        IMX_UART_LOG_TRACE(
            "Notifying TX FIFO purge complete. "
            "(interruptContextPtr->PurgedByteCount = %lu)",
            interruptContextPtr->PurgedByteCount);

        SerCx2PioTransmitPurgeFifoComplete(
            interruptContextPtr->SerCx2PioTransmit,
            interruptContextPtr->PurgedByteCount);
    }

    if (interruptContextPtr->TxDmaDrainState ==
        IMX_UART_STATE::WAITING_FOR_DPC) {

        interruptContextPtr->TxDmaDrainState = IMX_UART_STATE::COMPLETE;

        IMX_UART_LOG_TRACE("TX DMA: TX FIFO drain complete");
        IMXUartCompleteCustomTxTransactionRequest(
            interruptContextPtr->TxDmaTransactionContextPtr,
            STATUS_SUCCESS);
    }

    if (interruptContextPtr->WaitEvents != 0) {

        //
        // Interlocked exchange the wait events bitmask with 0
        //
        WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
        const ULONG waitEvents = interruptContextPtr->WaitEvents;

        NT_ASSERT((waitEvents & interruptContextPtr->WaitMask) == waitEvents);

        interruptContextPtr->WaitEvents = 0;
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

        IMX_UART_LOG_TRACE("Completing wait. (waitEvents = 0x%lx)", waitEvents);
        SerCx2CompleteWait(WdfInterruptGetDevice(WdfInterrupt), waitEvents);
    }
}

_Use_decl_annotations_
NTSTATUS IMXUartEvtSerCx2FileOpen (WDFDEVICE WdfDevice)
{
    IMX_UART_LOG_TRACE("Handle opened");

    IMX_UART_DEVICE_CONTEXT* deviceContextPtr =
            IMXUartGetDeviceContext(WdfDevice);

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            deviceContextPtr->InterruptContextPtr;

    NT_ASSERT(interruptContextPtr->RxState == IMX_UART_STATE::STOPPED);
    NT_ASSERT(interruptContextPtr->RxDmaState == IMX_UART_STATE::STOPPED);
    NT_ASSERT(interruptContextPtr->TxState == IMX_UART_STATE::STOPPED);
    NT_ASSERT(interruptContextPtr->TxDrainState == IMX_UART_STATE::STOPPED);
    NT_ASSERT(interruptContextPtr->TxPurgeState == IMX_UART_STATE::STOPPED);
    NT_ASSERT(interruptContextPtr->TxDmaState == IMX_UART_STATE::STOPPED);
    NT_ASSERT(interruptContextPtr->TxDmaDrainState == IMX_UART_STATE::STOPPED);
    NT_ASSERT(interruptContextPtr->CommStatusErrors == 0);
    NT_ASSERT(interruptContextPtr->WaitMask == 0);
    NT_ASSERT(interruptContextPtr->WaitEvents == 0);

    //
    // DMA request lines can be shared, take ownership while the device is
    // opened.
    //
    NTSTATUS status = IMXUartAcquireDmaRequestLineOwnership(
        interruptContextPtr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to acquire DMA request line ownership. "
            "(status = %!STATUS!)",
            status);

        return status;
    }

    if (deviceContextPtr->Initialized) {
        IMX_UART_LOG_TRACE("Restoring UART registers from saved state");
        WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
        NT_ASSERT((interruptContextPtr->Ucr1Copy & IMX_UART_UCR1_UARTEN) != 0);
        IMXUartRestoreHardwareFromShadowedRegisters(interruptContextPtr);
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
    } else {
        IMX_UART_LOG_TRACE("Configuring UART for the first time with reasonable defaults.");

        SERIAL_LINE_CONTROL lineControl = {0};
        SERIAL_HANDFLOW handflow = {0};

        lineControl.StopBits = STOP_BIT_1;
        lineControl.Parity = NO_PARITY;
        lineControl.WordLength = 8;

        status = IMXUartConfigureUart(
                WdfDevice,
                115200,
                &lineControl,
                &handflow,
                false);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "Failed to do initial configuration of UART. (status = %!STATUS!)",
                status);

            return status;
        }
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
VOID IMXUartEvtSerCx2FileClose (WDFDEVICE WdfDevice)
{
    IMX_UART_LOG_TRACE("Handle closed");

    IMX_UART_DEVICE_CONTEXT* deviceContextPtr =
            IMXUartGetDeviceContext(WdfDevice);

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            deviceContextPtr->InterruptContextPtr;

    NTSTATUS status = IMXUartStopRxDma(interruptContextPtr);
    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "IMXUartStopRxDma failed, (status = %!STATUS!)",
            status);
    }
    IMXUartReleaseDmaRequestLineOwnership(interruptContextPtr);

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    IMXUartResetHardwareAndPreserveShadowedRegisters(interruptContextPtr);
    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    //
    // Clear 'send break' and disable all interrupts
    //
    interruptContextPtr->Ucr1Copy &=
        ~(IMX_UART_UCR1_SNDBRK |
          IMX_UART_UCR1_RRDYEN |
          IMX_UART_UCR1_TRDYEN |
          IMX_UART_UCR1_TXMPTYEN);

    interruptContextPtr->Ucr2Copy &= ~(IMX_UART_UCR2_ATEN | IMX_UART_UCR2_RTSEN);
    interruptContextPtr->Ucr4Copy &= ~IMX_UART_UCR4_BKEN;

    interruptContextPtr->Usr1EnabledInterruptsMask = 0;
    interruptContextPtr->Usr2EnabledInterruptsMask = 0;

    interruptContextPtr->RxBuffer.Reset();
    interruptContextPtr->TxBuffer.Reset();

    interruptContextPtr->RxState = IMX_UART_STATE::STOPPED;
    interruptContextPtr->RxDmaState = IMX_UART_STATE::STOPPED;
    interruptContextPtr->TxState = IMX_UART_STATE::STOPPED;
    interruptContextPtr->TxDrainState = IMX_UART_STATE::STOPPED;
    interruptContextPtr->TxPurgeState = IMX_UART_STATE::STOPPED;
    interruptContextPtr->TxDmaState = IMX_UART_STATE::STOPPED;
    interruptContextPtr->TxDmaDrainState = IMX_UART_STATE::STOPPED;
    interruptContextPtr->CommStatusErrors = 0;
    interruptContextPtr->WaitMask = 0;
    interruptContextPtr->WaitEvents = 0;
}

_Use_decl_annotations_
VOID
IMXUartEvtSerCx2SetWaitMask (
    WDFDEVICE WdfDevice,
    WDFREQUEST WdfRequest,
    ULONG WaitMask
    )
{
    IMX_UART_DEVICE_CONTEXT* deviceContextPtr =
            IMXUartGetDeviceContext(WdfDevice);

    const ULONG unsupportedWaitFlags =
            SERIAL_EV_RXFLAG |
            SERIAL_EV_PERR |
            SERIAL_EV_RX80FULL |
            SERIAL_EV_EVENT1 |
            SERIAL_EV_EVENT2;

    if ((WaitMask & unsupportedWaitFlags) != 0) {
        IMX_UART_LOG_ERROR(
            "Wait mask contains unsupported flags. (WaitMask = 0x%lx, unsupportedWaitFlags = 0x%lx)",
            WaitMask,
            unsupportedWaitFlags);

        //
        // Spec explicitly calls for STATUS_INVALID_PARAMETER
        //
        WdfRequestComplete(WdfRequest, STATUS_INVALID_PARAMETER);
        return;
    }

    const ULONG unsupportedButAllowedForWinRTCompatWaitFlags =
            SERIAL_EV_DSR |
            SERIAL_EV_RLSD |
            SERIAL_EV_RING;

    if ((WaitMask & unsupportedButAllowedForWinRTCompatWaitFlags) != 0) {
        IMX_UART_LOG_WARNING(
            "Wait mask contains unsupported flags, but will be allowed for "
            "WinRT compat. "
            "(WaitMask = 0x%lx, "
            "unsupportedButAllowedForWinRTCompatWaitFlags = 0x%lx)",
            WaitMask,
            unsupportedButAllowedForWinRTCompatWaitFlags);
    }

    IMXUartSetWaitMask(deviceContextPtr, WaitMask);
    WdfRequestComplete(WdfRequest, STATUS_SUCCESS);
}

_Use_decl_annotations_
NTSTATUS
IMXUartEvtSerCx2Control (
    WDFDEVICE WdfDevice,
    WDFREQUEST WdfRequest,
    size_t /*OutputBufferLength*/,
    size_t /*InputBufferLength*/,
    ULONG IoControlCode
    )
{
    IMX_UART_DEVICE_CONTEXT* deviceContextPtr =
            IMXUartGetDeviceContext(WdfDevice);

    switch (IoControlCode) {
    case IOCTL_SERIAL_SET_BAUD_RATE:
        IMXUartIoctlSetBaudRate(deviceContextPtr, WdfRequest);
        return STATUS_SUCCESS;

    case IOCTL_SERIAL_GET_BAUD_RATE:
        IMXUartIoctlGetBaudRate(deviceContextPtr, WdfRequest);
        return STATUS_SUCCESS;

    case IOCTL_SERIAL_SET_HANDFLOW:
        IMXUartIoctlSetHandflow(deviceContextPtr, WdfRequest);
        return STATUS_SUCCESS;

    case IOCTL_SERIAL_GET_HANDFLOW:
        IMXUartIoctlGetHandflow(deviceContextPtr, WdfRequest);
        return STATUS_SUCCESS;

    case IOCTL_SERIAL_SET_LINE_CONTROL:
        IMXUartIoctlSetLineControl(deviceContextPtr, WdfRequest);
        return STATUS_SUCCESS;

    case IOCTL_SERIAL_GET_LINE_CONTROL:
        IMXUartIoctlGetLineControl(deviceContextPtr, WdfRequest);
        return STATUS_SUCCESS;

    case IOCTL_SERIAL_GET_PROPERTIES:
        IMXUartIoctlGetProperties(deviceContextPtr, WdfRequest);
        return STATUS_SUCCESS;

    case IOCTL_SERIAL_GET_COMMSTATUS:
        IMXUartIoctlGetCommStatus(deviceContextPtr, WdfRequest);
        return STATUS_SUCCESS;

    case IOCTL_SERIAL_GET_MODEMSTATUS:
        IMXUartIoctlGetModemStatus(deviceContextPtr, WdfRequest);
        return STATUS_SUCCESS;

    case IOCTL_SERIAL_SET_RTS: __fallthrough;
    case IOCTL_SERIAL_CLR_RTS:
        IMXUartIoctlSetClrRts(
            deviceContextPtr,
            WdfRequest,
            IoControlCode == IOCTL_SERIAL_SET_RTS);

        return STATUS_SUCCESS;

    case IOCTL_SERIAL_GET_DTRRTS:
        IMXUartIoctlGetDtrRts(deviceContextPtr, WdfRequest);
        return STATUS_SUCCESS;

    case IOCTL_SERIAL_SET_BREAK_ON: __fallthrough;
    case IOCTL_SERIAL_SET_BREAK_OFF:
        IMXUartIoctlSetBreakOnOff(
            deviceContextPtr,
            WdfRequest,
            IoControlCode == IOCTL_SERIAL_SET_BREAK_ON);

        return STATUS_SUCCESS;

    case IOCTL_SERIAL_GET_MODEM_CONTROL:
        IMXUartIoctlGetModemControl(deviceContextPtr, WdfRequest);
        return STATUS_SUCCESS;

    case IOCTL_SERIAL_RESET_DEVICE: __fallthrough;
    case IOCTL_SERIAL_SET_QUEUE_SIZE: __fallthrough;
    case IOCTL_SERIAL_SET_XOFF: __fallthrough;
    case IOCTL_SERIAL_SET_XON: __fallthrough;
    case IOCTL_SERIAL_GET_WAIT_MASK: __fallthrough;
    case IOCTL_SERIAL_SET_WAIT_MASK: __fallthrough;
    case IOCTL_SERIAL_WAIT_ON_MASK: __fallthrough;
    case IOCTL_SERIAL_PURGE: __fallthrough;
    case IOCTL_SERIAL_GET_CHARS: __fallthrough;
    case IOCTL_SERIAL_SET_CHARS: __fallthrough;
    case IOCTL_SERIAL_XOFF_COUNTER: __fallthrough;
    case IOCTL_SERIAL_CONFIG_SIZE: __fallthrough;
    case IOCTL_SERIAL_GET_STATS: __fallthrough;
    case IOCTL_SERIAL_CLEAR_STATS: __fallthrough;
    case IOCTL_SERIAL_IMMEDIATE_CHAR: __fallthrough;
    case IOCTL_SERIAL_SET_TIMEOUTS: __fallthrough;
    case IOCTL_SERIAL_GET_TIMEOUTS: __fallthrough;
    case IOCTL_SERIAL_APPLY_DEFAULT_CONFIGURATION:
        NT_ASSERT(!"Should have been handled by SerCx2!");
        __fallthrough;

    case IOCTL_SERIAL_SET_DTR: __fallthrough;
    case IOCTL_SERIAL_CLR_DTR: __fallthrough;
    case IOCTL_SERIAL_GET_COMMCONFIG: __fallthrough;
    case IOCTL_SERIAL_SET_COMMCONFIG: __fallthrough;
    case IOCTL_SERIAL_SET_MODEM_CONTROL: __fallthrough;
    case IOCTL_SERIAL_SET_FIFO_CONTROL: __fallthrough;
    default:
        WdfRequestComplete(WdfRequest, STATUS_NOT_SUPPORTED);
        return STATUS_NOT_SUPPORTED;
    }
}

_Use_decl_annotations_
ULONG
IMXUartEvtSerCx2PioReceiveReadBuffer (
    SERCX2PIORECEIVE PioReceive,
    UCHAR* BufferPtr,
    ULONG Length
    )
{
    IMX_UART_RX_CONTEXT* rxContextPtr = IMXUartGetRxContext(PioReceive);
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            rxContextPtr->InterruptContextPtr;

    interruptContextPtr->RxState = IMX_UART_STATE::IDLE;

    //
    // Check if there is any RX data pending from
    // previous DMA transaction?
    //
    ULONG bytesRead = IMXUartPioDequeueDmaBytes(
        interruptContextPtr,
        BufferPtr,
        Length);

    if (bytesRead != 0) {
        IMX_UART_LOG_TRACE(
            "Read bytes from DMA buffer into receive buffer. "
            "(Length = %lu, bytesRead = %lu)",
            Length,
            bytesRead);

        return bytesRead;
    }

    //
    // Read bytes from intermediate buffer into SerCx2's buffer
    //
    bytesRead = interruptContextPtr->RxBuffer.DequeueBytes(BufferPtr, Length);

    IMX_UART_LOG_TRACE(
        "Read bytes from intermediate buffer into receive buffer. (Length = %lu, bytesRead = %lu)",
        Length,
        bytesRead);

    return bytesRead;
}

_Use_decl_annotations_
VOID
IMXUartEvtSerCx2PioReceiveEnableReadyNotification (
    SERCX2PIORECEIVE PioReceive
    )
{
    IMX_UART_RX_CONTEXT* rxContextPtr = IMXUartGetRxContext(PioReceive);
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            rxContextPtr->InterruptContextPtr;

    IMX_UART_REGISTERS* registersPtr = interruptContextPtr->RegistersPtr;

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    if (interruptContextPtr->RxState == IMX_UART_STATE::CANCELLED) {
        IMX_UART_LOG_TRACE("RX notification was already cancelled.");
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
        return;
    }

    NT_ASSERT(
        interruptContextPtr->RxState ==
        IMX_UART_STATE::IDLE);

    //
    // If RRDY has been disabled and there are bytes available, we must
    // complete the request inline because the interrupt might never come.
    //
    const ULONG bufferCount = interruptContextPtr->RxBuffer.Count();
    if (((interruptContextPtr->Ucr1Copy & IMX_UART_UCR1_RRDYEN) == 0) &&
         (bufferCount != 0)) {

        interruptContextPtr->RxState = IMX_UART_STATE::COMPLETE;
        IMX_UART_LOG_TRACE(
            "Nonzero bytes in intermediate buffer, calling SerCx2PioReceiveReady(). (bufferCount = %lu)",
            bufferCount);

        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
        SerCx2PioReceiveReady(PioReceive);
        return;
    }

    //
    // Enable RRDY interrupt
    //
    if ((interruptContextPtr->Ucr1Copy & IMX_UART_UCR1_RRDYEN) == 0) {
        interruptContextPtr->Ucr1Copy |= IMX_UART_UCR1_RRDYEN;
        interruptContextPtr->Usr1EnabledInterruptsMask |= IMX_UART_USR1_RRDY;

        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Ucr1,
            interruptContextPtr->Ucr1Copy);
    }

    //
    // Enable aging character interrupt
    //
    if ((interruptContextPtr->Ucr2Copy & IMX_UART_UCR2_ATEN) == 0) {
        interruptContextPtr->Ucr2Copy |= IMX_UART_UCR2_ATEN;
        interruptContextPtr->Usr1EnabledInterruptsMask |= IMX_UART_USR1_AGTIM;

        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Ucr2,
            interruptContextPtr->Ucr2Copy);
    }

    interruptContextPtr->RxState = IMX_UART_STATE::WAITING_FOR_INTERRUPT;

    IMX_UART_LOG_TRACE("Enabled RRDY and AGTIM, going to WAITING_FOR_INTERRUPT");
    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
}

_Use_decl_annotations_
BOOLEAN
IMXUartEvtSerCx2PioReceiveCancelReadyNotification (
    SERCX2PIORECEIVE PioReceive
    )
{
    IMX_UART_RX_CONTEXT* rxContextPtr = IMXUartGetRxContext(PioReceive);
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            rxContextPtr->InterruptContextPtr;

    BOOLEAN cancelled;

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);

    const IMX_UART_STATE rxState = interruptContextPtr->RxState;

    switch (rxState) {
    case IMX_UART_STATE::STOPPED:
    case IMX_UART_STATE::CANCELLED:
        NT_ASSERT(!"Unexpected state");
        __fallthrough;

    case IMX_UART_STATE::IDLE:
    case IMX_UART_STATE::WAITING_FOR_INTERRUPT:
        interruptContextPtr->RxState = IMX_UART_STATE::CANCELLED;
        cancelled = TRUE;
        break;

    case IMX_UART_STATE::WAITING_FOR_DPC:
    case IMX_UART_STATE::COMPLETE:
    default:
        cancelled = FALSE;
        break;
    }

    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    IMX_UART_LOG_TRACE(
        "Processed receive cancel request. (rxState = %d, cancelled = %d)",
        int(rxState),
        cancelled);

    return cancelled;
}

_Use_decl_annotations_
ULONG
IMXUartEvtSerCx2PioTransmitWriteBuffer (
    SERCX2PIOTRANSMIT PioTransmit,
    PUCHAR Buffer,
    ULONG Length
    )
{
    IMX_UART_TX_CONTEXT* txContextPtr = IMXUartGetTxContext(PioTransmit);
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            txContextPtr->InterruptContextPtr;

    IMX_UART_REGISTERS* registersPtr = interruptContextPtr->RegistersPtr;

    interruptContextPtr->TxState = IMX_UART_STATE::IDLE;
    interruptContextPtr->TxDrainState = IMX_UART_STATE::IDLE;

    ULONG fifoBytesWritten = 0;
    if (interruptContextPtr->TxBuffer.IsEmpty()) {

        //
        // Write directly to TX FIFO
        //
        while (fifoBytesWritten < Length) {
            const ULONG uts = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Uts);
            if ((uts & IMX_UART_UTS_TXFULL) != 0) {
                break;
            }

            WRITE_REGISTER_NOFENCE_ULONG(
                &registersPtr->Txd,
                Buffer[fifoBytesWritten]);

            ++fifoBytesWritten;
        }
    }

    //
    // Queue remaining bytes to the intermediate buffer
    //
    ULONG bytesEnqueued = interruptContextPtr->TxBuffer.EnqueueBytes(
            Buffer + fifoBytesWritten,
            Length - fifoBytesWritten);

    //
    // If we put bytes in the intermediate TX buffer, enable TX interrupts
    // so the ISR will drain the intermediate buffer
    //
    if (bytesEnqueued != 0) {
        WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
        interruptContextPtr->Ucr1Copy |= IMX_UART_UCR1_TRDYEN;
        interruptContextPtr->Usr1EnabledInterruptsMask |= IMX_UART_USR1_TRDY;
        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Ucr1,
            interruptContextPtr->Ucr1Copy);

        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
    }

    IMX_UART_LOG_TRACE(
        "Serviced write buffer request. (Length = %lu, fifoBytesWritten = %lu, bytesEnqueued = %lu)",
        Length,
        fifoBytesWritten,
        bytesEnqueued);

    return fifoBytesWritten + bytesEnqueued;
}

_Use_decl_annotations_
VOID
IMXUartEvtSerCx2PioTransmitEnableReadyNotification (
    SERCX2PIOTRANSMIT PioTransmit
    )
{
    IMX_UART_TX_CONTEXT* txContextPtr = IMXUartGetTxContext(PioTransmit);
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            txContextPtr->InterruptContextPtr;

    IMX_UART_REGISTERS* registersPtr = interruptContextPtr->RegistersPtr;

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    if (interruptContextPtr->TxState == IMX_UART_STATE::CANCELLED) {
        IMX_UART_LOG_TRACE("Transmit ready notification was already cancelled");
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
        return;
    }

    //
    // Enable TRDY
    //
    interruptContextPtr->Ucr1Copy |= IMX_UART_UCR1_TRDYEN;
    interruptContextPtr->Usr1EnabledInterruptsMask |= IMX_UART_USR1_TRDY;
    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr1,
        interruptContextPtr->Ucr1Copy);

    interruptContextPtr->TxState = IMX_UART_STATE::WAITING_FOR_INTERRUPT;

    IMX_UART_LOG_TRACE("Enabled TRDY interrupt - going to WAITING_FOR_INTERRUPT");
    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
}

_Use_decl_annotations_
BOOLEAN
IMXUartEvtSerCx2PioTransmitCancelReadyNotification (
    SERCX2PIOTRANSMIT PioTransmit
    )
{
    IMX_UART_TX_CONTEXT* txContextPtr = IMXUartGetTxContext(PioTransmit);
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            txContextPtr->InterruptContextPtr;

    BOOLEAN cancelled;

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);

    const IMX_UART_STATE txState = interruptContextPtr->TxState;

    switch (txState) {
    case IMX_UART_STATE::STOPPED:
    case IMX_UART_STATE::CANCELLED:
        NT_ASSERT(!"Unexpected state");
        __fallthrough;

    case IMX_UART_STATE::IDLE:
    case IMX_UART_STATE::WAITING_FOR_INTERRUPT:
        interruptContextPtr->TxState = IMX_UART_STATE::CANCELLED;
        cancelled = TRUE;
        break;

    case IMX_UART_STATE::WAITING_FOR_DPC:
    case IMX_UART_STATE::COMPLETE:
    default:
        cancelled = FALSE;
        break;
    }

    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    IMX_UART_LOG_TRACE(
        "Processed transmit cancel request. (txState = %d, cancelled = %d)",
        int(txState),
        cancelled);

    return cancelled;
}

_Use_decl_annotations_
VOID
IMXUartEvtSerCx2PioTransmitDrainFifo (
    SERCX2PIOTRANSMIT PioTransmit
    )
{
    IMX_UART_TX_CONTEXT* txContextPtr = IMXUartGetTxContext(PioTransmit);
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            txContextPtr->InterruptContextPtr;

    IMX_UART_REGISTERS* registersPtr = interruptContextPtr->RegistersPtr;

    //
    // Clear drain complete flag and enable TX FIFO empty interrupt
    //
    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    if (interruptContextPtr->TxDrainState == IMX_UART_STATE::CANCELLED) {
        IMX_UART_LOG_TRACE("Drain FIFO request was already cancelled");
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
        return;
    }

    //
    // Can we complete the drain FIFO request synchronously?
    //
    ULONG usr2 = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Usr2);
    if ((usr2 & IMX_UART_USR2_TXFE) != 0) {
        IMX_UART_LOG_TRACE(
            "TX FIFO is empty, completing DrainFifo request inline. (usr2 = 0x%lx)",
            usr2);

        interruptContextPtr->TxDrainState = IMX_UART_STATE::COMPLETE;
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
        SerCx2PioTransmitDrainFifoComplete(PioTransmit);
        return;
    }

    //
    // Enable the TX Fifo Empty interrupt
    //
    interruptContextPtr->Ucr1Copy |= IMX_UART_UCR1_TXMPTYEN;
    interruptContextPtr->Usr2EnabledInterruptsMask |= IMX_UART_USR2_TXFE;
    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr1,
        interruptContextPtr->Ucr1Copy);

    IMX_UART_LOG_TRACE("Enabled TX FIFO empty interrupt to notify of drain completion");
    interruptContextPtr->TxDrainState = IMX_UART_STATE::WAITING_FOR_INTERRUPT;
    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
}

_Use_decl_annotations_
BOOLEAN
IMXUartEvtSerCx2PioTransmitCancelDrainFifo (
    SERCX2PIOTRANSMIT PioTransmit
    )
{
    IMX_UART_TX_CONTEXT* txContextPtr = IMXUartGetTxContext(PioTransmit);
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            txContextPtr->InterruptContextPtr;

    BOOLEAN cancelled;

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);

    const IMX_UART_STATE drainState = interruptContextPtr->TxDrainState;

    switch (drainState) {
    case IMX_UART_STATE::STOPPED:
    case IMX_UART_STATE::CANCELLED:
        NT_ASSERT(!"Unexpected state");
        __fallthrough;

    case IMX_UART_STATE::IDLE:
    case IMX_UART_STATE::WAITING_FOR_INTERRUPT:

        //
        // Let the ISR disable the TXFE interrupt, since we don't know who
        // else might be relying on the TXFE interrupt.
        //
        interruptContextPtr->TxDrainState = IMX_UART_STATE::CANCELLED;
        cancelled = TRUE;
        break;

    case IMX_UART_STATE::WAITING_FOR_DPC:
    case IMX_UART_STATE::COMPLETE:
    default:
        cancelled = FALSE;
        break;
    }

    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    IMX_UART_LOG_TRACE(
        "Processed drain cancellation request. (drainState = %d, cancelled = %d)",
        int(drainState),
        cancelled);

    return cancelled;
}

_Use_decl_annotations_
VOID
IMXUartEvtSerCx2PioTransmitPurgeFifo (
    SERCX2PIOTRANSMIT PioTransmit,
    ULONG BytesAlreadyTransmittedToHardware
    )
{
    IMX_UART_TX_CONTEXT* txContextPtr = IMXUartGetTxContext(PioTransmit);
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            txContextPtr->InterruptContextPtr;

    IMX_UART_REGISTERS* registersPtr = interruptContextPtr->RegistersPtr;

    NT_ASSERT(interruptContextPtr->TxPurgeState == IMX_UART_STATE::STOPPED);

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);

    //
    // Empty the TX intermediate buffer
    //
    ULONG discardedByteCount = interruptContextPtr->TxBuffer.Reset();
    NT_ASSERT(discardedByteCount <= BytesAlreadyTransmittedToHardware);
    UNREFERENCED_PARAMETER(BytesAlreadyTransmittedToHardware);

    //
    // Disable the TRDY interrupt
    //
    interruptContextPtr->Ucr1Copy &= ~IMX_UART_UCR1_TRDYEN;
    interruptContextPtr->Usr1EnabledInterruptsMask &= ~IMX_UART_USR1_TRDY;

    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr1,
        interruptContextPtr->Ucr1Copy);

    //
    // If the TX hardware FIFO is already empty, complete the request inline.
    //
    ULONG usr2 = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Usr2);
    if ((usr2 & IMX_UART_USR2_TXFE) != 0) {
        IMX_UART_LOG_TRACE(
            "TX FIFO is empty, completing PurgeFifo request inline. (BytesAlreadyTransmittedToHardware = %lu, discardedByteCount = %lu)",
            BytesAlreadyTransmittedToHardware,
            discardedByteCount);

        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

        SerCx2PioTransmitPurgeFifoComplete(
            PioTransmit,
            discardedByteCount);

        return;
    }

    //
    // If CTS flow control is enabled, the FIFO might not ever empty.
    // There is no way to empty just the transmit FIFO without also affecting
    // the receive FIFO.
    //
    if ((interruptContextPtr->Ucr2Copy & IMX_UART_UCR2_IRTS) == 0) {
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

        const ULONG usr2Tmp = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Usr2);
        if ((usr2Tmp & IMX_UART_USR2_TXFE) == 0) {
            IMX_UART_LOG_WARNING(
                "Unable to purge all bytes in TX FIFO. These bytes will be "
                "sent by the transmitter when CTS is asserted. (usr2 = 0x%x)",
                usr2Tmp);
        }

        SerCx2PioTransmitPurgeFifoComplete(
            PioTransmit,
            discardedByteCount);

        return;
    }

    //
    // Enable the TXFE interrupt and go to WAITING_FOR_INTERRUPT state
    //
    interruptContextPtr->Ucr1Copy |= IMX_UART_UCR1_TXMPTYEN;
    interruptContextPtr->Usr2EnabledInterruptsMask |= IMX_UART_USR2_TXFE;
    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr1,
        interruptContextPtr->Ucr1Copy);

    interruptContextPtr->PurgedByteCount = discardedByteCount;
    interruptContextPtr->TxPurgeState = IMX_UART_STATE::WAITING_FOR_INTERRUPT;

    IMX_UART_LOG_TRACE(
        "Discarded bytes from intermediate buffer, going to WAITING_FOR_INTERRUPT state. (BytesAlreadyTransmittedToHardware = %lu, discardedByteCount = %lu)",
        BytesAlreadyTransmittedToHardware,
        discardedByteCount);

    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
}

_Use_decl_annotations_
VOID
IMXUartEvtSerCx2CustomReceiveTransactionStart (
    SERCX2CUSTOMRECEIVETRANSACTION CustomReceiveTransaction,
    WDFREQUEST WdfRequest,
    PMDL MdlPtr,
    ULONG Offset,
    ULONG Length
    )
{
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        IMXUartGetRxDmaTransactionContext(CustomReceiveTransaction);

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        rxDmaTransactionContextPtr->InterruptContextPtr;

    IMX_UART_LOG_TRACE(
        "Starting custom receive transaction, length %lu", Length);

    //
    // Start the RX DMA if not already started
    //
    NTSTATUS status = IMXUartStartRxDma(
        rxDmaTransactionContextPtr->InterruptContextPtr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "IMXUartStartRxDma failed. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    rxDmaTransactionContextPtr->WdfRequest = WdfRequest;
    rxDmaTransactionContextPtr->BufferMdlPtr = MdlPtr;
    rxDmaTransactionContextPtr->BufferMdlOffset = Offset;
    rxDmaTransactionContextPtr->TransferLength = Length;
    rxDmaTransactionContextPtr->BytesTransferred = 0;
    rxDmaTransactionContextPtr->UnreportedBytes = 0;

    WdfSpinLockAcquire(rxDmaTransactionContextPtr->Lock);
    status = WdfRequestMarkCancelableEx(
        WdfRequest,
        IMXUartEvtCustomReceiveTransactionRequestCancel);

    if (NT_SUCCESS(status)) {
        //
        // Enable RX DMA and Aging DMA timer
        //
        // Even though we do not handle DMA receive transactions through
        // ISR/DPC, but rather through the DMA completion routine, and
        // DMA progress timer, we set the RxDma state to WAITING_FOR_DPC,
        // to mark it as active.
        //
        IMX_UART_REGISTERS* registersPtr = interruptContextPtr->RegistersPtr;

        WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
        interruptContextPtr->Ucr1Copy |=
            (IMX_UART_UCR1_RXDMAEN | IMX_UART_UCR1_ATDMAEN);

        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Ucr1,
            interruptContextPtr->Ucr1Copy);

        interruptContextPtr->RxDmaState = IMX_UART_STATE::WAITING_FOR_DPC;
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
        WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);

        //
        // Check if there are already pending bytes
        //

        bool isRequestCompleted = IMXUartRxDmaCopyToUserBuffer(
            rxDmaTransactionContextPtr,
            0);

        if (isRequestCompleted) {
            IMXUartCompleteCustomRxTransactionRequest(
                rxDmaTransactionContextPtr,
                STATUS_SUCCESS);

            return;
        }

        //
        // If all is good, start the DMA progress monitor timer
        // based on the transaction length.
        //
        IMXUartRxDmaStartProgressTimer(rxDmaTransactionContextPtr);
        return;
    } else {
        rxDmaTransactionContextPtr->WdfRequest = WDF_NO_HANDLE;
        WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);
        WdfRequestComplete(WdfRequest, status);
        return;
    }
}

_Use_decl_annotations_
VOID
IMXUartEvtSerCx2CustomReceiveTransactionCleanup (
    SERCX2CUSTOMRECEIVETRANSACTION CustomReceiveTransaction
    )
{
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        IMXUartGetRxDmaTransactionContext(CustomReceiveTransaction);

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        rxDmaTransactionContextPtr->InterruptContextPtr;

    IMX_UART_LOG_TRACE(
        "Cleaning up custom receive transaction, length %Iu, %Iu bytes transferred",
        rxDmaTransactionContextPtr->TransferLength,
        rxDmaTransactionContextPtr->BytesTransferred);

    //
    // We do not pause the DMA transfer to avoid loosing
    // data. New received data will be picked up by the
    // next custom/PIO transaction.
    // We set the RX DMA state to STOPPING, so it will not block
    // UART setup requests.
    //

    WdfSpinLockAcquire(rxDmaTransactionContextPtr->Lock);

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    if (interruptContextPtr->IsRxDmaStarted) {
        interruptContextPtr->RxDmaState = IMX_UART_STATE::STOPPING;
    }
    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    WdfTimerStop(rxDmaTransactionContextPtr->WdfProgressTimer, FALSE);
    rxDmaTransactionContextPtr->BufferMdlPtr = nullptr;

    WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);

    SerCx2CustomReceiveTransactionCleanupComplete(CustomReceiveTransaction);
}

_Use_decl_annotations_
VOID
IMXUartEvtSerCx2CustomReceiveTransactionQueryProgress (
    SERCX2CUSTOMRECEIVETRANSACTION CustomReceiveTransaction
    )
{
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        IMXUartGetRxDmaTransactionContext(CustomReceiveTransaction);

    SERCX2_CUSTOM_RECEIVE_TRANSACTION_PROGRESS transactionProgress;
    LONG unreportedBytes = InterlockedExchange(
        &rxDmaTransactionContextPtr->UnreportedBytes, 0);

    if (unreportedBytes != 0) {
        transactionProgress = SerCx2CustomReceiveTransactionBytesTransferred;
    } else {
        transactionProgress = SerCx2CustomReceiveTransactionNoProgress;
    }

    SerCx2CustomReceiveTransactionReportProgress(
        CustomReceiveTransaction,
        transactionProgress);
}

_Use_decl_annotations_
VOID
IMXUartEvtCustomReceiveTransactionRequestCancel (
    WDFREQUEST WdfRequest
    )
{
    IMX_UART_DEVICE_CONTEXT* deviceContextPtr = IMXUartGetDeviceContext(
        WdfIoQueueGetDevice(WdfRequestGetIoQueue(WdfRequest)));

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        deviceContextPtr->InterruptContextPtr;

    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        interruptContextPtr->RxDmaTransactionContextPtr;

    //
    // We do not pause the DMA transfer to avoid loosing
    // data. New received data which will be picked up by the
    // next custom/PIO transaction.
    // We set the RX DMA state to STOPPING, so it will not block
    // UART setup requests.
    //
    // We need to acquire the DMA transaction lock in order to
    // sync access to the caller buffer.
    //

    WdfSpinLockAcquire(rxDmaTransactionContextPtr->Lock);

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    if (interruptContextPtr->IsRxDmaStarted) {
        interruptContextPtr->RxDmaState = IMX_UART_STATE::STOPPING;
    }
    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    WdfTimerStop(rxDmaTransactionContextPtr->WdfProgressTimer, FALSE);

    IMX_UART_LOG_TRACE(
        "Canceling custom receive transaction, length %Iu, %Iu bytes transferred",
        rxDmaTransactionContextPtr->TransferLength,
        rxDmaTransactionContextPtr->BytesTransferred);

    WDFREQUEST wdfRequest = rxDmaTransactionContextPtr->WdfRequest;
    rxDmaTransactionContextPtr->WdfRequest = WDF_NO_HANDLE;
    rxDmaTransactionContextPtr->BufferMdlPtr = nullptr;
    WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);
    if (wdfRequest != WDF_NO_HANDLE) {
        WdfRequestCompleteWithInformation(
            wdfRequest,
            STATUS_SUCCESS,
            rxDmaTransactionContextPtr->BytesTransferred);
    }
}

_Use_decl_annotations_
BOOLEAN
IMXUartEvtWdfRxDmaTransactionConfigureDmaChannel (
    WDFDMATRANSACTION /* WdfDmaTransaction */,
    WDFDEVICE /* Device */,
    PVOID ContextPtr,
    PMDL /* MdlPtr */,
    size_t /* Offset */,
    size_t /* Length */
    )
{
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        static_cast<IMX_UART_RX_DMA_TRANSACTION_CONTEXT*>(ContextPtr);

    const IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        rxDmaTransactionContextPtr->InterruptContextPtr;

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    ULONG watermarkLevel =
        (interruptContextPtr->UfcrCopy & IMX_UART_UFCR_RXTL_MASK) >>
        IMX_UART_UFCR_RXTL_SHIFT;

    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    //
    // Set the RX DMA watermark level
    //
    DMA_ADAPTER* dmaAdapterPtr = rxDmaTransactionContextPtr->DmaAdapterPtr;
    NTSTATUS status = dmaAdapterPtr->DmaOperations->ConfigureAdapterChannel(
        dmaAdapterPtr,
        SDMA_CFG_FUN_SET_CHANNEL_WATERMARK_LEVEL,
        &watermarkLevel);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "SDMA_CFG_FUN_SET_CHANNEL_WATERMARK_LEVEL failed "
            "for RX DMA transaction. (status = %!STATUS!)",
            status);

        return FALSE;
    }

    //
    // Set notification threshold
    //
    ULONG notificationBytes = (ULONG)(rxDmaTransactionContextPtr->DmaBufferSize) / 4;
    status = dmaAdapterPtr->DmaOperations->ConfigureAdapterChannel(
        dmaAdapterPtr,
        SDMA_CFG_FUN_SET_CHANNEL_NOTIFICATION_THRESHOLD,
        &notificationBytes);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "SDMA_CFG_FUN_SET_CHANNEL_NOTIFICATION_THRESHOLD failed "
            "for RX DMA transaction. (status = %!STATUS!)",
            status);

        return FALSE;
    }

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN
IMXUartEvtWdfProgramRxDma (
    WDFDMATRANSACTION /* WdfDmaTransaction */,
    WDFDEVICE /* Device */,
    WDFCONTEXT ContextPtr,
    WDF_DMA_DIRECTION /* Direction */,
    PSCATTER_GATHER_LIST /* SgListPtr */
    )
{
    //
    // Since we are using System DMA, WDF programs the SG list into the
    // System DMA controller for us.
    // We use the EvtWdfProgramDma callback as a transaction setup.
    //

    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        static_cast<IMX_UART_RX_DMA_TRANSACTION_CONTEXT*>(ContextPtr);

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        rxDmaTransactionContextPtr->InterruptContextPtr;

    IMX_UART_REGISTERS* registersPtr = interruptContextPtr->RegistersPtr;

    //
    // Keep the DMA transfer paused until enabled
    // through the custom receive transaction.
    //
    // Disable RX DMA and Aging DMA timer
    //

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    interruptContextPtr->RxDmaState = IMX_UART_STATE::IDLE;
    interruptContextPtr->Ucr1Copy &=
        ~(IMX_UART_UCR1_RXDMAEN | IMX_UART_UCR1_ATDMAEN);

    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr1,
        interruptContextPtr->Ucr1Copy);

    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    NT_ASSERT(rxDmaTransactionContextPtr->DmaBufferPtr != nullptr);
    NT_ASSERT(rxDmaTransactionContextPtr->DmaBufferSize != 0);
    NT_ASSERT(rxDmaTransactionContextPtr->DmaBufferMdlPtr != nullptr);

    rxDmaTransactionContextPtr->DmaBufferReadPos = 0;
    rxDmaTransactionContextPtr->DmaBufferPendingBytes = 0;
    rxDmaTransactionContextPtr->BytesTransferred = 0;
    rxDmaTransactionContextPtr->LastDmaCounter = 0;
    return TRUE;
}

_Use_decl_annotations_
VOID
IMXUartEvtTimerRxDmaProgress (
    WDFTIMER WdfTimer
    )
{
    IMX_UART_RX_DMA_TIMER_CONTEXT* rxDmaTimerContextPtr =
        IMXUartGetRxDmaTimerContext(WdfTimer);

    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        rxDmaTimerContextPtr->RxDmaTransactionPtr;

    size_t bytesTransferred =
        IMXUartRxDmaGetBytesTransferred(rxDmaTransactionContextPtr);

    bool isRequestCompleted = IMXUartRxDmaCopyToUserBuffer(
        rxDmaTransactionContextPtr,
        bytesTransferred);

    if (bytesTransferred != 0) {
        IMX_UART_LOG_TRACE(
            "RX DMA timer: Got %Iu bytes. %Iu out of %Iu transferred",
            bytesTransferred,
            rxDmaTransactionContextPtr->BytesTransferred,
            rxDmaTransactionContextPtr->TransferLength);
    }

    if (isRequestCompleted) {
        IMXUartCompleteCustomRxTransactionRequest(
            rxDmaTransactionContextPtr,
            STATUS_SUCCESS);

        return;
    }

    IMXUartRxDmaStartProgressTimer(rxDmaTransactionContextPtr);
}

_Use_decl_annotations_
VOID
IMXUartEvtWdfRxDmaTransactionTransferComplete (
    WDFDMATRANSACTION /* WdfTransaction */,
    WDFDEVICE /* WdfDevice */,
    WDFCONTEXT ContextPtr,
    WDF_DMA_DIRECTION /* Direction */,
    DMA_COMPLETION_STATUS DmaStatus
    )
{
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        static_cast<IMX_UART_RX_DMA_TRANSACTION_CONTEXT*>(ContextPtr);

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        rxDmaTransactionContextPtr->InterruptContextPtr;

    IMX_UART_LOG_TRACE(
        "RX DMA: Transaction completed. (status = %!DMACOMPLETIONSTATUS!)",
        DmaStatus);

    WdfTimerStop(rxDmaTransactionContextPtr->WdfProgressTimer, FALSE);

    ULONG waitEvents = 0;

    if (DmaStatus == DmaComplete) {
        size_t bytesTransferred =
            IMXUartRxDmaGetBytesTransferred(rxDmaTransactionContextPtr);

        if (bytesTransferred != 0) {
            waitEvents |= SERIAL_EV_RXCHAR;
        }

        bool isRequestCompleted = IMXUartRxDmaCopyToUserBuffer(
            rxDmaTransactionContextPtr,
            bytesTransferred);

        IMX_UART_LOG_TRACE(
            "RX DMA: Got %Iu bytes. %Iu out of %Iu transferred ",
            bytesTransferred,
            rxDmaTransactionContextPtr->BytesTransferred,
            rxDmaTransactionContextPtr->TransferLength);

        if (isRequestCompleted) {
            IMXUartCompleteCustomRxTransactionRequest(
                rxDmaTransactionContextPtr,
                STATUS_SUCCESS);

        } else {
            IMXUartRxDmaStartProgressTimer(rxDmaTransactionContextPtr);
        }
    } else {
        NTSTATUS requestStatus;
        switch (DmaStatus) {
        case DmaAborted:
            requestStatus = STATUS_REQUEST_ABORTED;
            break;

        case DmaError:
            waitEvents |= SERIAL_EV_ERR;
            requestStatus = IMXUartGetErrorInformation(interruptContextPtr);
            break;

        case DmaCancelled:
            __fallthrough;

        default:
            requestStatus = STATUS_REQUEST_CANCELED;
            break;
        }

        if (rxDmaTransactionContextPtr->BytesTransferred != 0) {
            requestStatus = STATUS_SUCCESS;
        }

        IMXUartCompleteCustomRxTransactionRequest(
            rxDmaTransactionContextPtr,
            requestStatus);
    }

    //
    // Notify events if any...
    //
    if (waitEvents != 0) {
        IMXUartNotifyEventsDuringDma(
            rxDmaTransactionContextPtr,
            waitEvents,
            0);
    }
}

_Use_decl_annotations_
VOID
IMXUartRxDmaStartProgressTimer (
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* RxDmaTransactionContextPtr
    )
{
    //
    // Start the timer, based on the remaining bytes
    //

    size_t bytesLeftToTransfer = RxDmaTransactionContextPtr->TransferLength -
        RxDmaTransactionContextPtr->BytesTransferred;

    ULONG progressTimerUsec = (ULONG)bytesLeftToTransfer *
        RxDmaTransactionContextPtr->CurrentFrameTimeUsec;

    WdfTimerStart(
        RxDmaTransactionContextPtr->WdfProgressTimer,
        WDF_REL_TIMEOUT_IN_US(progressTimerUsec));
}

_Use_decl_annotations_
ULONG
IMXUartRxDmaGetBytesTransferred (
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* RxDmaTransactionContextPtr
    )
{
    WdfSpinLockAcquire(RxDmaTransactionContextPtr->Lock);

    size_t dmaCounter = RxDmaTransactionContextPtr->DmaBufferSize -
        IMXUartDmaReadCounter(RxDmaTransactionContextPtr->DmaAdapterPtr);

    size_t newBytes;
    if (dmaCounter >= RxDmaTransactionContextPtr->LastDmaCounter) {
        newBytes = dmaCounter - RxDmaTransactionContextPtr->LastDmaCounter;
    } else {
        newBytes = RxDmaTransactionContextPtr->DmaBufferSize -
            RxDmaTransactionContextPtr->LastDmaCounter +
            dmaCounter;
    }

    RxDmaTransactionContextPtr->LastDmaCounter = dmaCounter;
    WdfSpinLockRelease(RxDmaTransactionContextPtr->Lock);
    InterlockedAdd(
        &RxDmaTransactionContextPtr->UnreportedBytes,
        LONG(newBytes));

    return (ULONG)newBytes;
}

_Use_decl_annotations_
bool
IMXUartRxDmaCopyToUserBuffer (
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* RxDmaTransactionContextPtr,
    size_t NewBytesTransferred
    )
{
    //
    // Sync access to the caller buffer to make sure
    // request/buffer do not go aways if the request is canceled.
    //
    WdfSpinLockAcquire(RxDmaTransactionContextPtr->Lock);

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        RxDmaTransactionContextPtr->InterruptContextPtr;

    //
    // A DPC for RX completion/progress timer may have been queued
    // just before RX DMA was stopped. Ignore it.
    //
    if (!interruptContextPtr->IsRxDmaStarted) {
        NT_ASSERT(interruptContextPtr->RxDmaState == IMX_UART_STATE::STOPPED);
        IMX_UART_LOG_WARNING("RX DMA: DMA active after DMA was stopped!");
        WdfSpinLockRelease(RxDmaTransactionContextPtr->Lock);
        return true;
    }

    RxDmaTransactionContextPtr->DmaBufferPendingBytes += NewBytesTransferred;

    //
    // Update comm events, if any...
    //
    ULONG waitEvents = 0;
    ULONG commStatusInfo = 0;
    if (NewBytesTransferred != 0) {
        waitEvents = SERIAL_EV_RXCHAR;
    }

    //
    // Check for DMA buffer (SW) overrun
    //
    if (RxDmaTransactionContextPtr->DmaBufferPendingBytes >
        RxDmaTransactionContextPtr->DmaBufferSize) {

        waitEvents |= SERIAL_EV_ERR;
        commStatusInfo = SERIAL_ERROR_QUEUEOVERRUN;
    }

    if (waitEvents != 0) {
        IMXUartNotifyEventsDuringDma(
            RxDmaTransactionContextPtr,
            waitEvents,
            commStatusInfo);
    }

    if ((RxDmaTransactionContextPtr->BufferMdlPtr == nullptr) ||
        (RxDmaTransactionContextPtr->DmaBufferPendingBytes == 0)) {

        WdfSpinLockRelease(RxDmaTransactionContextPtr->Lock);
        return false;
    }

    //
    // DMA buffer runtime parameters
    //
    size_t dmaBufferPendingBytes = RxDmaTransactionContextPtr->DmaBufferPendingBytes;
    size_t dmaBufferReadPos = RxDmaTransactionContextPtr->DmaBufferReadPos;
    size_t dmaBufferSize = RxDmaTransactionContextPtr->DmaBufferSize;
    const UCHAR* dmaBufferPtr = RxDmaTransactionContextPtr->DmaBufferPtr;
    //
    // User buffer runtime parameters
    //
    PMDL mdlPtr = RxDmaTransactionContextPtr->BufferMdlPtr;
    size_t mdlOffset = RxDmaTransactionContextPtr->BufferMdlOffset;
    UCHAR* mdlAddr =
        reinterpret_cast<UCHAR*>(mdlPtr->MappedSystemVa) + mdlOffset;

    size_t maxBytesToCopy = RxDmaTransactionContextPtr->TransferLength -
        RxDmaTransactionContextPtr->BytesTransferred;

    size_t bytesToCopy = min(dmaBufferPendingBytes, maxBytesToCopy);
    size_t bytesCopied = 0;

    while (bytesToCopy != 0) {
        size_t mdlBytes = MmGetMdlByteCount(mdlPtr) - mdlOffset;
        if (mdlBytes == 0) {
            mdlPtr = mdlPtr->Next;
            if (mdlPtr == nullptr) {
                break;
            }
            mdlAddr = reinterpret_cast<UCHAR*>(mdlPtr->MappedSystemVa);
            mdlOffset = 0;
        }

        size_t dmaBufferBytes = dmaBufferSize - dmaBufferReadPos;
        dmaBufferBytes = min(dmaBufferBytes, bytesToCopy);

        size_t iterBytes = min(mdlBytes, dmaBufferBytes);

        RtlCopyMemory(
            mdlAddr,
            dmaBufferPtr + dmaBufferReadPos,
            iterBytes);

        dmaBufferReadPos = (dmaBufferReadPos + iterBytes) % dmaBufferSize;
        mdlOffset += iterBytes;
        bytesToCopy -= iterBytes;
        bytesCopied += iterBytes;
    }

    RxDmaTransactionContextPtr->DmaBufferReadPos = dmaBufferReadPos;
    RxDmaTransactionContextPtr->BufferMdlPtr = mdlPtr;
    RxDmaTransactionContextPtr->BufferMdlOffset = mdlOffset;
    RxDmaTransactionContextPtr->BytesTransferred += bytesCopied;
    RxDmaTransactionContextPtr->DmaBufferPendingBytes -= bytesCopied;

    bool isReqCompleted = RxDmaTransactionContextPtr->BytesTransferred ==
        RxDmaTransactionContextPtr->TransferLength;

    WdfSpinLockRelease(RxDmaTransactionContextPtr->Lock);
    return isReqCompleted;
}

_Use_decl_annotations_
ULONG
IMXUartPioDequeueDmaBytes (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr,
    PUCHAR Buffer,
    ULONG Length
    )
{
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        InterruptContextPtr->RxDmaTransactionContextPtr;

    //
    // If DMA is not used, nothing to do here...
    //
    if (rxDmaTransactionContextPtr == nullptr) {
        return 0;
    }

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        rxDmaTransactionContextPtr->InterruptContextPtr;

    //
    // If DMA is not active, nothing more to do here...
    // No need to sync access to RxDmaState since if it is
    // active it is modified later on in this routine, and if it is not,
    // it will be changed by the next custom receive transaction which
    // is synchronized with this routine by SerCx2.
    //
    if (!IMXUartIsRxDmaActive(interruptContextPtr)) {
        return 0;
    }

    size_t newBytesTransferred =
        IMXUartRxDmaGetBytesTransferred(rxDmaTransactionContextPtr);

    WdfSpinLockAcquire(rxDmaTransactionContextPtr->Lock);
    rxDmaTransactionContextPtr->DmaBufferPendingBytes += newBytesTransferred;

    IMX_UART_LOG_TRACE(
        "RX DMA: retrieving residual %Iu DMA bytes",
        rxDmaTransactionContextPtr->DmaBufferPendingBytes);

    if (rxDmaTransactionContextPtr->DmaBufferPendingBytes == 0) {
        //
        // Stop RX DMA
        //
        IMX_UART_REGISTERS* registersPtr = interruptContextPtr->RegistersPtr;

        WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
        interruptContextPtr->Ucr1Copy &=
            ~(IMX_UART_UCR1_RXDMAEN | IMX_UART_UCR1_ATDMAEN);

        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Ucr1,
            interruptContextPtr->Ucr1Copy);

        InterruptContextPtr->RxDmaState = IMX_UART_STATE::STOPPED;
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
        WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);
        return 0;
    }

    NT_ASSERT(InterruptContextPtr->RxDmaState == IMX_UART_STATE::STOPPING);

    size_t dmaBufferPendingBytes = rxDmaTransactionContextPtr->DmaBufferPendingBytes;
    size_t dmaBufferReadPos = rxDmaTransactionContextPtr->DmaBufferReadPos;
    size_t dmaBufferSize = rxDmaTransactionContextPtr->DmaBufferSize;
    const UCHAR* dmaBufferPtr = rxDmaTransactionContextPtr->DmaBufferPtr;
    size_t bytesToCopy = min(dmaBufferPendingBytes, Length);
    size_t bytesCopied = 0;

    while (bytesToCopy != 0) {
        size_t iterBytes = dmaBufferSize - dmaBufferReadPos;
        iterBytes = min(iterBytes, bytesToCopy);

        RtlCopyMemory(
            Buffer,
            dmaBufferPtr + dmaBufferReadPos,
            iterBytes);

        dmaBufferReadPos = (dmaBufferReadPos + iterBytes) % dmaBufferSize;
        bytesToCopy -= iterBytes;
        bytesCopied += iterBytes;
        Buffer += iterBytes;
    }

    rxDmaTransactionContextPtr->DmaBufferReadPos = dmaBufferReadPos;
    rxDmaTransactionContextPtr->BytesTransferred += bytesCopied;
    rxDmaTransactionContextPtr->DmaBufferPendingBytes -= bytesCopied;
    WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);
    return (ULONG)bytesCopied;
}


_Use_decl_annotations_
VOID
IMXUartCompleteCustomRxTransactionRequest (
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* RxDmaTransactionContextPtr,
    NTSTATUS RequestStatus
    )
{
    WdfSpinLockAcquire(RxDmaTransactionContextPtr->Lock);

    WDFREQUEST wdfRequest = RxDmaTransactionContextPtr->WdfRequest;
    if (wdfRequest == WDF_NO_HANDLE) {
        WdfSpinLockRelease(RxDmaTransactionContextPtr->Lock);
        return;
    }
    RxDmaTransactionContextPtr->WdfRequest = WDF_NO_HANDLE;

    ULONG_PTR requestInfo = RxDmaTransactionContextPtr->BytesTransferred;
    NTSTATUS status = WdfRequestUnmarkCancelable(wdfRequest);
    if (NT_SUCCESS(status) || (status == STATUS_CANCELLED)) {
        WdfSpinLockRelease(RxDmaTransactionContextPtr->Lock);
        WdfRequestCompleteWithInformation(
            wdfRequest,
            RequestStatus,
            requestInfo);

        return;
    }

    WdfSpinLockRelease(RxDmaTransactionContextPtr->Lock);
}

_Use_decl_annotations_
VOID
IMXUartEvtSerCx2CustomTransmitTransactionStart (
    SERCX2CUSTOMTRANSMITTRANSACTION CustomTransmitTransaction,
    WDFREQUEST WdfRequest,
    PMDL MdlPtr,
    ULONG Offset,
    ULONG Length
    )
{
    IMX_UART_LOG_TRACE("Start custom transmit transaction, length %lu", Length);

    IMX_UART_TX_DMA_TRANSACTION_CONTEXT* txDmaTransactionContextPtr =
        IMXUartGetTxDmaTransactionContext(CustomTransmitTransaction);

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        txDmaTransactionContextPtr->InterruptContextPtr;

    WDFDMATRANSACTION txWdfDmaTransaction = txDmaTransactionContextPtr->WdfDmaTransaction;

    NTSTATUS status = WdfDmaTransactionInitializeUsingOffset(
        txWdfDmaTransaction,
        IMXUartEvtWdfProgramTxDma,
        WdfDmaDirectionWriteToDevice,
        MdlPtr,
        Offset,
        Length);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "WdfDmaTransactionInitializeUsingOffset(...) failed. "
            "(status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    txDmaTransactionContextPtr->TransferLength = Length;
    txDmaTransactionContextPtr->BytesTransferred = 0;
    txDmaTransactionContextPtr->IsDeferredCancellation = false;
    interruptContextPtr->TxDmaState = IMX_UART_STATE::IDLE;
    interruptContextPtr->TxDmaDrainState = IMX_UART_STATE::IDLE;

    WdfDmaTransactionSetImmediateExecution(txWdfDmaTransaction, TRUE);
    status = WdfDmaTransactionExecute(
        txWdfDmaTransaction,
        static_cast<WDFCONTEXT>(txDmaTransactionContextPtr));

    if (!NT_SUCCESS(status)) {
        interruptContextPtr->TxDmaState = IMX_UART_STATE::STOPPED;
        WdfDmaTransactionRelease(txWdfDmaTransaction);
        IMX_UART_LOG_ERROR(
            "TX DMA: WdfDmaTransactionExecute Failed! (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    WdfSpinLockAcquire(txDmaTransactionContextPtr->Lock);
    txDmaTransactionContextPtr->WdfRequest = WdfRequest;
    status = WdfRequestMarkCancelableEx(
        WdfRequest,
        IMXUartEvtCustomTransmitTransactionRequestCancel);

    if (!NT_SUCCESS(status)) {
        txDmaTransactionContextPtr->IsDeferredCancellation = true;
        WdfSpinLockRelease(txDmaTransactionContextPtr->Lock);
        //
        // Cancellation continues in transaction completion routine.
        //
        WdfDmaTransactionStopSystemTransfer(
            txDmaTransactionContextPtr->WdfDmaTransaction);

        return;
    }

    //
    // Enable TX DMA
    //
    IMX_UART_REGISTERS* registersPtr = interruptContextPtr->RegistersPtr;

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    interruptContextPtr->Ucr1Copy |= IMX_UART_UCR1_TXDMAEN;
    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr1,
        interruptContextPtr->Ucr1Copy);

    //
    // DMA interrupt is handled by the framework, nevertheless we
    // we use to mark an active TX DMA transfer.
    //
    interruptContextPtr->TxDmaState = IMX_UART_STATE::WAITING_FOR_DPC;
    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
    WdfSpinLockRelease(txDmaTransactionContextPtr->Lock);
}

_Use_decl_annotations_
VOID
IMXUartEvtSerCx2CustomTransmitTransactionCleanup (
    SERCX2CUSTOMTRANSMITTRANSACTION CustomTransmitTransaction
    )
{
    IMX_UART_TX_DMA_TRANSACTION_CONTEXT* txDmaTransactionContextPtr =
        IMXUartGetTxDmaTransactionContext(CustomTransmitTransaction);

    IMX_UART_LOG_TRACE(
        "Cleaning up custom transmit transaction, length %Iu, %Iu bytes transferred",
        txDmaTransactionContextPtr->TransferLength,
        txDmaTransactionContextPtr->BytesTransferred);

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        txDmaTransactionContextPtr->InterruptContextPtr;

    //
    // Disable TX DMA
    //
    IMX_UART_REGISTERS* registersPtr = interruptContextPtr->RegistersPtr;

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    interruptContextPtr->Ucr1Copy &= ~IMX_UART_UCR1_TXDMAEN;
    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr1,
        interruptContextPtr->Ucr1Copy);

    interruptContextPtr->TxDmaState = IMX_UART_STATE::STOPPED;
    interruptContextPtr->TxDmaDrainState = IMX_UART_STATE::STOPPED;
    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    SerCx2CustomTransmitTransactionCleanupComplete(CustomTransmitTransaction);
}

_Use_decl_annotations_
VOID
IMXUartEvtCustomTransmitTransactionRequestCancel (
    WDFREQUEST WdfRequest
    )
{
    IMX_UART_DEVICE_CONTEXT* deviceContextPtr = IMXUartGetDeviceContext(
        WdfIoQueueGetDevice(WdfRequestGetIoQueue(WdfRequest)));

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        deviceContextPtr->InterruptContextPtr;

    IMX_UART_TX_DMA_TRANSACTION_CONTEXT* txDmaTransactionContextPtr =
        interruptContextPtr->TxDmaTransactionContextPtr;

    WdfSpinLockAcquire(txDmaTransactionContextPtr->Lock);

    WDFREQUEST wdfRequest = txDmaTransactionContextPtr->WdfRequest;
    if (wdfRequest == WDF_NO_HANDLE) {
        WdfSpinLockRelease(txDmaTransactionContextPtr->Lock);
        return;
    }

    //
    // Disable TX DMA
    //
    IMX_UART_REGISTERS* registersPtr = interruptContextPtr->RegistersPtr;

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    interruptContextPtr->Ucr1Copy &= ~IMX_UART_UCR1_TXDMAEN;
    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr1,
        interruptContextPtr->Ucr1Copy);

    //
    // Update the drain state if we can at this point.
    // We DO NOT change the TxDmaState since until it is completed,
    // so ISR uses the DMA path (vs PIO path).
    // TxDma cancellation is marked by IsDeferredCancellation.
    //
    switch (interruptContextPtr->TxDmaDrainState) {
    case IMX_UART_STATE::STOPPED:
    case IMX_UART_STATE::CANCELLED:
        NT_ASSERT(!"Unexpected state");
        __fallthrough;

    case IMX_UART_STATE::IDLE:
    case IMX_UART_STATE::WAITING_FOR_INTERRUPT:

        //
        // Let the ISR disable the TXFE interrupt, since we don't know who
        // else might be relying on the TXFE interrupt.
        //
        interruptContextPtr->TxDmaDrainState = IMX_UART_STATE::CANCELLED;
        break;

    case IMX_UART_STATE::WAITING_FOR_DPC:
    case IMX_UART_STATE::COMPLETE:
    default:
        break;
    }
    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    WDFDMATRANSACTION wdfDmaTransaction =
        txDmaTransactionContextPtr->WdfDmaTransaction;

    //
    // Check if request was canceled before map registers were
    // allocated. In this case complete the request locally since
    // DMA callbacks will not be called for this transaction.
    //
    if (WdfDmaTransactionCancel(wdfDmaTransaction)) {
        IMX_UART_LOG_TRACE(
            "Canceling custom transmit inline, length %Iu",
            txDmaTransactionContextPtr->TransferLength);

        txDmaTransactionContextPtr->WdfRequest = WDF_NO_HANDLE;
        WdfDmaTransactionRelease(wdfDmaTransaction);
        WdfSpinLockRelease(txDmaTransactionContextPtr->Lock);
        if (wdfRequest != WDF_NO_HANDLE) {
            WdfRequestComplete(
                wdfRequest,
                STATUS_CANCELLED);
        }
        return;
    }

    //
    // Update the number of bytes transferred.
    // The number of bytes transferred is unreliable, since the
    // DMA controller only updates the host upon completion of a buffer descriptor.
    //
    txDmaTransactionContextPtr->BytesTransferred =
        (txDmaTransactionContextPtr->TransferLength -
         IMXUartDmaReadCounter(txDmaTransactionContextPtr->DmaAdapterPtr));

    IMX_UART_LOG_TRACE(
        "Deferring custom transmit cancellation, length %Iu, %Iu bytes transferred",
        txDmaTransactionContextPtr->TransferLength,
        txDmaTransactionContextPtr->BytesTransferred);

    txDmaTransactionContextPtr->IsDeferredCancellation = true;
    WdfSpinLockRelease(txDmaTransactionContextPtr->Lock);

    //
    // Stop the DMA transaction, processing continues in
    // the transaction completion routine
    // IMXUartEvtWdfTxDmaTransactionTransferComplete.
    //
    WdfDmaTransactionStopSystemTransfer(wdfDmaTransaction);
}

_Use_decl_annotations_
BOOLEAN
IMXUartEvtWdfTxDmaTransactionConfigureDmaChannel (
    WDFDMATRANSACTION /* WdfDmaTransaction */,
    WDFDEVICE /* Device*/,
    PVOID ContextPtr,
    PMDL /* MdlPtr */,
    size_t /* Offset */,
    size_t Length
    )
{
    IMX_UART_TX_DMA_TRANSACTION_CONTEXT* txDmaTransactionContextPtr =
        static_cast<IMX_UART_TX_DMA_TRANSACTION_CONTEXT*>(ContextPtr);

    const IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        txDmaTransactionContextPtr->InterruptContextPtr;

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    ULONG watermarkLevel = IMX_UART_FIFO_COUNT -
        ((interruptContextPtr->UfcrCopy & IMX_UART_UFCR_TXTL_MASK) >>
        IMX_UART_UFCR_TXTL_SHIFT);

    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    IMX_UART_LOG_TRACE(
        "TX DMA transaction configuration: WL %lu, length %Iu",
        watermarkLevel,
        Length);

    //
    // Set the TX DMA watermark level
    //
    DMA_ADAPTER* dmaAdapterPtr = txDmaTransactionContextPtr->DmaAdapterPtr;
    NTSTATUS status = dmaAdapterPtr->DmaOperations->ConfigureAdapterChannel(
        dmaAdapterPtr,
        SDMA_CFG_FUN_SET_CHANNEL_WATERMARK_LEVEL,
        &watermarkLevel);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "SDMA_CFG_FUN_SET_CHANNEL_WATERMARK_LEVEL failed "
            "for TX DMA transaction. (status = %!STATUS!)",
            status);

        return FALSE;
    }

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN
IMXUartEvtWdfProgramTxDma (
    WDFDMATRANSACTION /* WdfDmaTransaction */,
    WDFDEVICE /* Device */,
    WDFCONTEXT /* ContextPtr */,
    WDF_DMA_DIRECTION /* Direction */,
    PSCATTER_GATHER_LIST /* SgListPtr */
    )
{
    //
    // Since we are using System DMA, WDF programs the SG list into the
    // System DMA controller for us.
    //
    return TRUE;
}

_Use_decl_annotations_
VOID
IMXUartEvtWdfTxDmaTransactionTransferComplete (
    WDFDMATRANSACTION /* WdfTransaction */,
    WDFDEVICE /* WdfDevice*/,
    WDFCONTEXT ContextPtr,
    WDF_DMA_DIRECTION /* Direction */,
    DMA_COMPLETION_STATUS DmaStatus
    )
{
    IMX_UART_TX_DMA_TRANSACTION_CONTEXT* txDmaTransactionContextPtr =
        static_cast<IMX_UART_TX_DMA_TRANSACTION_CONTEXT*>(ContextPtr);

    IMX_UART_LOG_TRACE(
        "TX DMA: Transaction completed. (status = %!DMACOMPLETIONSTATUS!)",
        DmaStatus);

    if (DmaStatus == DmaComplete) {
        IMXUartTxDmaDrainFifo(txDmaTransactionContextPtr);
        return;
    } else {
        NTSTATUS requestStatus;
        switch (DmaStatus) {
        case DmaAborted:
            requestStatus = STATUS_REQUEST_ABORTED;
            break;

        case DmaError:
            requestStatus = STATUS_DATA_OVERRUN;
            break;

        case DmaCancelled:
            __fallthrough;

        default:
            requestStatus = STATUS_REQUEST_CANCELED;
            break;
        }

        IMXUartCompleteCustomTxTransactionRequest(
            txDmaTransactionContextPtr,
            requestStatus);
    }
}

_Use_decl_annotations_
VOID
IMXUartTxDmaDrainFifo (
    IMX_UART_TX_DMA_TRANSACTION_CONTEXT* TxDmaTransactionContextPtr
    )
{
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        TxDmaTransactionContextPtr->InterruptContextPtr;

    IMX_UART_REGISTERS* registersPtr = interruptContextPtr->RegistersPtr;

    //
    // Clear drain complete flag and enable TX FIFO empty interrupt
    //
    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    if (interruptContextPtr->TxDmaDrainState == IMX_UART_STATE::CANCELLED) {
        IMX_UART_LOG_TRACE("TX DMA: TX DMA request was already cancelled");
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
        return;
    }

    //
    // Can we complete the drain FIFO request synchronously?
    //
    ULONG usr2 = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Usr2);
    if ((usr2 & IMX_UART_USR2_TXFE) != 0) {
        IMX_UART_LOG_TRACE(
            "TX DMA: TX FIFO is empty, completing DrainFifo request inline. "
            "(usr2 = 0x%lx)",
            usr2);

        interruptContextPtr->TxDmaDrainState = IMX_UART_STATE::COMPLETE;
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
        IMXUartCompleteCustomTxTransactionRequest(
            TxDmaTransactionContextPtr,
            STATUS_SUCCESS);

        return;
    }

    //
    // Enable the TX Fifo Empty interrupt
    //
    interruptContextPtr->Ucr1Copy |= IMX_UART_UCR1_TXMPTYEN;
    interruptContextPtr->Usr2EnabledInterruptsMask |= IMX_UART_USR2_TXFE;
    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr1,
        interruptContextPtr->Ucr1Copy);

    IMX_UART_LOG_TRACE(
        "TX DMA: enabled TX FIFO empty interrupt to notify of drain "
        "completion");

    interruptContextPtr->TxDmaDrainState =
        IMX_UART_STATE::WAITING_FOR_INTERRUPT;

    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
}

_Use_decl_annotations_
VOID
IMXUartCompleteCustomTxTransactionRequest (
    IMX_UART_TX_DMA_TRANSACTION_CONTEXT* TxDmaTransactionContextPtr,
    NTSTATUS RequestStatus
    )
{
    WdfSpinLockAcquire(TxDmaTransactionContextPtr->Lock);

    WDFREQUEST wdfRequest = TxDmaTransactionContextPtr->WdfRequest;
    if (wdfRequest == WDF_NO_HANDLE) {
        WdfSpinLockRelease(TxDmaTransactionContextPtr->Lock);
        return;
    }

    WDFDMATRANSACTION wdfDmaTransaction =
        TxDmaTransactionContextPtr->WdfDmaTransaction;

    BOOLEAN isDone;
    NTSTATUS dmaStatus;
    if (!NT_SUCCESS(RequestStatus) ||
        TxDmaTransactionContextPtr->IsDeferredCancellation) {

        isDone = WdfDmaTransactionDmaCompletedFinal(
            wdfDmaTransaction,
            TxDmaTransactionContextPtr->BytesTransferred,
            &dmaStatus);

        NT_ASSERT(isDone);

    } else {
        isDone = WdfDmaTransactionDmaCompleted(wdfDmaTransaction, &dmaStatus);
        TxDmaTransactionContextPtr->BytesTransferred =
            WdfDmaTransactionGetBytesTransferred(wdfDmaTransaction);
    }

    if (!isDone) {
        WdfSpinLockRelease(TxDmaTransactionContextPtr->Lock);
        return;
    }

    TxDmaTransactionContextPtr->WdfRequest = WDF_NO_HANDLE;
    WdfDmaTransactionRelease(wdfDmaTransaction);

    //
    // If request was canceled and few bytes were sent,
    // fix the status so caller gets number of bytes sent.
    //
    if ((RequestStatus == STATUS_REQUEST_CANCELED) &&
        (TxDmaTransactionContextPtr->BytesTransferred != 0)) {

        RequestStatus = STATUS_SUCCESS;
    }

    ULONG_PTR requestInfo = TxDmaTransactionContextPtr->BytesTransferred;

    if (TxDmaTransactionContextPtr->IsDeferredCancellation) {
        WdfSpinLockRelease(TxDmaTransactionContextPtr->Lock);
        WdfRequestCompleteWithInformation(
            wdfRequest,
            RequestStatus,
            requestInfo);

        return;
    }

    NTSTATUS cancelStatus = WdfRequestUnmarkCancelable(wdfRequest);
    if (NT_SUCCESS(cancelStatus) || (cancelStatus == STATUS_CANCELLED)) {
        WdfSpinLockRelease(TxDmaTransactionContextPtr->Lock);
        WdfRequestCompleteWithInformation(
            wdfRequest,
            RequestStatus,
            requestInfo);

        return;
    }

    WdfSpinLockRelease(TxDmaTransactionContextPtr->Lock);
}

_Use_decl_annotations_
NTSTATUS
IMXUartGetErrorInformation (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    )
{
    IMX_UART_REGISTERS* registersPtr = InterruptContextPtr->RegistersPtr;

    WdfInterruptAcquireLock(InterruptContextPtr->WdfInterrupt);

    //
    // Read log and acknowledge errors that are not handled through
    // interrupts.
    //

    const ULONG usr1 = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Usr1);
    const ULONG usr2 = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Usr2);

    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Usr1,
        usr1 & (IMX_UART_USR1_FRAMERR | IMX_UART_USR1_PARITYERR));

    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Usr2,
        usr2 & (IMX_UART_USR2_ORE | IMX_UART_USR2_BRCD));

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // The following error types are checked by ascending severity order,
    // which is reflected in the return status code.
    //

    if ((usr1 & IMX_UART_USR1_FRAMERR) != 0) {
        InterruptContextPtr->CommStatusErrors |= SERIAL_ERROR_FRAMING;
        status = STATUS_BAD_DATA;
    }

    if ((usr1 & IMX_UART_USR1_PARITYERR) != 0) {
        InterruptContextPtr->CommStatusErrors |= SERIAL_ERROR_PARITY;
        status = STATUS_PARITY_ERROR;
    }

    if ((usr2 & IMX_UART_USR2_BRCD) != 0) {
        InterruptContextPtr->CommStatusErrors |= SERIAL_ERROR_BREAK;
        status = STATUS_REQUEST_ABORTED;
    }

    if ((usr2 & IMX_UART_USR2_ORE) != 0) {
        InterruptContextPtr->CommStatusErrors |= SERIAL_ERROR_OVERRUN;
        status = STATUS_DATA_OVERRUN;
    }

    IMX_UART_LOG_TRACE(
        "DMA error: CommStatus: 0x%08X ",
        InterruptContextPtr->CommStatusErrors);

    WdfInterruptReleaseLock(InterruptContextPtr->WdfInterrupt);
    return status;
}

_Use_decl_annotations_
VOID
IMXUartNotifyEventsDuringDma (
    IMX_UART_DMA_TRANSACTION_CONTEXT* DmaTransactionContextPtr,
    const ULONG WaitEvents,
    const ULONG CommStatusInfo
    )
{
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        DmaTransactionContextPtr->InterruptContextPtr;

    if (CommStatusInfo != 0) {
        IMX_UART_LOG_TRACE("DMA events: CommStatus: 0x%08X ", CommStatusInfo);
    }

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
    ULONG waitEvents = WaitEvents & interruptContextPtr->WaitMask;
    interruptContextPtr->WaitEvents &= ~waitEvents;
    interruptContextPtr->CommStatusErrors |= CommStatusInfo;
    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
    if (waitEvents != 0) {
        IMX_UART_LOG_TRACE("DMA: Completing wait. "
            "(WaitEvents = 0x%lx, CommStatusInfo = 0x%lx)",
            waitEvents,
            CommStatusInfo);

        SerCx2CompleteWait(DmaTransactionContextPtr->WdfDevice, waitEvents);
    }
}

_Use_decl_annotations_
VOID
IMXUartEvtSerCx2PurgeFifos (
    WDFDEVICE WdfDevice,
    BOOLEAN PurgeRxFifo,
    BOOLEAN PurgeTxFifo
    )
{
    IMX_UART_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    IMX_UART_DEVICE_CONTEXT* deviceContextPtr =
        IMXUartGetDeviceContext(WdfDevice);

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        deviceContextPtr->InterruptContextPtr;

    IMX_UART_REGISTERS* registersPtr = deviceContextPtr->RegistersPtr;

    //
    // If DMA is being used, stop RX DMA to clear all pending
    // bytes.
    //
    if (PurgeRxFifo != FALSE) {
        NTSTATUS status = IMXUartStopRxDma(interruptContextPtr);
        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "IMXUartStopRxDma failed, (status = %!STATUS!)",
                status);
        }
    }

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);

    if (PurgeRxFifo != FALSE) {
        //
        // We assume that the FIFO is purged if the UART is off.
        //
        if ((interruptContextPtr->Ucr1Copy & IMX_UART_UCR1_UARTEN) != 0) {
            ULONG rxd;
            do {
                rxd = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Rxd);
            } while ((rxd & IMX_UART_RXD_CHARRDY) != 0);
        }

        interruptContextPtr->RxBuffer.Reset();
    }

    if (PurgeTxFifo != FALSE) {

        //
        // If transmit FIFO is not empty, reset the UART to clear the TX FIFO
        //
        const ULONG usr2 = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Usr2);
        if ((usr2 & IMX_UART_USR2_TXFE) == 0) {

            //
            // The transmit FIFO should only be nonempty if the UART is on.
            //
            NT_ASSERT((interruptContextPtr->Ucr1Copy & IMX_UART_UCR1_UARTEN) != 0);

            if (PurgeRxFifo == FALSE) {
                IMX_UART_LOG_WARNING(
                    "UART must be reset to purge TX FIFO which will also purge "
                    "RX FIFO, however RxFifo purge was not requested.");
            }

            IMXUartResetHardwareAndPreserveShadowedRegisters(interruptContextPtr);
            IMXUartRestoreHardwareFromShadowedRegisters(interruptContextPtr);

            NT_ASSERT(
                (READ_REGISTER_NOFENCE_ULONG(&registersPtr->Usr2) &
                    IMX_UART_USR2_TXFE) != 0);
        }

        interruptContextPtr->TxBuffer.Reset();
    }

    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    IMX_UART_LOG_TRACE(
        "Purged FIFOs. (PurgeRxFifo = %d, PurgeTxFifo = %d)",
        PurgeRxFifo,
        PurgeTxFifo);
}

_Use_decl_annotations_
NTSTATUS IMXUartReadDeviceProperties (
    WDFDEVICE WdfDevice,
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr
    )
{
    IMX_UART_ASSERT_MAX_IRQL(APC_LEVEL);

    ACPI_EVAL_OUTPUT_BUFFER UNALIGNED* dsdBufferPtr = nullptr;
    UINT32 dteMode = 0;
    NTSTATUS status = 0;

    //
    // Default is DCE mode if dte-mode is not present or it is equal to 0
    // DTE mode is optional if dte-mode is present and not equal to 0
    //
    DeviceContextPtr->DTEModeSelected = FALSE;

    status = AcpiQueryDsd(WdfDeviceWdmGetPhysicalDevice(WdfDevice), &dsdBufferPtr);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    const ACPI_METHOD_ARGUMENT UNALIGNED* devicePropertiesPkgPtr;
    status = AcpiParseDsdAsDeviceProperties(dsdBufferPtr, &devicePropertiesPkgPtr);
    if (!NT_SUCCESS(status)) {
        if (dsdBufferPtr != nullptr) {
            ExFreePoolWithTag(dsdBufferPtr, ACPI_TAG_EVAL_OUTPUT_BUFFER);
        }

        return status;
    }

    status = AcpiDevicePropertiesQueryIntegerValue(
        devicePropertiesPkgPtr,
        "dte-mode",
        &dteMode);
    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_WARNING(
            "dte-mode is not present. (status = %!STATUS!)",
            status);
    }
    else {
        DeviceContextPtr->DTEModeSelected = (dteMode != 0);
    }

    if (dsdBufferPtr != nullptr) {
        ExFreePoolWithTag(dsdBufferPtr, ACPI_TAG_EVAL_OUTPUT_BUFFER);
    }

    return status;
}

_Use_decl_annotations_
NTSTATUS
IMXUartEvtSerCx2ApplyConfig (
    WDFDEVICE WdfDevice,
    PVOID ConnectionParametersPtr
    )
{
    IMX_UART_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    ULONG baudRate;
    SERIAL_LINE_CONTROL lineControl;
    SERIAL_HANDFLOW handflow;
    bool rtsCtsEnabled;
    NTSTATUS status = IMXUartParseConnectionDescriptor(
            static_cast<const RH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER*>(ConnectionParametersPtr),
            &baudRate,
            &lineControl,
            &handflow,
            &rtsCtsEnabled);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to parse connection descriptor. (status = %!STATUS!)",
            status);

        return status;
    }

    return IMXUartConfigureUart(
            WdfDevice,
            baudRate,
            &lineControl,
            &handflow,
            rtsCtsEnabled);
}

_Use_decl_annotations_
NTSTATUS
IMXUartConfigureUart (
    WDFDEVICE WdfDevice,
    ULONG BaudRate,
    const SERIAL_LINE_CONTROL *LineControl,
    const SERIAL_HANDFLOW *Handflow,
    bool RtsCtsEnabled
    )
{
    IMX_UART_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    IMX_UART_DEVICE_CONTEXT* deviceContextPtr =
            IMXUartGetDeviceContext(WdfDevice);

    NTSTATUS status = IMXUartCheckValidStateForConfigIoctl(deviceContextPtr);
    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Configuration cannot be applied while IO is active. (status = %!STATUS!)",
            status);

        return status;
    }

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            deviceContextPtr->InterruptContextPtr;

    IMX_UART_REGISTERS* registersPtr = deviceContextPtr->RegistersPtr;

    deviceContextPtr->RtsCtsLinesEnabled = RtsCtsEnabled;

    if (deviceContextPtr->Initialized) {
        WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
        status = IMXUartResetHardwareAndClearShadowedRegisters(
                interruptContextPtr);

        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "Failed to reset hardware. (status = %!STATUS!)",
                status);

            return status;
        }
    }

    //
    // Set up UCR2-UCR4
    //
    {
        WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
        interruptContextPtr->Ucr2Copy =
            IMX_UART_UCR2_CTS |          // RTS asserted
            IMX_UART_UCR2_IRTS |         // CTS ignored
            IMX_UART_UCR2_TXEN |         // Enable transmitter
            IMX_UART_UCR2_RXEN |         // Enable receiver
            IMX_UART_UCR2_SRST;          // Do not reset

        interruptContextPtr->Ucr3Copy =
            IMX_UART_UCR3_DSR |          // assert DSR
            IMX_UART_UCR3_RXDMUXSEL;

        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Ucr3,
            interruptContextPtr->Ucr3Copy);

        //
        // Set RTS trigger level
        //
        const ULONG rtsTriggerLevel = min(
            IMX_UART_FIFO_COUNT,
            deviceContextPtr->Parameters.RtsTriggerLevel);

        interruptContextPtr->Ucr4Copy =
            (rtsTriggerLevel << IMX_UART_UCR4_CTSTL_SHIFT);

        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Ucr4,
            interruptContextPtr->Ucr4Copy);

        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
    }

    status = IMXUartSetBaudRate(deviceContextPtr, BaudRate);
    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to set baud rate. (status = %!STATUS!, baudRate = %lu)",
            status,
            BaudRate);

        return status;
    }

    status = IMXUartSetLineControl(deviceContextPtr, LineControl);
    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to set line control. (status = %!STATUS!)",
            status);

        return status;
    }

    status = IMXUartSetHandflow(deviceContextPtr, Handflow);
    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to set handflow. (status = %!STATUS!)",
            status);

        return status;
    }

    //
    // Read Device Properties - check if dte-mode is present.
    //
    status = IMXUartReadDeviceProperties(WdfDevice, deviceContextPtr);

    //
    // Enable the UART
    //
    {
        WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);

        interruptContextPtr->Ucr1Copy |= IMX_UART_UCR1_UARTEN;
        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Ucr1,
            interruptContextPtr->Ucr1Copy);

        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
    }

    deviceContextPtr->Initialized = true;
    return STATUS_SUCCESS;
}

//
// UART configuration changes (Baud, Handflow, etc.) are only allowed before
// the first read or write IRP is sent. Changing line configuration while
// data transmission is in progress could have unpredictable effects.
//
NTSTATUS
IMXUartCheckValidStateForConfigIoctl (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr
    )
{
    const IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            DeviceContextPtr->InterruptContextPtr;

    switch (interruptContextPtr->RxState) {
    case IMX_UART_STATE::WAITING_FOR_INTERRUPT:
    case IMX_UART_STATE::WAITING_FOR_DPC:
        return STATUS_INVALID_DEVICE_STATE;
    }

    switch (interruptContextPtr->RxDmaState) {
    case IMX_UART_STATE::WAITING_FOR_DPC:
        return STATUS_INVALID_DEVICE_STATE;
    }

    switch (interruptContextPtr->TxState) {
    case IMX_UART_STATE::WAITING_FOR_INTERRUPT:
    case IMX_UART_STATE::WAITING_FOR_DPC:
        return STATUS_INVALID_DEVICE_STATE;
    }

    switch (interruptContextPtr->TxDrainState) {
    case IMX_UART_STATE::WAITING_FOR_INTERRUPT:
    case IMX_UART_STATE::WAITING_FOR_DPC:
        return STATUS_INVALID_DEVICE_STATE;
    }

    switch (interruptContextPtr->TxPurgeState) {
    case IMX_UART_STATE::WAITING_FOR_INTERRUPT:
    case IMX_UART_STATE::WAITING_FOR_DPC:
        return STATUS_INVALID_DEVICE_STATE;
    }

    switch (interruptContextPtr->TxDmaState) {
    case IMX_UART_STATE::WAITING_FOR_DPC:
        return STATUS_INVALID_DEVICE_STATE;
    }

    switch (interruptContextPtr->TxDmaDrainState) {
    case IMX_UART_STATE::WAITING_FOR_INTERRUPT:
    case IMX_UART_STATE::WAITING_FOR_DPC:
        return STATUS_INVALID_DEVICE_STATE;
    }

    return STATUS_SUCCESS;
}

//
// Follow procedure in datasheet section 64.10.2 to reset hardware.
//
NTSTATUS
IMXUartResetHardwareAndPreserveShadowedRegisters (
    const IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    )
{
    IMX_UART_REGISTERS* registersPtr = InterruptContextPtr->RegistersPtr;

    //
    // Disable UART and reset control registers
    //
    WRITE_REGISTER_NOFENCE_ULONG(&registersPtr->Ucr1, 0);
    WRITE_REGISTER_NOFENCE_ULONG(&registersPtr->Ucr3, 0);
    WRITE_REGISTER_NOFENCE_ULONG(&registersPtr->Ucr4, 0);
    WRITE_REGISTER_NOFENCE_ULONG(&registersPtr->Umcr, 0);

    //
    // Initiate reset by clearing IMX_UART_UCR2_SRST bit
    //
    WRITE_REGISTER_NOFENCE_ULONG(&registersPtr->Ucr2, 0);

    //
    // Wait for SOFTRST bit to deassert
    //
    ULONG uts;
    int i;
    for (i = 1000; i != 0; --i) {
        KeStallExecutionProcessor(1);
        uts = READ_REGISTER_NOFENCE_ULONG(&registersPtr->Uts);
        if ((uts & IMX_UART_UTS_SOFTRST) == 0) {
            return STATUS_SUCCESS;
        }
    }

    IMX_UART_LOG_ERROR(
        "Timed out waiting for SOFTRST bit to clear. (uts = 0x%lx)",
        uts);

    NT_ASSERT(FALSE);
    return STATUS_IO_TIMEOUT;
}

NTSTATUS
IMXUartResetHardwareAndClearShadowedRegisters (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    )
{
    NTSTATUS status = IMXUartResetHardwareAndPreserveShadowedRegisters(
            InterruptContextPtr);

    InterruptContextPtr->Ucr1Copy = 0;
    InterruptContextPtr->Ucr2Copy = IMX_UART_UCR2_SRST;
    InterruptContextPtr->Ucr3Copy = 0;
    InterruptContextPtr->Ucr4Copy = 0;
    InterruptContextPtr->UbirCopy = 0;
    InterruptContextPtr->UbmrCopy = 0;
    InterruptContextPtr->UfcrCopy = 0;

    return status;
}

void
IMXUartRestoreHardwareFromShadowedRegisters (
    const IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    )
{
    IMX_UART_REGISTERS* registersPtr = InterruptContextPtr->RegistersPtr;

    //
    // Precondition: UART should be disabled
    //
    NT_ASSERT(
        (READ_REGISTER_NOFENCE_ULONG(&registersPtr->Ucr1) &
        IMX_UART_UCR1_UARTEN) == 0);

    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr2,
        InterruptContextPtr->Ucr2Copy);

    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr3,
        InterruptContextPtr->Ucr3Copy);

    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr4,
        InterruptContextPtr->Ucr4Copy);

    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ufcr,
        InterruptContextPtr->UfcrCopy);

    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ubir,
        InterruptContextPtr->UbirCopy);

    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ubmr,
        InterruptContextPtr->UbmrCopy);

    //
    // Write Ucr1 last because it contains the UART enable bit
    //
    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr1,
        InterruptContextPtr->Ucr1Copy);
}

_Use_decl_annotations_
void
IMXUartComputeFifoThresholds (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    ULONG BaudRate,
    ULONG BitsPerFrame,
    ULONG* TxFifoThresholdPtr,
    ULONG* RxFifoThresholdPtr
    )
{
    //
    // Compute the minimum number of characters that take just longer to
    // transmit than the time threshold.
    //
    ULONG txThreshold = IMXUartComputeCharactersPerDuration(
            DeviceContextPtr->Parameters.TxFifoThresholdUs,
            BaudRate,
            BitsPerFrame);

    //
    // Clamp in range [2, IMX_UART_FIFO_COUNT]
    //
    if (txThreshold < 2) {
        txThreshold = 2;
    } else if (txThreshold > IMX_UART_FIFO_COUNT) {
        txThreshold = IMX_UART_FIFO_COUNT;
    }

    ULONG rxThreshold = IMXUartComputeCharactersPerDuration(
            DeviceContextPtr->Parameters.RxFifoThresholdUs,
            BaudRate,
            BitsPerFrame);

    //
    // Clamp in range [1, IMX_UART_FIFO_COUNT - 1], which will cause
    // IMX_UART_FIFO_COUNT - rxThreshold to be in the same range.
    //
    if (rxThreshold < 1) {
        rxThreshold = 1;
    } else if (rxThreshold > (IMX_UART_FIFO_COUNT - 1)) {
        rxThreshold = IMX_UART_FIFO_COUNT - 1;
    }

    *TxFifoThresholdPtr = txThreshold;
    *RxFifoThresholdPtr = IMX_UART_FIFO_COUNT - rxThreshold;
}

ULONG
IMXUartComputeCharactersPerDuration (
    ULONG DurationUs,
    ULONG BaudRate,
    ULONG BitsPerFrame
    )
{
    //
    // Compute the FIFO threshold so there's at least this much time between
    // when the interrupt is asserted and when the FIFO goes empty (or full).
    //
    const ULONG minimumTime = ULONG(DurationUs * 10000000ULL / 1000000ULL);
    const ULONG timePerFrame = BitsPerFrame * 10000000UL / BaudRate;

    return (minimumTime + (timePerFrame - 1)) / timePerFrame;
}

//
// This routine has been unit tested and will compute baud rates as follows:
//
// DesiredBaud     ActualBaud     RfDiv     Ubir     Ubmr    Error (%)
//        1200    1199.197628         7      109    65519 0.066864
//        2400    2398.395256         7      219    65519 0.066864
//        4800    4796.790511         7      439    65519 0.066864
//        9600    9593.581022         7      879    65519 0.066864
//       19200   19198.063841         7     1760    65519 0.010084
//       38400   38396.127682         7     3521    65519 0.010084
//       76800   76792.255364         7     7043    65519 0.010084
//      115200  115199.284842         7    10566    65519 0.000621
//      230400  230398.569684         7    21133    65519 0.000621
//      460800  460797.139369         7    42267    65519 0.000621
//      614400  614392.551893         7    56356    65519 0.001212
//      921600  921596.459096         5    60382    65519 0.000384
//     1843200 1843177.655678         2    48305    65519 0.001212
//     3686400 3686355.311355         1    48305    65519 0.001212
//     5000000 5000000.000000         1    65519    65519 0.000000
// Baud not supported: 5000001
//
NTSTATUS
IMXUartComputeDividers (
    ULONG UartModuleClock,
    ULONG DesiredBaudRate,
    _Out_ ULONG* RfDivPtr,
    _Out_ ULONG* UbirPtr,
    _Out_ ULONG* UbmrPtr
    )
{
    ULONG rfDiv = UartModuleClock / (16 * DesiredBaudRate);
    if (rfDiv == 0) {
        IMX_UART_LOG_ERROR(
            "Baud rate is too high. (DesiredBaudRate = %lu, UartModuleClock = %lu)",
            DesiredBaudRate,
            UartModuleClock);

        return STATUS_NOT_SUPPORTED;
    }

    rfDiv = min(7, rfDiv);

    const ULONG num = UartModuleClock;
    const ULONG denom = 16 * rfDiv * DesiredBaudRate;
    const ULONG scale = (max(num, denom) + 65535) / 65536;
    const ULONG ubmrPlusOne = num / scale;
    const ULONG ubirPlusOne = denom / scale;

    if ((ubmrPlusOne == 0) || (ubirPlusOne == 0)) {
        IMX_UART_LOG_ERROR(
            "Baud rate is too high. (DesiredBaudRate = %lu, UartModuleClock = %lu)",
            DesiredBaudRate,
            UartModuleClock);

        return STATUS_NOT_SUPPORTED;
    }

    *RfDivPtr = rfDiv;
    *UbirPtr = ubirPlusOne - 1;
    *UbmrPtr = ubmrPlusOne - 1;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
IMXUartSetBaudRate (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    ULONG BaudRate
    )
{
    IMX_UART_REGISTERS* registersPtr = DeviceContextPtr->RegistersPtr;

    ULONG rfDiv;
    ULONG ubir;
    ULONG ubmr;
    NTSTATUS status = IMXUartComputeDividers(
            DeviceContextPtr->Parameters.ModuleClockFrequency,
            BaudRate,
            &rfDiv,
            &ubir,
            &ubmr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "IMXUartComputeDividers(...) failed. (status = %!STATUS!)",
            status);

        return status;
    }

    //
    // Baud rate could get set before line control settings, so just assume
    // the typical case of 10 bits per frame.
    //
    const ULONG bitsPerFrame8N1 = 10;
    ULONG txThreshold;
    ULONG rxThreshold;
    IMXUartComputeFifoThresholds(
        DeviceContextPtr,
        BaudRate,
        bitsPerFrame8N1,
        &txThreshold,
        &rxThreshold);

    NT_ASSERT((txThreshold >= 2) && (txThreshold <= IMX_UART_FIFO_COUNT));
    NT_ASSERT((rxThreshold >= 1) && (rxThreshold <= (IMX_UART_FIFO_COUNT - 1)));

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            DeviceContextPtr->InterruptContextPtr;

    ULONG txDpcThreshold = IMXUartComputeCharactersPerDuration(
            DeviceContextPtr->Parameters.TxDpcThresholdUs,
            BaudRate,
            bitsPerFrame8N1);

    txDpcThreshold = min(
        DeviceContextPtr->InterruptContextPtr->TxBuffer.Size - 1,
        txDpcThreshold + 1);

    //
    // Update registers under the interrupt spinlock
    //
    {
        WdfInterruptAcquireLock(DeviceContextPtr->WdfInterrupt);

        interruptContextPtr->TxDpcThreshold = txDpcThreshold;

        interruptContextPtr->UfcrCopy =
            (txThreshold << IMX_UART_UFCR_TXTL_SHIFT) |
            IMXUartRfDivMaskFromIntegerValue(rfDiv) |
            (DeviceContextPtr->DTEModeSelected ? IMX_UART_UFCR_DCEDTE : 0) |
            (rxThreshold << IMX_UART_UFCR_RXTL_SHIFT);

        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Ufcr,
            interruptContextPtr->UfcrCopy);

        interruptContextPtr->UbirCopy = ubir;
        WRITE_REGISTER_NOFENCE_ULONG(&registersPtr->Ubir, ubir);

        interruptContextPtr->UbmrCopy = ubmr;
        WRITE_REGISTER_NOFENCE_ULONG(&registersPtr->Ubmr, ubmr);

        DeviceContextPtr->CurrentBaudRate = BaudRate;
        WdfInterruptReleaseLock(DeviceContextPtr->WdfInterrupt);
    }

    IMX_UART_LOG_INFORMATION(
        "Successfully set baud rate. "
        "(BaudRate = %lu, rfDiv = %lu, ubir = %lu, ubmr = %lu, "
        "rxThreshold = %lu, txThreshold = %lu, txDpcThreshold = %lu)",
        BaudRate,
        rfDiv,
        ubir,
        ubmr,
        rxThreshold,
        txThreshold,
        txDpcThreshold);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
IMXUartSetLineControl (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    const SERIAL_LINE_CONTROL* LineControlPtr
    )
{
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            DeviceContextPtr->InterruptContextPtr;

    ULONG setMask = 0;
    ULONG clearMask = 0;

    switch (LineControlPtr->StopBits) {
    case STOP_BIT_1:
        clearMask |= IMX_UART_UCR2_STPB;
        break;

    case STOP_BITS_2:
        setMask |= IMX_UART_UCR2_STPB;
        break;

    case STOP_BITS_1_5:
        IMX_UART_LOG_ERROR("STOP_BITS_1_5 is not supported.");
        return STATUS_INVALID_PARAMETER;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    switch (LineControlPtr->Parity) {
    case NO_PARITY:
        clearMask |= IMX_UART_UCR2_PREN;
        break;

    case ODD_PARITY:
        setMask |= IMX_UART_UCR2_PREN;
        setMask |= IMX_UART_UCR2_PROE;
        break;

    case EVEN_PARITY:
        setMask |= IMX_UART_UCR2_PREN;
        clearMask |= IMX_UART_UCR2_PROE;
        break;

    case MARK_PARITY:
    case SPACE_PARITY:
        IMX_UART_LOG_ERROR(
            "MARK_PARITY and SPACE_PARITY are not supported. (LineControlPtr->Parity = %d)",
            LineControlPtr->Parity);

        return STATUS_INVALID_PARAMETER;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    switch (LineControlPtr->WordLength) {
    case 7:
        clearMask |= IMX_UART_UCR2_WS;
        break;

    case 8:
        setMask |= IMX_UART_UCR2_WS;
        break;

    default:
        IMX_UART_LOG_ERROR(
            "Unsupported word length: %d",
            LineControlPtr->WordLength);

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Set mask and clear mask should not overlap
    //
    NT_ASSERT((setMask & clearMask) == 0);

    IMX_UART_LOG_INFORMATION(
        "Updating line control. (setMask = 0x%lx, clearMask = 0x%lx, StopBits = %d, Parity = %d, WordLength = %d)",
        setMask,
        clearMask,
        LineControlPtr->StopBits,
        LineControlPtr->Parity,
        LineControlPtr->WordLength);

    //
    // Update UCR2 register under the interrupt spinlock
    //
    {
        WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);

        interruptContextPtr->Ucr2Copy &= ~clearMask;
        interruptContextPtr->Ucr2Copy |= setMask;
        WRITE_REGISTER_NOFENCE_ULONG(
            &interruptContextPtr->RegistersPtr->Ucr2,
            interruptContextPtr->Ucr2Copy);

        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
    }

    return STATUS_SUCCESS;
}

//
// The IMX6 Datasheet and Windows meanings of RTS/CTS are swapped.
//
// RTS is an output pin and has two modes:
//
//    SERIAL_RTS_CONTROL - whoever has a handle open to the serial port
//                         can manually set the state of RTS using
//                         the IOCTL_SERIAL_SET_RTS/IOCTL_SERIAL_CLR_RTS
//                         IOCTLs
//
//    SERIAL_RTS_HANDSHAKE - the UART hardware automatically sets and
//                           clears RTS depending on RX FIFO level. It is
//                           illegal to send IOCTL_SERIAL_SET/CLR_RTS
//                           in this mode.
//
// On the IMX6 UART controller, RTS is controlled by CTSC bit, which selects
// between manual and automatic control of RTS. When the controller is in
// manual RTS mode, the CTS bit controls the state of RTS (makes sense!).
//
// CTS is an input pin and has two modes:
//
//    SERIAL_CTS_HANDSHAKE bit set - the transmitter hardware only sends
//                                   bytes when the CTS pin is high.
//
//    SERIAL_CTS_HANDSHAKE bit clear - the transmitter hardware
//                                     ignores CTS and always sends bytes.
//
// On the IMX6 UART controller, CTS is controller by the IRTS bit, which
// selects between ignoring and paying attention to the CTS line.
//
_Use_decl_annotations_
NTSTATUS
IMXUartSetHandflow (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    const SERIAL_HANDFLOW* LineControlPtr
    )
{
    ULONG setMask = 0;
    ULONG clearMask = 0;

    //
    // Handle output flow control setting. The transmitter on this UART can
    // optionally monitor the CTS pin - an input on this UART - to control when
    // the transmitter hardware sends data.
    //
    switch (LineControlPtr->ControlHandShake & SERIAL_OUT_HANDSHAKEMASK) {
    case 0:

        //
        // Ignore CTS (an input pin). Since the IMX6 datasheet swaps the
        // meaning of RTS and CTS, we set the "Ignore RTS bit", which
        // really means to ignore CTS.
        //
        setMask |= IMX_UART_UCR2_IRTS;
        break;

    case SERIAL_CTS_HANDSHAKE:

        //
        // Pay attention to CTS. Since the IMX6 datasheet swaps the meaning
        // of RTS and CTS, we clear the "Ignore RTS bit", which really means
        // to pay attention to CTS.
        //
        if (!DeviceContextPtr->RtsCtsLinesEnabled) {
            IMX_UART_LOG_ERROR(
                "RTS/CTS flow control is not allowed on this UART instance "
                "since RTS/CTS lines were not specified in the "
                "SerialLinesEnabled mask in the UartSerialBus() connection "
                "descriptor.");

            return STATUS_INVALID_DEVICE_STATE;
        }

        clearMask |= IMX_UART_UCR2_IRTS;
        break;

    case SERIAL_DSR_HANDSHAKE:
    case SERIAL_DCD_HANDSHAKE:
    default:
        IMX_UART_LOG_ERROR(
            "Unsupported flow control setting. (LineControlPtr->ControlHandShake = 0x%lx)",
            LineControlPtr->ControlHandShake);

        return STATUS_NOT_SUPPORTED;
    }

    if ((LineControlPtr->ControlHandShake & ~SERIAL_OUT_HANDSHAKEMASK) != 0) {
        IMX_UART_LOG_ERROR(
            "Unsupported ControlHandShake flag. "
            "(LineControlPtr->ControlHandShake = 0x%lx)",
            LineControlPtr->ControlHandShake);

        return STATUS_NOT_SUPPORTED;
    }

    //
    // Handle RTS flow control. The receiver on this UART can optionally
    // set the RTS pin - an output on this UART - depending on FIFO level.
    // Or the user can control it manually with IOCTLs.
    //
    switch (LineControlPtr->FlowReplace & SERIAL_RTS_MASK) {
    case 0:

        //
        // Treat no RTS setting the same as manual control
        //
        clearMask |= IMX_UART_UCR2_CTSC;
        setMask |= IMX_UART_UCR2_CTS;
        break;

    case SERIAL_RTS_CONTROL:

        if (!DeviceContextPtr->RtsCtsLinesEnabled) {
            IMX_UART_LOG_WARNING(
                "SERIAL_RTS_CONTROL was specified, but RTS/CTS were not "
                "specified in the SerialLinesEnabled mask in the "
                "UartSerialBus() connection descriptor, so RTS/CTS may not "
                "be pinned out on this UART.");
        }

        //
        // Enable the CTS bit to control the RTS line and ensure line is
        // initially asserted.
        //
        clearMask |= IMX_UART_UCR2_CTSC;
        setMask |= IMX_UART_UCR2_CTS;
        break;

    case SERIAL_RTS_HANDSHAKE:

        if (!DeviceContextPtr->RtsCtsLinesEnabled) {
            IMX_UART_LOG_WARNING(
                "SERIAL_RTS_HANDSHAKE was specified, but RTS/CTS were not "
                "specified in the SerialLinesEnabled mask in the "
                "UartSerialBus() connection descriptor, so RTS/CTS may not "
                "be pinned out on this UART.");
        }

        //
        // Enable hardware control of the CTS (actually the RTS) line.
        //
        setMask |= IMX_UART_UCR2_CTSC;
        break;

    case SERIAL_TRANSMIT_TOGGLE:
    default:
        IMX_UART_LOG_ERROR(
            "Unsupported RTS flow control value. "
            "(LineControlPtr->FlowReplace = 0x%lx)",
            LineControlPtr->FlowReplace);

        return STATUS_NOT_SUPPORTED;
    }

    if ((LineControlPtr->FlowReplace & ~SERIAL_RTS_MASK) != 0) {
        IMX_UART_LOG_ERROR(
            "Unsupported FlowReplace flag. (LineControlPtr->FlowReplace = 0x%lx)",
            LineControlPtr->FlowReplace);

        return STATUS_NOT_SUPPORTED;
    }

    NT_ASSERT((setMask & clearMask) == 0);

    //
    // Ignore XonLimit/XoffLimit since we do not support Xon/Xoff flow control
    //

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            DeviceContextPtr->InterruptContextPtr;

    //
    // Update UCR2 under the interrupt spinlock
    //
    {
        WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);

        interruptContextPtr->Ucr2Copy &= ~clearMask;
        interruptContextPtr->Ucr2Copy |= setMask;
        WRITE_REGISTER_NOFENCE_ULONG(
            &interruptContextPtr->RegistersPtr->Ucr2,
            interruptContextPtr->Ucr2Copy);

        DeviceContextPtr->CurrentFlowReplace = LineControlPtr->FlowReplace;
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
    }

    return STATUS_SUCCESS;
}


_Use_decl_annotations_
void
IMXUartSetBreakOnOff (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    bool BreakOn
    )
{
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            DeviceContextPtr->InterruptContextPtr;

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);

    if (BreakOn) {
        interruptContextPtr->Ucr1Copy |= IMX_UART_UCR1_SNDBRK;
    } else {
        interruptContextPtr->Ucr1Copy &= ~IMX_UART_UCR1_SNDBRK;
    }

    WRITE_REGISTER_NOFENCE_ULONG(
        &interruptContextPtr->RegistersPtr->Ucr1,
        interruptContextPtr->Ucr1Copy);

    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    IMX_UART_LOG_TRACE("Set break state. (BreakOn = %d)", BreakOn);
}


_Use_decl_annotations_
void
IMXUartSetWaitMask (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    ULONG WaitMask
    )
{
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            DeviceContextPtr->InterruptContextPtr;

    IMX_UART_REGISTERS* registersPtr =
            interruptContextPtr->RegistersPtr;

    //
    // Mask of bits that changed from the old wait mask to the new wait mask
    //
    const ULONG waitMaskDifference = interruptContextPtr->WaitMask ^ WaitMask;

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);

    //
    // Nothing to do here for SERIAL_EV_RXCHAR or SERIAL_EV_ERR -
    // the ISR handles everything
    //

    if ((waitMaskDifference & SERIAL_EV_BREAK) != 0) {
        if ((WaitMask & SERIAL_EV_BREAK) != 0) {
            interruptContextPtr->Ucr4Copy |= IMX_UART_UCR4_BKEN;
            interruptContextPtr->Usr2EnabledInterruptsMask |= IMX_UART_USR2_BRCD;
        } else {
            interruptContextPtr->Ucr4Copy &= ~IMX_UART_UCR4_BKEN;
            interruptContextPtr->Usr2EnabledInterruptsMask &= ~IMX_UART_USR2_BRCD;
        }

        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Ucr4,
            interruptContextPtr->Ucr4Copy);

        //
        // Clear BRCD bit
        //
        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Usr2,
            IMX_UART_USR2_BRCD);
    }

    if ((waitMaskDifference & SERIAL_EV_TXEMPTY) != 0) {
        if ((WaitMask & SERIAL_EV_TXEMPTY) != 0) {
            interruptContextPtr->Ucr1Copy |= IMX_UART_UCR1_TXMPTYEN;
            interruptContextPtr->Usr2EnabledInterruptsMask |= IMX_UART_USR2_TXFE;
            WRITE_REGISTER_NOFENCE_ULONG(
                &registersPtr->Ucr1,
                interruptContextPtr->Ucr1Copy);
        } else {
            // Always disabled by ISR
        }
    }

    if ((waitMaskDifference & SERIAL_EV_CTS) != 0) {
        if ((WaitMask & SERIAL_EV_CTS) != 0) {
            interruptContextPtr->Ucr2Copy &= ~IMX_UART_UCR2_RTEC_MASK;
            interruptContextPtr->Ucr2Copy |=
                IMX_UART_UCR2_RTSEN | IMX_UART_UCR2_RTEC_BOTH;

            interruptContextPtr->Usr2EnabledInterruptsMask |= IMX_UART_USR2_RTSF;
        } else {
            interruptContextPtr->Ucr2Copy &= ~IMX_UART_UCR2_RTSEN;
            interruptContextPtr->Usr2EnabledInterruptsMask &= ~IMX_UART_USR2_RTSF;
        }

        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Ucr2,
            interruptContextPtr->Ucr2Copy);

        //
        // Clear RTSF bit
        //
        WRITE_REGISTER_NOFENCE_ULONG(
            &registersPtr->Usr2,
            IMX_UART_USR2_RTSF);
    }

    interruptContextPtr->WaitMask = WaitMask;
    interruptContextPtr->WaitEvents = 0;
    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);

    IMX_UART_LOG_TRACE("Successfully set wait mask. (WaitMask = 0x%lx)", WaitMask);
}

_Use_decl_annotations_
VOID
IMXUartIoctlSetBaudRate (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    NTSTATUS status = IMXUartCheckValidStateForConfigIoctl(DeviceContextPtr);
    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_WARNING(
            "Behavior is undefined if baud is set while IO is active. (status = %!STATUS!)",
            status);
    }

    SERIAL_BAUD_RATE* inputBufferPtr;
    status = WdfRequestRetrieveInputBuffer(
            WdfRequest,
            sizeof(*inputBufferPtr),
            reinterpret_cast<PVOID*>(&inputBufferPtr),
            nullptr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to retreive input buffer for IOCTL_SERIAL_SET_BAUD_RATE request. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    status = IMXUartSetBaudRate(DeviceContextPtr, inputBufferPtr->BaudRate);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(WdfRequest, status);
        return;
    }

    status = IMXUartUpdateDmaSettings(DeviceContextPtr);
    WdfRequestComplete(WdfRequest, status);
}

_Use_decl_annotations_
VOID
IMXUartIoctlGetBaudRate (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    SERIAL_BAUD_RATE* outputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to retrieve output buffer for IOCTL_SERIAL_GET_BAUD_RATE request. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    outputBufferPtr->BaudRate = DeviceContextPtr->CurrentBaudRate;
    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
void
IMXUartIoctlSetLineControl (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    NTSTATUS status = IMXUartCheckValidStateForConfigIoctl(DeviceContextPtr);
    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_WARNING(
            "Behavior is undefined if line control is modified while IO is active. (status = %!STATUS!)",
            status);
    }

    SERIAL_LINE_CONTROL* inputBufferPtr;
    status = WdfRequestRetrieveInputBuffer(
            WdfRequest,
            sizeof(*inputBufferPtr),
            reinterpret_cast<PVOID*>(&inputBufferPtr),
            nullptr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to retrieve input buffer for IOCTL_SERIAL_SET_LINE_CONTROL request. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    status = IMXUartSetLineControl(DeviceContextPtr, inputBufferPtr);
    WdfRequestComplete(WdfRequest, status);
}

_Use_decl_annotations_
void
IMXUartIoctlGetLineControl (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    SERIAL_LINE_CONTROL* outputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to retrieve output buffer for IOCTL_SERIAL_GET_LINE_CONTROL request. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    WdfInterruptAcquireLock(DeviceContextPtr->WdfInterrupt);
    const ULONG ucr2 = DeviceContextPtr->InterruptContextPtr->Ucr2Copy;
    WdfInterruptReleaseLock(DeviceContextPtr->WdfInterrupt);

    if ((ucr2 & IMX_UART_UCR2_STPB) == 0) {
        outputBufferPtr->StopBits = STOP_BIT_1;
    } else {
        outputBufferPtr->StopBits = STOP_BITS_2;
    }

    if ((ucr2 & IMX_UART_UCR2_PREN) == 0) {
        outputBufferPtr->Parity = NO_PARITY;
    } else {
        if ((ucr2 & IMX_UART_UCR2_PROE) == 0) {
            outputBufferPtr->Parity = EVEN_PARITY;
        } else {
            outputBufferPtr->Parity = ODD_PARITY;
        }
    }

    if ((ucr2 & IMX_UART_UCR2_WS) == 0) {
        outputBufferPtr->WordLength = 7;
    } else {
        outputBufferPtr->WordLength = 8;
    }

    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
void
IMXUartIoctlSetHandflow (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    NTSTATUS status = IMXUartCheckValidStateForConfigIoctl(DeviceContextPtr);
    if (!NT_SUCCESS(status)) {
        //
        // This is a warning because the MITT tests set handflow after
        // performing IO.
        //
        IMX_UART_LOG_WARNING(
            "Behavior is undefined if line control is set after IO has been started. (status = %!STATUS!)",
            status);
    }

    SERIAL_HANDFLOW* inputBufferPtr;
    status = WdfRequestRetrieveInputBuffer(
            WdfRequest,
            sizeof(*inputBufferPtr),
            reinterpret_cast<PVOID*>(&inputBufferPtr),
            nullptr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to retrieve input buffer for IOCTL_SERIAL_SET_HANDFLOW request. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    status = IMXUartSetHandflow(DeviceContextPtr, inputBufferPtr);
    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to set handflow. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    WdfRequestComplete(WdfRequest, STATUS_SUCCESS);
}

_Use_decl_annotations_
void
IMXUartIoctlGetHandflow (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    SERIAL_HANDFLOW* outputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to retrieve output buffer for IOCTL_SERIAL_GET_HANDFLOW request. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    WdfInterruptAcquireLock(DeviceContextPtr->WdfInterrupt);
    const ULONG ucr2 = DeviceContextPtr->InterruptContextPtr->Ucr2Copy;
    WdfInterruptReleaseLock(DeviceContextPtr->WdfInterrupt);

    *outputBufferPtr = SERIAL_HANDFLOW();

    //
    // If the Ignore RTS bit (which is really the Ignore CTS bit) is
    // NOT set, then the controller is paying attention to CTS, which
    // means we're using CTS handshaking.
    //
    if ((ucr2 & IMX_UART_UCR2_IRTS) == 0) {
        outputBufferPtr->ControlHandShake |= SERIAL_CTS_HANDSHAKE;
    }

    if ((ucr2 & IMX_UART_UCR2_CTSC) != 0) {

        //
        // If the CTS Pin control bit (which is really the RTS pin control bit)
        // is set, it means RTS is automatically controlled by hardware.
        //
        outputBufferPtr->FlowReplace |= SERIAL_RTS_HANDSHAKE;

    } else {
        if ((DeviceContextPtr->CurrentFlowReplace & SERIAL_RTS_MASK) ==
             SERIAL_RTS_CONTROL)
        {
            //
            // If the CTS Pin control bit (which is really the RTS pin control bit)
            // is clear, it means RTS is under manual control.
            //
            outputBufferPtr->FlowReplace |= SERIAL_RTS_CONTROL;
        }
    }

    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
void
IMXUartIoctlGetProperties (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    SERIAL_COMMPROP* outputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to retrieve output buffer for IOCTL_SERIAL_GET_PROPERTIES request. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    const IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            DeviceContextPtr->InterruptContextPtr;

    *outputBufferPtr = SERIAL_COMMPROP();

    outputBufferPtr->PacketLength = sizeof(*outputBufferPtr);
    outputBufferPtr->PacketVersion = 2;
    outputBufferPtr->ServiceMask = SERIAL_SP_SERIALCOMM;
    outputBufferPtr->MaxTxQueue = interruptContextPtr->TxBuffer.Capacity();
    outputBufferPtr->MaxRxQueue = interruptContextPtr->RxBuffer.Capacity();
    outputBufferPtr->MaxBaud =
        DeviceContextPtr->Parameters.ModuleClockFrequency / 16;

    outputBufferPtr->ProvSubType = SERIAL_SP_RS232;
    outputBufferPtr->ProvCapabilities =
        SERIAL_PCF_RTSCTS |
        SERIAL_PCF_PARITY_CHECK |
        SERIAL_PCF_TOTALTIMEOUTS |
        SERIAL_PCF_INTTIMEOUTS;

    outputBufferPtr->SettableParams =
        SERIAL_SP_PARITY |
        SERIAL_SP_BAUD |
        SERIAL_SP_DATABITS |
        SERIAL_SP_STOPBITS |
        SERIAL_SP_HANDSHAKING |
        SERIAL_SP_PARITY_CHECK;

    outputBufferPtr->SettableBaud = SERIAL_BAUD_USER;
    outputBufferPtr->SettableData = SERIAL_DATABITS_7 | SERIAL_DATABITS_8;
    outputBufferPtr->SettableStopParity =
        SERIAL_STOPBITS_10 |
        SERIAL_STOPBITS_20 |
        SERIAL_PARITY_NONE |
        SERIAL_PARITY_ODD |
        SERIAL_PARITY_EVEN;

    outputBufferPtr->CurrentTxQueue = 0;
    outputBufferPtr->CurrentRxQueue = 0;

    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
void
IMXUartIoctlGetCommStatus (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    SERIAL_STATUS* outputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to retrieve output buffer for IOCTL_SERIAL_GET_COMMSTATUS request. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            DeviceContextPtr->InterruptContextPtr;

    *outputBufferPtr = SERIAL_STATUS();

    //
    // Read and reset error mask
    //
    {
        WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);
        outputBufferPtr->Errors = interruptContextPtr->CommStatusErrors;
        interruptContextPtr->CommStatusErrors = 0;
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
    }

    //
    // If output CTS flow control is on, check the status of the RTSS bit
    // (which really indicates states of CTS pin)
    //
    if ((interruptContextPtr->Ucr2Copy & IMX_UART_UCR2_IRTS) == 0) {
        const ULONG usr1 = READ_REGISTER_NOFENCE_ULONG(
                &interruptContextPtr->RegistersPtr->Usr1);

        if ((usr1 & IMX_UART_USR1_RTSS) != 0) {
            outputBufferPtr->HoldReasons |= SERIAL_TX_WAITING_FOR_CTS;
        }
    }

    //
    // Check if break is currently in progress
    //
    if ((interruptContextPtr->Ucr1Copy & IMX_UART_UCR1_SNDBRK) != 0) {
        outputBufferPtr->HoldReasons |= SERIAL_TX_WAITING_ON_BREAK;
    }

    outputBufferPtr->AmountInInQueue = 0;
    outputBufferPtr->AmountInOutQueue = interruptContextPtr->TxBuffer.Count();
    outputBufferPtr->EofReceived = FALSE;
    outputBufferPtr->WaitForImmediate = FALSE;

    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
void
IMXUartIoctlGetModemStatus (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    //
    // Define locally so we are less likely to conflict with future
    // public definitions.
    //
    enum SERIAL_MSR : ULONG {
        //
        // This bit is the delta clear to send.  It is used to indicate
        // that the clear to send bit (in this register) has *changed*
        // since this register was last read by the CPU.
        //
        SERIAL_MSR_DCTS = 0x01,

        //
        // This bit is the delta data set ready.  It is used to indicate
        // that the data set ready bit (in this register) has *changed*
        // since this register was last read by the CPU.
        //
        SERIAL_MSR_DDSR = 0x02,

        //
        // This is the trailing edge ring indicator.  It is used to indicate
        // that the ring indicator input has changed from a low to high state.
        //
        SERIAL_MSR_TERI = 0x04,

        //
        // This bit is the delta data carrier detect.  It is used to indicate
        // that the data carrier bit (in this register) has *changed*
        // since this register was last read by the CPU.
        //
        SERIAL_MSR_DDCD = 0x08,

        //
        // This bit contains the (complemented) state of the clear to send
        // (CTS) line.
        //
        SERIAL_MSR_CTS = 0x10,

        //
        // This bit contains the (complemented) state of the data set ready
        // (DSR) line.
        //
        SERIAL_MSR_DSR = 0x20,

        //
        // This bit contains the (complemented) state of the ring indicator
        // (RI) line.
        //
        SERIAL_MSR_RI = 0x40,

        //
        // This bit contains the (complemented) state of the data carrier detect
        // (DCD) line.
        //
        SERIAL_MSR_DCD = 0x80,
    };

    ULONG* outputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to retrieve output buffer for IOCTL_SERIAL_GET_MODEMSTATUS request. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    ULONG msr = 0;

    const ULONG usr1 = READ_REGISTER_NOFENCE_ULONG(
            &DeviceContextPtr->RegistersPtr->Usr1);

    //
    // Check the RTS Delta bit (aka. CTS delta bit), and clear bit by writing 1
    //
    if ((usr1 & IMX_UART_USR1_RTSD) != 0) {
        msr |= SERIAL_MSR_DCTS;

        WRITE_REGISTER_NOFENCE_ULONG(
            &DeviceContextPtr->RegistersPtr->Usr1,
            IMX_UART_USR1_RTSD);
    }

    //
    // Read current RTS (aka. CTS) status
    //
    if ((usr1 & IMX_UART_USR1_RTSS) != 0) {
        msr |= SERIAL_MSR_CTS;
    }

    *outputBufferPtr = msr;
    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
void
IMXUartIoctlSetClrRts (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest,
    bool RtsState
    )
{
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            DeviceContextPtr->InterruptContextPtr;

    WdfInterruptAcquireLock(interruptContextPtr->WdfInterrupt);

    //
    // Only allowed when CTSC is clear
    //
    if ((interruptContextPtr->Ucr2Copy & IMX_UART_UCR2_CTSC) != 0) {
        WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
        IMX_UART_LOG_ERROR(
            "Attempted to set state of RTS pin when manual RTS control is not enabled. "
            "(interruptContextPtr->Ucr2Copy = 0x%lx)",
            interruptContextPtr->Ucr2Copy);

        WdfRequestComplete(WdfRequest, STATUS_INVALID_PARAMETER);
        return;
    }

    if (RtsState) {
        interruptContextPtr->Ucr2Copy |= IMX_UART_UCR2_CTS;
    } else {
        interruptContextPtr->Ucr2Copy &= ~IMX_UART_UCR2_CTS;
    }

    WRITE_REGISTER_NOFENCE_ULONG(
        &interruptContextPtr->RegistersPtr->Ucr2,
        interruptContextPtr->Ucr2Copy);

    WdfInterruptReleaseLock(interruptContextPtr->WdfInterrupt);
    WdfRequestComplete(WdfRequest, STATUS_SUCCESS);
}

_Use_decl_annotations_
void
IMXUartIoctlGetDtrRts (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    ULONG* outputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to retrieve output buffer for IOCTL_SERIAL_GET_DTRRTS request. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    const IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            DeviceContextPtr->InterruptContextPtr;

    ULONG mask = 0;

    if ((interruptContextPtr->Ucr2Copy & IMX_UART_UCR2_CTS) != 0) {
        mask |= SERIAL_RTS_STATE;
    }

    *outputBufferPtr = mask;
    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
void
IMXUartIoctlSetBreakOnOff (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest,
    bool BreakOn
    )
{
    IMXUartSetBreakOnOff(DeviceContextPtr, BreakOn);
    WdfRequestComplete(WdfRequest, STATUS_SUCCESS);
}

_Use_decl_annotations_
void
IMXUartIoctlGetModemControl (
    const IMX_UART_DEVICE_CONTEXT* DeviceContextPtr,
    WDFREQUEST WdfRequest
    )
{
    //
    // These masks define access the modem control register.
    // Define locally to avoid conflict with potential future public definition.
    //
    enum SERIAL_MCR : ULONG {

        //
        // This bit controls the data terminal ready (DTR) line.  When
        // this bit is set the line goes to logic 0 (which is then inverted
        // by normal hardware).  This is normally used to indicate that
        // the device is available to be used.  Some odd hardware
        // protocols (like the kernel debugger) use this for handshaking
        // purposes.
        //
        SERIAL_MCR_DTR = 0x01,

        //
        // This bit controls the ready to send (RTS) line.  When this bit
        // is set the line goes to logic 0 (which is then inverted by the normal
        // hardware).  This is used for hardware handshaking.  It indicates that
        // the hardware is ready to send data and it is waiting for the
        // receiving end to set clear to send (CTS).
        //
        SERIAL_MCR_RTS = 0x02,

        //
        // This bit is used for general purpose output.
        //
        SERIAL_MCR_OUT1 = 0x04,

        //
        // This bit is used for general purpose output.
        //
        SERIAL_MCR_OUT2 = 0x08,

        //
        // This bit controls the loopback testing mode of the device.  Basically
        // the outputs are connected to the inputs (and vice versa).
        //
        SERIAL_MCR_LOOP = 0x10,
    };

    ULONG* outputBufferPtr;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
            WdfRequest,
            sizeof(*outputBufferPtr),
            reinterpret_cast<PVOID*>(&outputBufferPtr),
            nullptr);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to retrieve output buffer for IOCTL_SERIAL_GET_MODEM_CONTROL request. (status = %!STATUS!)",
            status);

        WdfRequestComplete(WdfRequest, status);
        return;
    }

    const IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            DeviceContextPtr->InterruptContextPtr;

    ULONG mcr = 0;

    if ((interruptContextPtr->Ucr2Copy & IMX_UART_UCR2_CTS) != 0) {
        mcr |= SERIAL_MCR_RTS;
    }

    *outputBufferPtr = mcr;
    WdfRequestCompleteWithInformation(
        WdfRequest,
        STATUS_SUCCESS,
        sizeof(*outputBufferPtr));
}

_Use_decl_annotations_
NTSTATUS
IMXUartUpdateDmaSettings (
    IMX_UART_DEVICE_CONTEXT* DeviceContextPtr
    )
{
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        DeviceContextPtr->InterruptContextPtr;

    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        interruptContextPtr->RxDmaTransactionContextPtr;

    //
    // DMA is not used, nothing left to do here...
    //
    if (rxDmaTransactionContextPtr == nullptr) {
        return STATUS_SUCCESS;
    }

    //
    // If baud rate has not changed, nothing left to do here...
    //
    if (rxDmaTransactionContextPtr->CurrentBaudRate ==
        DeviceContextPtr->CurrentBaudRate) {

        return STATUS_SUCCESS;
    }

    IMX_UART_LOG_TRACE("RX DMA: Updating RX DMA settings due to baud rate change");

    //
    // Update the information required for the progress timer interval.
    //
    rxDmaTransactionContextPtr->CurrentBaudRate =
        DeviceContextPtr->CurrentBaudRate;

    const ULONG bitsPerFrame8N1 = 10;
    rxDmaTransactionContextPtr->CurrentFrameTimeUsec =
        bitsPerFrame8N1 * 1000000UL / DeviceContextPtr->CurrentBaudRate;

    //
    // Most important we need to update the RX DMA watermark level,
    // which can only be set before the transactions starts.
    // Thus we need to stop the RX DMA if it is currently running.
    //
    WdfSpinLockAcquire(rxDmaTransactionContextPtr->Lock);
    if (!interruptContextPtr->IsRxDmaStarted) {
        IMX_UART_LOG_TRACE("RX DMA: RX DMA settings updated inline");
        WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);
        return STATUS_SUCCESS;
    }
    WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);

    return IMXUartStopRxDma(interruptContextPtr);
}

_Use_decl_annotations_
NTSTATUS
IMXUartStartRxDma (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    )
{
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        InterruptContextPtr->RxDmaTransactionContextPtr;

    //
    // If DMA is not used, nothing to do here...
    //
    if (rxDmaTransactionContextPtr == nullptr) {
        return STATUS_SUCCESS;
    }

    //
    // If DMA has already been started, nothing to do here...
    //
    WdfSpinLockAcquire(rxDmaTransactionContextPtr->Lock);
    if (InterruptContextPtr->IsRxDmaStarted) {
        WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);
        return STATUS_SUCCESS;
    }

    //
    // Initialize and start the DMA transaction.
    // Transfer is paused until enabled when the SerCX2 'Custom Receive'
    // transaction is started...
    //

    WDFDMATRANSACTION rxWdfDmaTransaction =
        rxDmaTransactionContextPtr->WdfDmaTransaction;

    NTSTATUS status = WdfDmaTransactionInitialize(
        rxWdfDmaTransaction,
        IMXUartEvtWdfProgramRxDma,
        WdfDmaDirectionReadFromDevice,
        rxDmaTransactionContextPtr->DmaBufferMdlPtr,
        rxDmaTransactionContextPtr->DmaBufferPtr,
        rxDmaTransactionContextPtr->DmaBufferSize);

    if (!NT_SUCCESS(status)) {
        WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);
        IMX_UART_LOG_ERROR(
            "WdfDmaTransactionInitialize(...) failed. "
            "(status = %!STATUS!)",
            status);

        return status;
    }

    //
    //  Start the DMA transaction synchronously!
    //
    WdfDmaTransactionSetImmediateExecution(rxWdfDmaTransaction, TRUE);
    status = WdfDmaTransactionExecute(
        rxWdfDmaTransaction,
        static_cast<WDFCONTEXT>(rxDmaTransactionContextPtr));

    if (!NT_SUCCESS(status)) {
        InterruptContextPtr->RxDmaState = IMX_UART_STATE::STOPPED;
        WdfDmaTransactionRelease(rxWdfDmaTransaction);
        WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);
        IMX_UART_LOG_ERROR(
            "WdfDmaTransactionExecute Failed! (status = %!STATUS!)",
            status);

        return status;
    }

    InterruptContextPtr->IsRxDmaStarted = true;
    WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
IMXUartStopRxDma (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    )
{
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        InterruptContextPtr->RxDmaTransactionContextPtr;

    //
    // If DMA is not used, nothing to do here...
    //
    if (rxDmaTransactionContextPtr == nullptr) {
        return STATUS_SUCCESS;
    }

    //
    // If DMA already stopped, nothing to do here...
    //
    WdfSpinLockAcquire(rxDmaTransactionContextPtr->Lock);
    if (!InterruptContextPtr->IsRxDmaStarted) {
        NT_ASSERT(InterruptContextPtr->RxDmaState == IMX_UART_STATE::STOPPED);
        WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);
        return STATUS_SUCCESS;
    }
    InterruptContextPtr->IsRxDmaStarted = false;

    WdfTimerStop(rxDmaTransactionContextPtr->WdfProgressTimer, FALSE);

    //
    // Stop RX DMA
    //

    IMX_UART_REGISTERS* registersPtr = InterruptContextPtr->RegistersPtr;

    WdfInterruptAcquireLock(InterruptContextPtr->WdfInterrupt);
    InterruptContextPtr->Ucr1Copy &=
        ~(IMX_UART_UCR1_RXDMAEN | IMX_UART_UCR1_ATDMAEN);

    WRITE_REGISTER_NOFENCE_ULONG(
        &registersPtr->Ucr1,
        InterruptContextPtr->Ucr1Copy);

    InterruptContextPtr->RxDmaState = IMX_UART_STATE::STOPPED;
    WdfInterruptReleaseLock(InterruptContextPtr->WdfInterrupt);

    //
    // Stop the RX DMA transaction
    //

    NTSTATUS status;
    WDFDMATRANSACTION wdfDmaTransaction =
        rxDmaTransactionContextPtr->WdfDmaTransaction;

    //
    // Check if we are stopping before map registers were allocated.
    // In this case we release the transaction locally since
    // DMA callbacks will not be called for it.
    //
    if (WdfDmaTransactionCancel(wdfDmaTransaction)) {
        IMX_UART_LOG_TRACE(
            "RX DMA: Transaction canceled before map registers allocation");

    } else {
        IMX_UART_LOG_TRACE("RX DMA: transaction canceled during transfer");
        WdfDmaTransactionStopSystemTransfer(wdfDmaTransaction);
        WdfDmaTransactionDmaCompletedFinal(
            rxDmaTransactionContextPtr->WdfDmaTransaction,
            rxDmaTransactionContextPtr->BytesTransferred,
            &status);
    }

    status = WdfDmaTransactionRelease(wdfDmaTransaction);
    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_WARNING(
            "RX DMA: WdfDmaTransactionRelease failed. (status = %!STATUS!",
            status);
    }

    rxDmaTransactionContextPtr->DmaBufferPendingBytes = 0;
    WdfSpinLockRelease(rxDmaTransactionContextPtr->Lock);
    return status;
}

_Use_decl_annotations_
NTSTATUS
IMXUartAcquireDmaRequestLineOwnership (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    )
{
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        InterruptContextPtr->RxDmaTransactionContextPtr;

    PDMA_ADAPTER rxDmaAdapterPtr = nullptr;
    if (rxDmaTransactionContextPtr != nullptr) {
        //
        // Acquire RX DMA request line
        //
        rxDmaAdapterPtr = rxDmaTransactionContextPtr->DmaAdapterPtr;
        NTSTATUS status = rxDmaAdapterPtr->DmaOperations->ConfigureAdapterChannel(
            rxDmaAdapterPtr,
            SDMA_CFG_FUN_ACQUIRE_REQUEST_LINE,
            &rxDmaTransactionContextPtr->DmaRequestLine);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "SDMA_CFG_FUN_ACQUIRE_REQUEST_LINE failed for RX line %lu. "
                "(status = %!STATUS!)",
                rxDmaTransactionContextPtr->DmaRequestLine,
                status);
            return status;
        }
    }

    IMX_UART_TX_DMA_TRANSACTION_CONTEXT* txDmaTransactionContextPtr =
        InterruptContextPtr->TxDmaTransactionContextPtr;

    if (txDmaTransactionContextPtr != nullptr) {
        //
        // Acquire TX DMA request line
        //
        PDMA_ADAPTER txDmaAdapterPtr = txDmaTransactionContextPtr->DmaAdapterPtr;
        NTSTATUS status = txDmaAdapterPtr->DmaOperations->ConfigureAdapterChannel(
            txDmaAdapterPtr,
            SDMA_CFG_FUN_ACQUIRE_REQUEST_LINE,
            &txDmaTransactionContextPtr->DmaRequestLine);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "SDMA_CFG_FUN_ACQUIRE_REQUEST_LINE failed for TX line %lu. "
                "(status = %!STATUS!)",
                txDmaTransactionContextPtr->DmaRequestLine,
                status);

            if (rxDmaAdapterPtr != nullptr) {
                rxDmaAdapterPtr->DmaOperations->ConfigureAdapterChannel(
                    rxDmaAdapterPtr,
                    SDMA_CFG_FUN_RELEASE_REQUEST_LINE,
                    &rxDmaTransactionContextPtr->DmaRequestLine);
            }
            return status;
        }
    }

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
IMXUartReleaseDmaRequestLineOwnership (
    IMX_UART_INTERRUPT_CONTEXT* InterruptContextPtr
    )
{
    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        InterruptContextPtr->RxDmaTransactionContextPtr;

    NTSTATUS rxStatus = STATUS_SUCCESS;
    if (rxDmaTransactionContextPtr != nullptr) {
        //
        // Release RX DMA request line
        //
        PDMA_ADAPTER rxDmaAdapterPtr = rxDmaTransactionContextPtr->DmaAdapterPtr;
        rxStatus = rxDmaAdapterPtr->DmaOperations->ConfigureAdapterChannel(
            rxDmaAdapterPtr,
            SDMA_CFG_FUN_RELEASE_REQUEST_LINE,
            &rxDmaTransactionContextPtr->DmaRequestLine);

        if (!NT_SUCCESS(rxStatus)) {
            IMX_UART_LOG_ERROR(
                "SDMA_CFG_FUN_RELEASE_REQUEST_LINE failed for RX line %lu. "
                "(status = %!STATUS!)",
                rxDmaTransactionContextPtr->DmaRequestLine,
                rxStatus);
        }

        NT_ASSERT(rxDmaTransactionContextPtr->DmaBufferPtr != nullptr);
        RtlZeroMemory(
            rxDmaTransactionContextPtr->DmaBufferPtr,
            rxDmaTransactionContextPtr->DmaBufferSize);
    }

    IMX_UART_TX_DMA_TRANSACTION_CONTEXT* txDmaTransactionContextPtr =
        InterruptContextPtr->TxDmaTransactionContextPtr;

    NTSTATUS txStatus = STATUS_SUCCESS;
    if (txDmaTransactionContextPtr != nullptr) {
        //
        // Release TX DMA request line
        //
        PDMA_ADAPTER txDmaAdapterPtr = txDmaTransactionContextPtr->DmaAdapterPtr;
        txStatus = txDmaAdapterPtr->DmaOperations->ConfigureAdapterChannel(
            txDmaAdapterPtr,
            SDMA_CFG_FUN_RELEASE_REQUEST_LINE,
            &txDmaTransactionContextPtr->DmaRequestLine);

        if (!NT_SUCCESS(txStatus)) {
            IMX_UART_LOG_ERROR(
                "SDMA_CFG_FUN_ACQUIRE_REQUEST_LINE failed for TX line %lu. "
                "(status = %!STATUS!)",
                txDmaTransactionContextPtr->DmaRequestLine,
                txStatus);
        }
    }

    if (!NT_SUCCESS(rxStatus)) {
        return rxStatus;
    }
    return txStatus;
}

IMX_UART_NONPAGED_SEGMENT_END; //================================================
IMX_UART_PAGED_SEGMENT_BEGIN; //=================================================

_Use_decl_annotations_
NTSTATUS
IMXUartEvtDevicePrepareHardware (
    WDFDEVICE WdfDevice,
    WDFCMRESLIST /*ResourcesRaw*/,
    WDFCMRESLIST ResourcesTranslated
    )
{
    PAGED_CODE();
    IMX_UART_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    const CM_PARTIAL_RESOURCE_DESCRIPTOR* memResourcePtr = nullptr;
    const CM_PARTIAL_RESOURCE_DESCRIPTOR* rxDmaResourcePtr = nullptr;
    const CM_PARTIAL_RESOURCE_DESCRIPTOR* txDmaResourcePtr = nullptr;
    ULONG interruptResourceCount = 0;
    ULONG dmaResourceCount = 0;
    LARGE_INTEGER selfConnectionId;
    selfConnectionId.QuadPart = 0;

    //
    // Look for single memory resource, single interrupt resource, and
    // optional UartSerialBus connection resource
    //
    const ULONG resourceCount = WdfCmResourceListGetCount(ResourcesTranslated);
    for (ULONG i = 0; i < resourceCount; ++i) {
        const CM_PARTIAL_RESOURCE_DESCRIPTOR* resourcePtr =
            WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

        switch (resourcePtr->Type) {
        case CmResourceTypeMemory:
            if (memResourcePtr != nullptr) {
                IMX_UART_LOG_WARNING(
                    "Received unexpected memory resource. (resourcePtr = 0x%p)",
                    resourcePtr);

                break;
            }

            memResourcePtr = resourcePtr;
            break;
        case CmResourceTypeInterrupt:
            switch (interruptResourceCount) {
            case 0: break;
            default:
                IMX_UART_LOG_WARNING(
                    "Received unexpected interrupt resource. "
                    "(interruptResourceCount = %lu, resourcePtr = 0x%p)",
                    interruptResourceCount,
                    resourcePtr);
            }

            ++interruptResourceCount;
            break;
        case CmResourceTypeDma:
            switch (dmaResourceCount) {
            case 0:
                rxDmaResourcePtr = resourcePtr;
                break;
            case 1:
                txDmaResourcePtr = resourcePtr;
                break;
            default:
                IMX_UART_LOG_WARNING(
                    "Received unexpected DMA resource. "
                    "(dmaResourceCount = %lu, resourcePtr = 0x%p)",
                    dmaResourceCount,
                    resourcePtr);
            }

            ++dmaResourceCount;
            break;

        case CmResourceTypeConnection:
            switch (resourcePtr->u.Connection.Class) {
            case CM_RESOURCE_CONNECTION_CLASS_SERIAL:
                switch (resourcePtr->u.Connection.Type) {
                case CM_RESOURCE_CONNECTION_TYPE_SERIAL_UART:
                    if (selfConnectionId.QuadPart != 0) {
                        IMX_UART_LOG_WARNING(
                            "Received unexpected UART connection resource. "
                            "(selfConnectionId = 0x%llx)",
                            selfConnectionId.QuadPart);

                        break;
                    }

                    selfConnectionId.LowPart =
                        resourcePtr->u.Connection.IdLowPart;

                    selfConnectionId.HighPart =
                        resourcePtr->u.Connection.IdHighPart;

                    NT_ASSERT(selfConnectionId.QuadPart != 0);
                    break; // CM_RESOURCE_CONNECTION_TYPE_SERIAL_UART
                } // switch (resourcePtr->u.Connection.Type)
                break; // CM_RESOURCE_CONNECTION_CLASS_SERIAL
            } // switch (resourcePtr->u.Connection.Class)
            break; // CmResourceTypeConnection
        } // switch (resourcePtr->Type)
    } // for (...)

    if ((memResourcePtr == nullptr) || (interruptResourceCount < 1)) {
        IMX_UART_LOG_ERROR(
            "Did not receive required memory resource and interrupt resource. "
            "(memResourcePtr = 0x%p, interruptResourceCount = %lu)",
            memResourcePtr,
            interruptResourceCount);

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if ((dmaResourceCount != 0) && (dmaResourceCount != 2)) {
        IMX_UART_LOG_ERROR(
            "Invalid DMA resource count %lu, driver expects either 0 or 2. ",
            dmaResourceCount);

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (memResourcePtr->u.Memory.Length < sizeof(IMX_UART_REGISTERS)) {
        IMX_UART_LOG_ERROR(
            "Memory resource is too small. "
            "(memResourcePtr->u.Memory.Length = %lu, "
            "sizeof(IMX_UART_REGISTERS) = %lu)",
            memResourcePtr->u.Memory.Length,
            sizeof(IMX_UART_REGISTERS));

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    //
    // ReleaseHardware is ALWAYS called, even if PrepareHardware fails, so
    // the cleanup of registersPtr is handled there.
    //
    IMX_UART_DEVICE_CONTEXT* deviceContextPtr =
            IMXUartGetDeviceContext(WdfDevice);

    NT_ASSERT(memResourcePtr->Type == CmResourceTypeMemory);
    deviceContextPtr->RegistersPtr = static_cast<IMX_UART_REGISTERS*>(
        MmMapIoSpaceEx(
            memResourcePtr->u.Memory.Start,
            sizeof(IMX_UART_REGISTERS),
            PAGE_READWRITE | PAGE_NOCACHE));

    if (deviceContextPtr->RegistersPtr == nullptr) {
        IMX_UART_LOG_LOW_MEMORY(
            "MmMapIoSpaceEx(...) failed. "
            "(memResourcePtr->u.Memory.Start = 0x%llx, "
            "sizeof(IMX_UART_REGISTERS) = %lu)",
            memResourcePtr->u.Memory.Start.QuadPart,
            sizeof(IMX_UART_REGISTERS));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
            deviceContextPtr->InterruptContextPtr;

    interruptContextPtr->RegistersPtr = deviceContextPtr->RegistersPtr;

    //
    // Allocate intermediate buffers
    //
    {
        NT_ASSERT(deviceContextPtr->TxBufferStorage == nullptr);
        deviceContextPtr->TxBufferStorage = static_cast<BYTE*>(
                ExAllocatePoolWithTag(
                    NonPagedPoolNx,
                    deviceContextPtr->Parameters.TxIntermediateBufferSize,
                    IMX_UART_POOL_TAG));

        if (deviceContextPtr->TxBufferStorage == nullptr) {
            IMX_UART_LOG_LOW_MEMORY(
                "Failed to allocate TxBufferStorage. (TxIntermediateBufferSize = %lu)",
                deviceContextPtr->Parameters.TxIntermediateBufferSize);

            return STATUS_NO_MEMORY;
        }

        interruptContextPtr->TxBuffer.SetBuffer(
            deviceContextPtr->TxBufferStorage,
            deviceContextPtr->Parameters.TxIntermediateBufferSize);

        NT_ASSERT(deviceContextPtr->RxBufferStorage == nullptr);
        deviceContextPtr->RxBufferStorage = static_cast<BYTE*>(
                ExAllocatePoolWithTag(
                    NonPagedPoolNx,
                    deviceContextPtr->Parameters.RxIntermediateBufferSize,
                    IMX_UART_POOL_TAG));

        if (deviceContextPtr->RxBufferStorage == nullptr) {
            IMX_UART_LOG_LOW_MEMORY(
                "Failed to allocate RxBufferStorage. (RxIntermediateBufferSize = %lu)",
                deviceContextPtr->Parameters.RxIntermediateBufferSize);

            return STATUS_NO_MEMORY;
        }

        interruptContextPtr->RxBuffer.SetBuffer(
            deviceContextPtr->RxBufferStorage,
            deviceContextPtr->Parameters.RxIntermediateBufferSize);
    }

    NTSTATUS status;

    if (dmaResourceCount != 0) {
        NT_ASSERT(rxDmaResourcePtr != nullptr);
        NT_ASSERT(txDmaResourcePtr != nullptr);

        status = IMXUartInitializeDma(
            WdfDevice,
            memResourcePtr,
            rxDmaResourcePtr,
            txDmaResourcePtr);

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    status =
        IMXUartResetHardwareAndClearShadowedRegisters(interruptContextPtr);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Create device interface if we received a UartSerialBus resource
    //
    if (selfConnectionId.QuadPart != 0) {
        RTL_OSVERSIONINFOW verInfo = {0};
        verInfo.dwOSVersionInfoSize = sizeof(verInfo);
        status = RtlGetVersion(&verInfo);
        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "RtlGetVersion(...) failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }

        //
        // Self-publishing is only necessary on OS versions prior to 19H1
        //
        if (verInfo.dwBuildNumber <= 17763) {
            status = IMXUartCreateDeviceInterface(WdfDevice, selfConnectionId);
            if (!NT_SUCCESS(status)) {
                IMX_UART_LOG_ERROR(
                    "IMXUartCreateDeviceInterface(...) failed. "
                    "(status = %!STATUS!, selfConnectionId = %llx)",
                    status,
                    selfConnectionId.QuadPart);

                return status;
            }
        } else {
            IMX_UART_LOG_INFORMATION(
                "Skipping self-publishing due to OS version. "
                "Use SerCx2 to publish a device interface. "
                "(BuildNumber = %d)",
                verInfo.dwBuildNumber);
        }
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
IMXUartEvtDeviceReleaseHardware (
    WDFDEVICE WdfDevice,
    WDFCMRESLIST /*ResourcesTranslated*/
    )
{
    PAGED_CODE();
    IMX_UART_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    IMX_UART_DEVICE_CONTEXT* deviceContextPtr =
            IMXUartGetDeviceContext(WdfDevice);

    deviceContextPtr->InterruptContextPtr->RxBuffer.SetBuffer(nullptr, 0);
    if (deviceContextPtr->RxBufferStorage != nullptr) {
        ExFreePoolWithTag(deviceContextPtr->RxBufferStorage, IMX_UART_POOL_TAG);
        deviceContextPtr->RxBufferStorage = nullptr;
    }

    deviceContextPtr->InterruptContextPtr->TxBuffer.SetBuffer(nullptr, 0);
    if (deviceContextPtr->TxBufferStorage != nullptr) {
        ExFreePoolWithTag(deviceContextPtr->TxBufferStorage, IMX_UART_POOL_TAG);
        deviceContextPtr->TxBufferStorage = nullptr;
    }

    if (deviceContextPtr->RegistersPtr != nullptr) {
        MmUnmapIoSpace(
            deviceContextPtr->RegistersPtr,
            sizeof(IMX_UART_REGISTERS));

        deviceContextPtr->RegistersPtr = nullptr;
    }

    IMXUartDeinitializeDma(WdfDevice);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
IMXUartEvtDeviceAdd (
    WDFDRIVER /*WdfDriver*/,
    WDFDEVICE_INIT* DeviceInitPtr
    )
{
    PAGED_CODE();
    IMX_UART_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    //
    // Set PNP and Power callbacks
    //
    {
        WDF_PNPPOWER_EVENT_CALLBACKS wdfPnppowerEventCallbacks;
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&wdfPnppowerEventCallbacks);
        wdfPnppowerEventCallbacks.EvtDevicePrepareHardware =
            IMXUartEvtDevicePrepareHardware;

        wdfPnppowerEventCallbacks.EvtDeviceReleaseHardware =
            IMXUartEvtDeviceReleaseHardware;

        WdfDeviceInitSetPnpPowerEventCallbacks(
            DeviceInitPtr,
            &wdfPnppowerEventCallbacks);

    } // PNP and Power

    //
    // Call SerCx2InitializeDeviceInit() to attach the SerCX2 to the
    // WDF pipeline.
    //
    NTSTATUS status = SerCx2InitializeDeviceInit(DeviceInitPtr);
    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "SerCx2InitializeDeviceInit(...) failed, (status = %!STATUS!)",
            status);

        return status;
    }

    //
    // Override the default security descriptor with one that allows
    // usermode read/write access.
    //
    DECLARE_CONST_UNICODE_STRING(
        SDDL_DEVOBJ_SERCX_SYS_ALL_ADM_ALL_UMDF_ALL_USERS_RDWR,
        L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;UD)(A;;GRGW;;;BU)");

    status = WdfDeviceInitAssignSDDLString(
            DeviceInitPtr,
            &SDDL_DEVOBJ_SERCX_SYS_ALL_ADM_ALL_UMDF_ALL_USERS_RDWR);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "WdfDeviceInitAssignSDDLString(...) failed. (status = %!STATUS!, SDDL_DEVOBJ_SERCX_SYS_ALL_ADM_ALL_UMDF_ALL_USERS_RDWR = %wZ)",
            status,
            &SDDL_DEVOBJ_SERCX_SYS_ALL_ADM_ALL_UMDF_ALL_USERS_RDWR);

        return status;
    }

    //
    // Create and initialize the WDF device.
    //
    WDFDEVICE wdfDevice;
    IMX_UART_DEVICE_CONTEXT* deviceContextPtr;
    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &attributes,
            IMX_UART_DEVICE_CONTEXT);

        status = WdfDeviceCreate(&DeviceInitPtr, &attributes, &wdfDevice);
        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "WdfDeviceCreate(...) failed. (status = %!STATUS!)",
                status);

            return status;
        }

        deviceContextPtr =
            new (IMXUartGetDeviceContext(wdfDevice)) IMX_UART_DEVICE_CONTEXT();

        deviceContextPtr->WdfDevice = wdfDevice;
    }

    //
    // Set power capabilities
    //
    {
        WDF_DEVICE_POWER_CAPABILITIES powerCaps;
        WDF_DEVICE_POWER_CAPABILITIES_INIT(&powerCaps);
        powerCaps.DeviceD1 = WdfFalse;
        powerCaps.DeviceD2 = WdfFalse;

        WdfDeviceSetPowerCapabilities(wdfDevice, &powerCaps);
    }

    //
    // Set PNP capabilities
    //
    {
        WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
        WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);

        pnpCaps.Removable = WdfTrue;
        pnpCaps.SurpriseRemovalOK = WdfTrue;

        WdfDeviceSetPnpCapabilities(wdfDevice, &pnpCaps);
    }

    //
    // Assign S0 idle policy
    //
    {
        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(
            &idleSettings,
            IdleCannotWakeFromS0);

        idleSettings.IdleTimeout = IdleTimeoutDefaultValue;
        idleSettings.UserControlOfIdleSettings = IdleAllowUserControl;
        idleSettings.Enabled = WdfTrue;
        idleSettings.PowerUpIdleDeviceOnSystemWake = WdfFalse;
        idleSettings.IdleTimeoutType = SystemManagedIdleTimeout;
        idleSettings.ExcludeD3Cold = WdfFalse;

        status = WdfDeviceAssignS0IdleSettings(wdfDevice, &idleSettings);
        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "WdfDeviceAssignS0IdleSettings(...) failed. (status = %!STATUS!)",
                status);

            return status;
        }
    }

    //
    // Create an interrupt object
    //
    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr;
    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &attributes,
            IMX_UART_INTERRUPT_CONTEXT);

        WDF_INTERRUPT_CONFIG interruptConfig;
        WDF_INTERRUPT_CONFIG_INIT(
            &interruptConfig,
            IMXUartEvtInterruptIsr,
            IMXUartEvtInterruptDpc);

        WDFINTERRUPT wdfInterrupt;
        status = WdfInterruptCreate(
                wdfDevice,
                &interruptConfig,
                &attributes,
                &wdfInterrupt);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "Failed to create interrupt object. (status = %!STATUS!)",
                status);

            return status;
        }

        interruptContextPtr = new (IMXUartGetInterruptContext(wdfInterrupt))
                IMX_UART_INTERRUPT_CONTEXT();

        interruptContextPtr->WdfInterrupt = wdfInterrupt;
        deviceContextPtr->WdfInterrupt = wdfInterrupt;
        deviceContextPtr->InterruptContextPtr = interruptContextPtr;
    }

    //
    // Initialize SerCx2 class extension.
    //
    {
        SERCX2_CONFIG serCx2Config;
        SERCX2_CONFIG_INIT(
            &serCx2Config,
            IMXUartEvtSerCx2ApplyConfig,
            IMXUartEvtSerCx2Control,
            IMXUartEvtSerCx2PurgeFifos);

        serCx2Config.EvtSerCx2SetWaitMask = IMXUartEvtSerCx2SetWaitMask;
        serCx2Config.EvtSerCx2FileOpen = IMXUartEvtSerCx2FileOpen;
        serCx2Config.EvtSerCx2FileClose = IMXUartEvtSerCx2FileClose;

        status = SerCx2InitializeDevice(wdfDevice, &serCx2Config);
        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "SerCx2InitializeDevice(...) failed. (status = %!STATUS!)",
                status);

            return status;
        }
    }

    //
    // Configure receive PIO contexts and callbacks
    //
    {
        SERCX2_PIO_RECEIVE_CONFIG serCx2PioReceiveConfig;
        SERCX2_PIO_RECEIVE_CONFIG_INIT(
            &serCx2PioReceiveConfig,
            IMXUartEvtSerCx2PioReceiveReadBuffer,
            IMXUartEvtSerCx2PioReceiveEnableReadyNotification,
            IMXUartEvtSerCx2PioReceiveCancelReadyNotification);

        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &attributes,
            IMX_UART_RX_CONTEXT);

        //
        // Create the SerCx2 RX object
        //
        SERCX2PIORECEIVE pioReceive;
        status = SerCx2PioReceiveCreate(
                wdfDevice,
                &serCx2PioReceiveConfig,
                &attributes,
                &pioReceive);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "SerCx2PioReceiveCreate(...) failed. (status = %!STATUS!)",
                status);

            return status;
        }

        auto rxContextPtr =
            new (IMXUartGetRxContext(pioReceive)) IMX_UART_RX_CONTEXT();

        rxContextPtr->InterruptContextPtr = interruptContextPtr;
        interruptContextPtr->SerCx2PioReceive = pioReceive;
    } // Configure receive PIO

    //
    // Configure transmit PIO contexts and callbacks
    //
    {

        SERCX2_PIO_TRANSMIT_CONFIG serCx2PioTransmitConfig;
        SERCX2_PIO_TRANSMIT_CONFIG_INIT(
            &serCx2PioTransmitConfig,
            IMXUartEvtSerCx2PioTransmitWriteBuffer,
            IMXUartEvtSerCx2PioTransmitEnableReadyNotification,
            IMXUartEvtSerCx2PioTransmitCancelReadyNotification);

        serCx2PioTransmitConfig.EvtSerCx2PioTransmitDrainFifo =
            IMXUartEvtSerCx2PioTransmitDrainFifo;

        serCx2PioTransmitConfig.EvtSerCx2PioTransmitCancelDrainFifo =
            IMXUartEvtSerCx2PioTransmitCancelDrainFifo;

        serCx2PioTransmitConfig.EvtSerCx2PioTransmitPurgeFifo =
            IMXUartEvtSerCx2PioTransmitPurgeFifo;

        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &attributes,
            IMX_UART_TX_CONTEXT);

        //
        // Create the SerCx2 TX object
        //
        SERCX2PIOTRANSMIT pioTransmit;
        status = SerCx2PioTransmitCreate(
                wdfDevice,
                &serCx2PioTransmitConfig,
                &attributes,
                &pioTransmit);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "SerCx2PioTransmitCreate(...) failed, (status = %!STATUS!)",
                status);

            return status;
        }

        auto txContextPtr =
            new (IMXUartGetTxContext(pioTransmit)) IMX_UART_TX_CONTEXT();

        txContextPtr->InterruptContextPtr = interruptContextPtr;
        interruptContextPtr->SerCx2PioTransmit = pioTransmit;
    }

    //
    // Query parameters from registry
    //
    {
        IMX_UART_WDFKEY wdfKey;
        status = WdfDeviceOpenRegistryKey(
                wdfDevice,
                PLUGPLAY_REGKEY_DEVICE,
                KEY_READ,
                WDF_NO_OBJECT_ATTRIBUTES,
                &wdfKey.Handle);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "WdfDeviceOpenRegistryKey(...) failed. (status = %!STATUS!)",
                status);

           return status;
        }

        const struct _REGVAL_DESCRIPTOR {
            PCWSTR ValueName;
            ULONG* DestinationPtr;
            ULONG DefaultValue;
        } regTable[] = {
            {
                L"RxIntermediateBufferSize",
                &deviceContextPtr->Parameters.RxIntermediateBufferSize,
                1024,
            },
            {
                L"RxDmaIntermediateBufferSize",
                &deviceContextPtr->Parameters.RxDmaIntermediateBufferSize,
                1024 * 8,
            },
            {
                L"TxIntermediateBufferSize",
                &deviceContextPtr->Parameters.TxIntermediateBufferSize,
                1024,
            },
            {
                L"TxFifoThresholdUs",
                &deviceContextPtr->Parameters.TxFifoThresholdUs,
                10,
            },
            {
                L"RxFifoThresholdUs",
                &deviceContextPtr->Parameters.RxFifoThresholdUs,
                50,
            },
            {
                L"TxDpcThresholdUs",
                &deviceContextPtr->Parameters.TxDpcThresholdUs,
                50,
            },
            {
                L"RtsTriggerLevel",
                &deviceContextPtr->Parameters.RtsTriggerLevel,
                30,
            },
            {
                L"ModuleClockFrequency",
                &deviceContextPtr->Parameters.ModuleClockFrequency,
                80000000, // 80 Mhz (UART_CLK_ROOT)
            },
            {
                L"RxDmaMinTransactionLength",
                &deviceContextPtr->Parameters.RxDmaMinTransactionLength,
                4 * IMX_UART_FIFO_COUNT,
            },
            {
                L"TxDmaMinTransactionLength",
                &deviceContextPtr->Parameters.TxDmaMinTransactionLength,
                4 * IMX_UART_FIFO_COUNT,
            }
        };

        for (ULONG i = 0; i < ARRAYSIZE(regTable); ++i) {
            const _REGVAL_DESCRIPTOR* descriptorPtr = &regTable[i];

            UNICODE_STRING valueName;
            status = RtlUnicodeStringInit(
                    &valueName,
                    descriptorPtr->ValueName);

            NT_ASSERT(NT_SUCCESS(status));

            status = WdfRegistryQueryULong(
                    wdfKey.Handle,
                    &valueName,
                    descriptorPtr->DestinationPtr);

            if (!NT_SUCCESS(status)) {
                IMX_UART_LOG_WARNING(
                    "Failed to query registry value, using default value. "
                    "(status = %!STATUS!, "
                    "descriptorPtr->ValueName = %S, "
                    "descriptorPtr->DefaultValue = %lu)",
                    status,
                    descriptorPtr->ValueName,
                    descriptorPtr->DefaultValue);

                *descriptorPtr->DestinationPtr = descriptorPtr->DefaultValue;
            }
        }
    } // Close registry handle

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
VOID IMXUartEvtDriverUnload (WDFDRIVER WdfDriver)
{
    PAGED_CODE();

    DRIVER_OBJECT* driverObjectPtr = WdfDriverWdmGetDriverObject(WdfDriver);
    WPP_CLEANUP(driverObjectPtr);
}

//
// Routine Description:
//
//  Creates a device interface, using the supplied resource hub connection ID
//  as a reference string. When SerCx2 receives a FileCreate request, it
//  looks for the connection ID in the filename, and queries the resource
//  hub to determine initial connection settings.
//
//  The _DDN ACPI method is queried to determine the device friendly name
//  as seen by WinRT. If the _DDN method is not present, no friendly name
//  will be assigned.
//
//  In 19H1, SerCx2 added support to publish a device interface. When this
//  driver no longer needs to support RS5 (17763), this function can be deleted.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object
//
//  ConnectionId - A resource hub Connection Id to a UartSerialBus connection.
//
// Return Value:
//
//  STATUS_SUCCESS - On success
//
_Use_decl_annotations_
NTSTATUS
IMXUartCreateDeviceInterface (
    WDFDEVICE WdfDevice,
    LARGE_INTEGER ConnectionId
    )
{
    PAGED_CODE();

    NT_ASSERT(ConnectionId.QuadPart != 0LL);

    DECLARE_UNICODE_STRING_SIZE(
        referenceString,
        RESOURCE_HUB_CONNECTION_FILE_SIZE);

    NTSTATUS status = RESOURCE_HUB_ID_TO_FILE_NAME(
            ConnectionId.LowPart,
            ConnectionId.HighPart,
            referenceString.Buffer);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "RESOURCE_HUB_ID_TO_FILE_NAME() failed. (status = %!STATUS!)",
            status);

        return status;
    }

    referenceString.Length = RESOURCE_HUB_CONNECTION_FILE_SIZE - sizeof(WCHAR);

    status = WdfDeviceCreateDeviceInterface(
            WdfDevice,
            &GUID_DEVINTERFACE_COMPORT,
            &referenceString);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "WdfDeviceCreateDeviceInterface(...) failed. (status = %!STATUS!, "
            "GUID_DEVINTERFACE_COMPORT = %!GUID!, referenceString = %wZ)",
            status,
            &GUID_DEVINTERFACE_COMPORT,
            &referenceString);

        return status;
    }

    struct _SYMLINK_NAME {
        WDFSTRING WdfString = WDF_NO_HANDLE;
        ~_SYMLINK_NAME ()
        {
            PAGED_CODE();
            if (this->WdfString == WDF_NO_HANDLE) return;
            WdfObjectDelete(this->WdfString);
        }
    } symlinkName;
    status = WdfStringCreate(
            nullptr,
            WDF_NO_OBJECT_ATTRIBUTES,
            &symlinkName.WdfString);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "WdfStringCreate(...) failed. (status = %!STATUS!)",
            status);

        return status;
    }

    status = WdfDeviceRetrieveDeviceInterfaceString(
            WdfDevice,
            &GUID_DEVINTERFACE_COMPORT,
            &referenceString,
            symlinkName.WdfString);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "WdfDeviceRetrieveDeviceInterfaceString() failed. "
            "(status = %!STATUS!, GUID_DEVINTERFACE_COMPORT = %!GUID!)",
            status,
            &GUID_DEVINTERFACE_COMPORT);

        return status;
    }

    UNICODE_STRING symlinkNameWsz;
    WdfStringGetUnicodeString(symlinkName.WdfString, &symlinkNameWsz);

    //
    // IsRestricted interface property works in conjunction with the
    // RestrictedOverrideForSystemContainerAllowed flag set on the interface
    // class to allow access to internal devices (belonging to system
    // container) through the device broker. Setting it to FALSE is required
    // to explicitly grant access.
    //

    DEVPROP_BOOLEAN isRestricted = DEVPROP_FALSE;
    status = IoSetDeviceInterfacePropertyData(
            &symlinkNameWsz,
            &DEVPKEY_DeviceInterface_Restricted,
            0,
            0, // Flags
            DEVPROP_TYPE_BOOLEAN,
            sizeof(isRestricted),
            &isRestricted);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "IoSetDeviceInterfacePropertyData(...DEVPKEY_DeviceInterface_Restricted...) failed. "
            "(status = %!STATUS!, symlinkNameWsz = %wZ)",
            status,
            &symlinkNameWsz);

        return status;
    }

    //
    // Get DosDeviceName from registry
    //
    IMX_UART_WDFKEY parametersKey;
    status = WdfDeviceOpenRegistryKey(
            WdfDevice,
            PLUGPLAY_REGKEY_DEVICE,
            KEY_QUERY_VALUE,
            WDF_NO_OBJECT_ATTRIBUTES,
            &parametersKey.Handle);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to open device parameters registry key. (status = %!STATUS!)",
            status);

        return status;
    }

    DECLARE_CONST_UNICODE_STRING(dosDeviceNameRegvalStr, L"DosDeviceName");
    DECLARE_UNICODE_STRING_SIZE(portName, 64);
    status = WdfRegistryQueryUnicodeString(
            parametersKey.Handle,
            &dosDeviceNameRegvalStr,
            nullptr, // ValueByteLength
            &portName);

    if (NT_SUCCESS(status) &&
       ((portName.Length + sizeof(WCHAR)) < portName.MaximumLength)) {

        // Null-terminate PortName
        portName.Buffer[portName.Length / sizeof(WCHAR)] = UNICODE_NULL;

        // Set the port friendly name
        status = IoSetDeviceInterfacePropertyData(
                &symlinkNameWsz,
                &DEVPKEY_DeviceInterface_Serial_PortName,
                LOCALE_NEUTRAL,
                0, // Flags
                DEVPROP_TYPE_STRING,
                portName.Length + sizeof(WCHAR),
                portName.Buffer);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "IoSetDeviceInterfacePropertyData(...DEVPKEY_DeviceInterface_Serial_PortName...) failed. "
                "(status = %!STATUS!, symlinkNameWsz = %wZ, portName = %wZ)",
                status,
                &symlinkNameWsz,
                &portName);

            return status;
        }

        IMX_UART_LOG_INFORMATION(
            "Successfully assigned PortName to device interface. "
            "(symlinkNameWsz = %wZ, portName = %wZ)",
            &symlinkNameWsz,
            &portName);

    } else {
        IMX_UART_LOG_WARNING(
            "Failed to query DosDeviceName from registry. Skipping assignment "
            "of PortName. (status = %!STATUS!, dosDeviceNameRegvalStr = %wZ, "
            "portName.Length = %d, portName.MaximumLength = %d)",
            status,
            &dosDeviceNameRegvalStr,
            portName.Length,
            portName.MaximumLength);
    }

    IMX_UART_LOG_INFORMATION(
        "Successfully created device interface. (symlinkNameWsz = %wZ)",
        &symlinkNameWsz);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
IMXUartParseConnectionDescriptor (
    const RH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER* RhBufferPtr,
    ULONG* BaudRatePtr,
    SERIAL_LINE_CONTROL* LineControlPtr,
    SERIAL_HANDFLOW* HandflowPtr,
    bool* RtsCtsEnabledPtr
    )
{
    PAGED_CODE();
    IMX_UART_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    //
    // See section 6.4.3.8.2 of the ACPI 5.0 specification
    //
    enum PNP_SERIAL_BUS_TYPE {
        PNP_SERIAL_BUS_TYPE_I2C = 0x1,
        PNP_SERIAL_BUS_TYPE_SPI = 0x2,
        PNP_SERIAL_BUS_TYPE_UART = 0x3,
    };

    //
    // ACPI-defined values for the TypeSpecificFlags field of
    // PNP_UART_SERIAL_BUS_DESCRIPTOR
    //
    enum UART_SERIAL_FLAG : ULONG {
        UART_SERIAL_FLAG_FLOW_CTL_NONE =    (0x0000 << 0),
        UART_SERIAL_FLAG_FLOW_CTL_HW =      (0x0001 << 0),
        UART_SERIAL_FLAG_FLOW_CTL_XONXOFF = (0x0002 << 0),
        UART_SERIAL_FLAG_FLOW_CTL_MASK =    (0x0003 << 0),
        UART_SERIAL_FLAG_STOP_BITS_0 =      (0x0000 << 2),
        UART_SERIAL_FLAG_STOP_BITS_1 =      (0x0001 << 2),
        UART_SERIAL_FLAG_STOP_BITS_1_5 =    (0x0002 << 2),
        UART_SERIAL_FLAG_STOP_BITS_2 =      (0x0003 << 2),
        UART_SERIAL_FLAG_STOP_BITS_MASK =   (0x0003 << 2),
        UART_SERIAL_FLAG_DATA_BITS_5 =      (0x0000 << 4),
        UART_SERIAL_FLAG_DATA_BITS_6 =      (0x0001 << 4),
        UART_SERIAL_FLAG_DATA_BITS_7 =      (0x0002 << 4),
        UART_SERIAL_FLAG_DATA_BITS_8 =      (0x0003 << 4),
        UART_SERIAL_FLAG_DATA_BITS_9 =      (0x0004 << 4),
        UART_SERIAL_FLAG_DATA_BITS_MASK =   (0x0007 << 4),
        UART_SERIAL_FLAG_BIG_ENDIAN =       (0x0001 << 7),
    };

    //
    // ACPI-defined values for the Parity field of
    // PNP_UART_SERIAL_BUS_DESCRIPTOR
    //
    enum UART_SERIAL_PARITY : ULONG {
        UART_SERIAL_PARITY_NONE =            0x00,
        UART_SERIAL_PARITY_EVEN =            0x01,
        UART_SERIAL_PARITY_ODD =             0x02,
        UART_SERIAL_PARITY_MARK =            0x03,
        UART_SERIAL_PARITY_SPACE =           0x04,
    };

    //
    // ACPI-defined values for the SerialLinesEnabled field of
    // PNP_UART_SERIAL_BUS_DESCRIPTOR
    //
    enum UART_SERIAL_LINES : ULONG {
        UART_SERIAL_LINES_DCD =             (0x0001 << 2),
        UART_SERIAL_LINES_RI =              (0x0001 << 3),
        UART_SERIAL_LINES_DSR =             (0x0001 << 4),
        UART_SERIAL_LINES_DTR =             (0x0001 << 5),
        UART_SERIAL_LINES_CTS =             (0x0001 << 6),
        UART_SERIAL_LINES_RTS =             (0x0001 << 7),
    };

    #include <pshpack1.h>
    struct PNP_UART_SERIAL_BUS_DESCRIPTOR : PNP_SERIAL_BUS_DESCRIPTOR {
        ULONG BaudRate;
        USHORT RxBufferSize;
        USHORT TxBufferSize;
        UCHAR Parity;
        UCHAR SerialLinesEnabled;

        //
        // followed by optional Vendor Data and
        // PNP_IO_DESCRIPTOR_RESOURCE_NAME
        //
    };
    #include <poppack.h>

    //
    // Get ACPI descriptor
    //
    const PNP_UART_SERIAL_BUS_DESCRIPTOR* uartDescriptorPtr;
    {
        if (RhBufferPtr->PropertiesLength < sizeof(*uartDescriptorPtr)) {
            IMX_UART_LOG_ERROR(
                "Connection properties is too small. (RhBufferPtr->PropertiesLength = %lu, sizeof(*uartDescriptorPtr) = %lu)",
                RhBufferPtr->PropertiesLength,
                sizeof(*uartDescriptorPtr));

            return STATUS_INVALID_PARAMETER;
        }

        uartDescriptorPtr =
            reinterpret_cast<const PNP_UART_SERIAL_BUS_DESCRIPTOR*>(
                &RhBufferPtr->ConnectionProperties);

        if (uartDescriptorPtr->SerialBusType != PNP_SERIAL_BUS_TYPE_UART) {
            IMX_UART_LOG_ERROR(
                "ACPI Connnection descriptor is not a UART connection descriptor. (uartDescriptorPtr->SerialBusType = 0x%lx, PNP_SERIAL_BUS_TYPE_UART = 0x%lx)",
                uartDescriptorPtr->SerialBusType,
                PNP_SERIAL_BUS_TYPE_UART);

            return STATUS_INVALID_PARAMETER;
        }
    }

    RtlZeroMemory(LineControlPtr, sizeof(*LineControlPtr));
    RtlZeroMemory(HandflowPtr, sizeof(*HandflowPtr));

    const ULONG typeSpecificFlags = uartDescriptorPtr->TypeSpecificFlags;

    //
    // Validate flow control
    //
    switch (typeSpecificFlags & UART_SERIAL_FLAG_FLOW_CTL_MASK) {
    case UART_SERIAL_FLAG_FLOW_CTL_NONE:
        HandflowPtr->ControlHandShake &= ~SERIAL_OUT_HANDSHAKEMASK;
        HandflowPtr->FlowReplace &= ~SERIAL_RTS_MASK;
        HandflowPtr->FlowReplace |= SERIAL_RTS_CONTROL;
        break;

    case UART_SERIAL_FLAG_FLOW_CTL_HW:
        HandflowPtr->ControlHandShake &= ~SERIAL_OUT_HANDSHAKEMASK;
        HandflowPtr->ControlHandShake |= SERIAL_CTS_HANDSHAKE;
        HandflowPtr->FlowReplace &= ~SERIAL_RTS_MASK;
        HandflowPtr->FlowReplace |= SERIAL_RTS_HANDSHAKE;
        break;

    case UART_SERIAL_FLAG_FLOW_CTL_XONXOFF:
    default:

        IMX_UART_LOG_ERROR(
            "Unsupported flow control value. (typeSpecificFlags = 0x%lx)",
            typeSpecificFlags);

        return STATUS_NOT_SUPPORTED;
    }

    //
    // Validate stop bits
    //

    switch (typeSpecificFlags & UART_SERIAL_FLAG_STOP_BITS_MASK) {
    case UART_SERIAL_FLAG_STOP_BITS_1:
        LineControlPtr->StopBits = STOP_BIT_1;
        break;

    case UART_SERIAL_FLAG_STOP_BITS_1_5:
        LineControlPtr->StopBits = STOP_BITS_1_5;
        break;

    case UART_SERIAL_FLAG_STOP_BITS_2:
        LineControlPtr->StopBits = STOP_BITS_2;
        break;

    case UART_SERIAL_FLAG_STOP_BITS_0:
    default:

        IMX_UART_LOG_ERROR(
            "Unsupported serial stop bits. (typeSpecificFlags = 0x%lx)",
            typeSpecificFlags);

        return STATUS_NOT_SUPPORTED;
    }

    //
    // Validate data bits
    //
    switch (typeSpecificFlags & UART_SERIAL_FLAG_DATA_BITS_MASK) {
    case UART_SERIAL_FLAG_DATA_BITS_5:
        LineControlPtr->WordLength = 5;
        break;

    case UART_SERIAL_FLAG_DATA_BITS_6:
        LineControlPtr->WordLength = 6;
        break;

    case UART_SERIAL_FLAG_DATA_BITS_7:
        LineControlPtr->WordLength = 7;
        break;

    case UART_SERIAL_FLAG_DATA_BITS_8:
        LineControlPtr->WordLength = 8;
        break;

    case UART_SERIAL_FLAG_DATA_BITS_9:
        LineControlPtr->WordLength = 9;
        break;

    default:
        IMX_UART_LOG_ERROR(
            "Invalid data bits flag. (typeSpecificFlags = 0x%lx)",
            typeSpecificFlags);

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check big endian bit
    //
    if ((typeSpecificFlags & UART_SERIAL_FLAG_BIG_ENDIAN) != 0) {
        IMX_UART_LOG_ERROR(
            "Big endian mode is not supported. (typeSpecificFlags = 0x%lx)",
            typeSpecificFlags);

        return STATUS_NOT_SUPPORTED;
    }

    //
    // Validate parity
    //
    switch (uartDescriptorPtr->Parity) {
    case UART_SERIAL_PARITY_NONE:
        LineControlPtr->Parity = NO_PARITY;
        break;

    case UART_SERIAL_PARITY_EVEN:
        LineControlPtr->Parity = EVEN_PARITY;
        break;

    case UART_SERIAL_PARITY_ODD:
        LineControlPtr->Parity = ODD_PARITY;
        break;

    case UART_SERIAL_PARITY_MARK:
        LineControlPtr->Parity = MARK_PARITY;
        break;

    case UART_SERIAL_PARITY_SPACE:
        LineControlPtr->Parity = SPACE_PARITY;
        break;

    default:
        IMX_UART_LOG_ERROR(
            "Invalid parity. (Parity = %d)",
            uartDescriptorPtr->Parity);

        return STATUS_INVALID_PARAMETER;
    }

    //
    // We support RTS/CTS only
    //
    switch (uartDescriptorPtr->SerialLinesEnabled) {
    case 0:
        *RtsCtsEnabledPtr = false;
        break;

    case UART_SERIAL_LINES_CTS | UART_SERIAL_LINES_RTS:
        //
        // The presence of these flags is how we know whether or not to allow
        // RTS/CTS handshaking to be enabled in SetHandflow.
        //
        *RtsCtsEnabledPtr = true;
        break;

    default:
        IMX_UART_LOG_ERROR(
            "SerialLinesEnabled not supported. (SerialLinesEnabled = 0x%lx)",
            uartDescriptorPtr->SerialLinesEnabled);

        return STATUS_NOT_SUPPORTED;
    }

    *BaudRatePtr = uartDescriptorPtr->BaudRate;
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
IMXUartInitializeDma (
    WDFDEVICE WdfDevice,
    const CM_PARTIAL_RESOURCE_DESCRIPTOR* RegistersResourcePtr,
    const CM_PARTIAL_RESOURCE_DESCRIPTOR* RxDmaResourcePtr,
    const CM_PARTIAL_RESOURCE_DESCRIPTOR* TxDmaResourcePtr
    )
{
    PAGED_CODE();
    IMX_UART_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    IMX_UART_DEVICE_CONTEXT* deviceContextPtr =
        IMXUartGetDeviceContext(WdfDevice);

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        deviceContextPtr->InterruptContextPtr;

    NTSTATUS status;

    //
    // Configure receive DMA contexts and callbacks.
    //
    // For receive DMA we use SerCx2 custom receive which is implemented
    // using the WDF system DMA profile.
    // We cannot use SerCx2 system DMA directly since it does not support
    // auto-initialize (cyclic) transactions.
    //
    if (deviceContextPtr->Parameters.RxDmaMinTransactionLength == 0) {
        IMX_UART_LOG_WARNING("RX DMA is disabled");
    } else {
        SERCX2_CUSTOM_RECEIVE_CONFIG serCx2CustomRxConfig;
        SERCX2_CUSTOM_RECEIVE_CONFIG_INIT(&serCx2CustomRxConfig);

        serCx2CustomRxConfig.Exclusive = FALSE; // PIO/custom can be combined
        serCx2CustomRxConfig.MaximumTransactionLength = MAXULONG;
        serCx2CustomRxConfig.MinimumTransactionLength =
            deviceContextPtr->Parameters.RxDmaMinTransactionLength;;

        //
        // Create the SerCx2 Custom Receive object
        //
        SERCX2CUSTOMRECEIVE serCx2CustomRx;
        status = SerCx2CustomReceiveCreate(
            WdfDevice,
            &serCx2CustomRxConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &serCx2CustomRx);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "SerCx2CustomReceiveCreate(...) failed. (status = %!STATUS!)",
                status);

            return status;
        }

        //
        // Create the custom receive transaction
        //
        SERCX2_CUSTOM_RECEIVE_TRANSACTION_CONFIG serCx2CustomRxTransactionConfig;
        SERCX2_CUSTOM_RECEIVE_TRANSACTION_CONFIG_INIT(
            &serCx2CustomRxTransactionConfig,
            IMXUartEvtSerCx2CustomReceiveTransactionStart,
            nullptr,
            IMXUartEvtSerCx2CustomReceiveTransactionQueryProgress);

        serCx2CustomRxTransactionConfig.EvtSerCx2CustomReceiveTransactionCleanup =
            IMXUartEvtSerCx2CustomReceiveTransactionCleanup;

        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &attributes,
            IMX_UART_RX_DMA_TRANSACTION_CONTEXT);

        SERCX2CUSTOMRECEIVETRANSACTION serCx2CustomRxTransaction;
        status = SerCx2CustomReceiveTransactionCreate(
            serCx2CustomRx,
            &serCx2CustomRxTransactionConfig,
            &attributes,
            &serCx2CustomRxTransaction);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "SerCx2CustomReceiveTransactionCreate(...) failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }

        auto dmaTransactionContextPtr =
            new (IMXUartGetRxDmaTransactionContext(
                 serCx2CustomRxTransaction))
            IMX_UART_RX_DMA_TRANSACTION_CONTEXT();

        dmaTransactionContextPtr->SerCx2CustomRx = serCx2CustomRx;
        dmaTransactionContextPtr->SerCx2CustomRxTransaction =
            serCx2CustomRxTransaction;
        interruptContextPtr->RxDmaTransactionContextPtr =
            dmaTransactionContextPtr;

        //
        // Create a WDF system DMA object and transaction that will be used
        // by our custom RX transaction.
        //

        WDF_DMA_ENABLER_CONFIG wdfDmaEnablerConfig;
        WDF_DMA_ENABLER_CONFIG_INIT(
            &wdfDmaEnablerConfig,
            WdfDmaProfileSystem,
            SDMA_MAX_TRANSFER_LENGTH);

        wdfDmaEnablerConfig.WdmDmaVersionOverride = DEVICE_DESCRIPTION_VERSION3;

        WDFDMAENABLER wdfDmaEnabler;
        status = WdfDmaEnablerCreate(
            WdfDevice,
            &wdfDmaEnablerConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &wdfDmaEnabler);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "WdfDmaEnablerCreate(...) failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }

        //
        // Configure the system DMA for:
        // - Demand mode
        // - Auto initialize
        //

        PHYSICAL_ADDRESS uartRxdPA;
        uartRxdPA.QuadPart = RegistersResourcePtr->u.Memory.Start.QuadPart +
            FIELD_OFFSET(IMX_UART_REGISTERS, Rxd);

        WDF_DMA_SYSTEM_PROFILE_CONFIG wdfDmaSystemProfileConfig;
        WDF_DMA_SYSTEM_PROFILE_CONFIG_INIT(
            &wdfDmaSystemProfileConfig,
            uartRxdPA,
            Width8Bits,
            (PCM_PARTIAL_RESOURCE_DESCRIPTOR)RxDmaResourcePtr);

        wdfDmaSystemProfileConfig.DemandMode = TRUE;
        wdfDmaSystemProfileConfig.LoopedTransfer = TRUE;

        status = WdfDmaEnablerConfigureSystemProfile(
            wdfDmaEnabler,
            &wdfDmaSystemProfileConfig,
            WdfDmaDirectionReadFromDevice);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "WdfDmaEnablerCreate(...) failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &attributes,
            IMX_UART_RX_DMA_TRANSACTION_CONTEXT);

        WDFDMATRANSACTION wdfDmaTransaction;
        status = WdfDmaTransactionCreate(
            wdfDmaEnabler,
            WDF_NO_OBJECT_ATTRIBUTES,
            &wdfDmaTransaction);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "DMA RX: WdfDmaEnablerCreate(...) failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }

        dmaTransactionContextPtr->WdfDevice = WdfDevice;
        dmaTransactionContextPtr->InterruptContextPtr = interruptContextPtr;
        dmaTransactionContextPtr->WdfDmaEnabler = wdfDmaEnabler;
        dmaTransactionContextPtr->WdfDmaTransaction = wdfDmaTransaction;
        dmaTransactionContextPtr->DmaAdapterPtr =
            WdfDmaEnablerWdmGetDmaAdapter(
                wdfDmaEnabler,
                WdfDmaDirectionReadFromDevice);

        dmaTransactionContextPtr->DmaRequestLine =
            RxDmaResourcePtr->u.DmaV3.RequestLine;

        NT_ASSERT(deviceContextPtr->Parameters.RxDmaIntermediateBufferSize >=
            IMX_UART_RX_DMA_MIN_BUFFER_SIZE);

        dmaTransactionContextPtr->DmaBufferSize = max(
            deviceContextPtr->Parameters.RxDmaIntermediateBufferSize,
            IMX_UART_RX_DMA_MIN_BUFFER_SIZE);

        status = WdfSpinLockCreate(
            WDF_NO_OBJECT_ATTRIBUTES,
            &dmaTransactionContextPtr->Lock);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "WdfSpinLockCreate(...) for RX DMA lock failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }

        //
        // Allocate the DMA RX buffer and related resources.
        // Buffer is allocated as non paged/non-cached.
        //

        dmaTransactionContextPtr->DmaBufferPtr = static_cast<UCHAR*>(
            MmAllocateNonCachedMemory(
                dmaTransactionContextPtr->DmaBufferSize));

        if (dmaTransactionContextPtr->DmaBufferPtr == nullptr) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            IMX_UART_LOG_ERROR(
                "MmAllocateNonCachedMemory(...) failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }

        dmaTransactionContextPtr->DmaBufferMdlPtr = IoAllocateMdl(
            dmaTransactionContextPtr->DmaBufferPtr,
            static_cast<ULONG>(dmaTransactionContextPtr->DmaBufferSize),
            FALSE,
            FALSE,
            nullptr);

        if (dmaTransactionContextPtr->DmaBufferMdlPtr == nullptr) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            IMX_UART_LOG_ERROR(
                "IoAllocateMdl(...) failed. (status = %!STATUS!)",
                status);

            return status;
        }
        MmBuildMdlForNonPagedPool(dmaTransactionContextPtr->DmaBufferMdlPtr);

        WdfDmaTransactionSetChannelConfigurationCallback(
            wdfDmaTransaction,
            IMXUartEvtWdfRxDmaTransactionConfigureDmaChannel,
            dmaTransactionContextPtr);

        WdfDmaTransactionSetTransferCompleteCallback(
            wdfDmaTransaction,
            IMXUartEvtWdfRxDmaTransactionTransferComplete,
            dmaTransactionContextPtr);

        //
        // Since we are using a cyclic buffer we will need to
        // poll the transfer to figure out the transfer progress
        //

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &attributes,
            IMX_UART_RX_DMA_TIMER_CONTEXT);

        attributes.ParentObject = wdfDmaTransaction;

        WDF_TIMER_CONFIG wdfTimerConfig;
        WDF_TIMER_CONFIG_INIT(
            &wdfTimerConfig,
            IMXUartEvtTimerRxDmaProgress);

        wdfTimerConfig.TolerableDelay = 0;
        wdfTimerConfig.AutomaticSerialization = FALSE;

        status = WdfTimerCreate(
            &wdfTimerConfig,
            &attributes,
            &dmaTransactionContextPtr->WdfProgressTimer);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "WdfTimerCreate(...) failed. (status = %!STATUS!)",
                status);
            return status;
        }

        auto customRxTimerContextPtr =
            new (IMXUartGetRxDmaTimerContext(
                dmaTransactionContextPtr->WdfProgressTimer))
            IMX_UART_RX_DMA_TIMER_CONTEXT();

        customRxTimerContextPtr->RxDmaTransactionPtr = dmaTransactionContextPtr;
    } // Configure receive DMA

    //
    // Configure transmit DMA contexts and callbacks.
    //
    // For transmit DMA we use SerCx2 custom transmit which is implemented
    // using the WDF system DMA profile.
    // We cannot use SerCx2 system DMA directly since we are using custom
    // receive due to the SerCx2 receive auto-initialize (cyclic) limitation.
    //
    if (deviceContextPtr->Parameters.TxDmaMinTransactionLength == 0) {
        IMX_UART_LOG_WARNING("TX DMA is disabled");
    } else {
        SERCX2_CUSTOM_TRANSMIT_CONFIG serCx2CustomTxConfig;
        SERCX2_CUSTOM_TRANSMIT_CONFIG_INIT(&serCx2CustomTxConfig);

        serCx2CustomTxConfig.Exclusive = FALSE; // PIO/custom can be combined
        serCx2CustomTxConfig.MaximumTransactionLength = MAXULONG;
        serCx2CustomTxConfig.MinimumTransactionLength =
            deviceContextPtr->Parameters.TxDmaMinTransactionLength;

        //
        // Create the SerCx2 Custom Transmit object
        //
        SERCX2CUSTOMTRANSMIT serCx2CustomTx;
        status = SerCx2CustomTransmitCreate(
            WdfDevice,
            &serCx2CustomTxConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &serCx2CustomTx);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "SerCx2CustomReceiveCreate(...) failed. (status = %!STATUS!)",
                status);

            return status;
        }

        //
        // Create the custom transmit transaction
        //
        SERCX2_CUSTOM_TRANSMIT_TRANSACTION_CONFIG serCx2CustomTxTransactionConfig;
        SERCX2_CUSTOM_TRANSMIT_TRANSACTION_CONFIG_INIT(
            &serCx2CustomTxTransactionConfig,
            IMXUartEvtSerCx2CustomTransmitTransactionStart);

        serCx2CustomTxTransactionConfig.EvtSerCx2CustomTransmitTransactionCleanup =
            IMXUartEvtSerCx2CustomTransmitTransactionCleanup;

        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
            &attributes,
            IMX_UART_TX_DMA_TRANSACTION_CONTEXT);

        SERCX2CUSTOMTRANSMITTRANSACTION serCx2CustomTxTransaction;
        status = SerCx2CustomTransmitTransactionCreate(
            serCx2CustomTx,
            &serCx2CustomTxTransactionConfig,
            &attributes,
            &serCx2CustomTxTransaction);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "SerCx2CustomReceiveTransactionCreate(...) failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }

        auto dmaTransactionContextPtr =
            new (IMXUartGetTxDmaTransactionContext(
                serCx2CustomTxTransaction))
            IMX_UART_TX_DMA_TRANSACTION_CONTEXT();

        dmaTransactionContextPtr->InterruptContextPtr = interruptContextPtr;
        dmaTransactionContextPtr->SerCx2CustomTx = serCx2CustomTx;
        dmaTransactionContextPtr->SerCx2CustomTxTransaction =
            serCx2CustomTxTransaction;

        interruptContextPtr->TxDmaTransactionContextPtr =
            dmaTransactionContextPtr;

        //
        // Create a WDF system DMA object and transaction that will be used
        // by our custom TX transaction.
        //

        WDF_DMA_ENABLER_CONFIG wdfDmaEnablerCofig;
        WDF_DMA_ENABLER_CONFIG_INIT(
            &wdfDmaEnablerCofig,
            WdfDmaProfileSystem,
            SDMA_MAX_TRANSFER_LENGTH);

        wdfDmaEnablerCofig.WdmDmaVersionOverride = DEVICE_DESCRIPTION_VERSION3;

        WDFDMAENABLER wdfDmaEnabler;
        status = WdfDmaEnablerCreate(
            WdfDevice,
            &wdfDmaEnablerCofig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &wdfDmaEnabler);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "WdfDmaEnablerCreate(...) failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }

        //
        // Configure the system DMA for:
        // - Demand mode
        //

        PHYSICAL_ADDRESS uartTxdPA;
        uartTxdPA.QuadPart = RegistersResourcePtr->u.Memory.Start.QuadPart +
            FIELD_OFFSET(IMX_UART_REGISTERS, Txd);

        WDF_DMA_SYSTEM_PROFILE_CONFIG wdfDmaSystemProfileConfig;
        WDF_DMA_SYSTEM_PROFILE_CONFIG_INIT(
            &wdfDmaSystemProfileConfig,
            uartTxdPA,
            Width8Bits,
            (PCM_PARTIAL_RESOURCE_DESCRIPTOR)TxDmaResourcePtr);

        wdfDmaSystemProfileConfig.DemandMode = TRUE;

        status = WdfDmaEnablerConfigureSystemProfile(
            wdfDmaEnabler,
            &wdfDmaSystemProfileConfig,
            WdfDmaDirectionReadFromDevice);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "WdfDmaEnablerCreate(...) failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }

        WDFDMATRANSACTION wdfDmaTransaction;
        status = WdfDmaTransactionCreate(
            wdfDmaEnabler,
            WDF_NO_OBJECT_ATTRIBUTES,
            &wdfDmaTransaction);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "WdfDmaEnablerCreate(...) failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }

        dmaTransactionContextPtr->WdfDevice = WdfDevice;
        dmaTransactionContextPtr->InterruptContextPtr = interruptContextPtr;
        dmaTransactionContextPtr->WdfDmaEnabler = wdfDmaEnabler;
        dmaTransactionContextPtr->WdfDmaTransaction = wdfDmaTransaction;
        dmaTransactionContextPtr->DmaAdapterPtr =
            WdfDmaEnablerWdmGetDmaAdapter(
                wdfDmaEnabler,
                WdfDmaDirectionWriteToDevice);

        dmaTransactionContextPtr->DmaRequestLine =
            TxDmaResourcePtr->u.DmaV3.RequestLine;

        status = WdfSpinLockCreate(
            WDF_NO_OBJECT_ATTRIBUTES,
            &dmaTransactionContextPtr->Lock);

        if (!NT_SUCCESS(status)) {
            IMX_UART_LOG_ERROR(
                "WdfSpinLockCreate(...) for DMA TX lock failed. "
                "(status = %!STATUS!)",
                status);

            return status;
        }

        WdfDmaTransactionSetChannelConfigurationCallback(
            wdfDmaTransaction,
            IMXUartEvtWdfTxDmaTransactionConfigureDmaChannel,
            dmaTransactionContextPtr);

        WdfDmaTransactionSetTransferCompleteCallback(
            wdfDmaTransaction,
            IMXUartEvtWdfTxDmaTransactionTransferComplete,
            dmaTransactionContextPtr);

    } // Configure transmit DMA

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
VOID
IMXUartDeinitializeDma (
    WDFDEVICE WdfDevice
    )
{
    PAGED_CODE();
    IMX_UART_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    IMX_UART_DEVICE_CONTEXT* deviceContextPtr =
        IMXUartGetDeviceContext(WdfDevice);

    IMX_UART_INTERRUPT_CONTEXT* interruptContextPtr =
        deviceContextPtr->InterruptContextPtr;

    IMX_UART_RX_DMA_TRANSACTION_CONTEXT* rxDmaTransactionContextPtr =
        interruptContextPtr->RxDmaTransactionContextPtr;

    if (rxDmaTransactionContextPtr == nullptr) {
        return;
    }

    if (rxDmaTransactionContextPtr->DmaBufferMdlPtr != nullptr) {
        IoFreeMdl(rxDmaTransactionContextPtr->DmaBufferMdlPtr);
    }
    rxDmaTransactionContextPtr->DmaBufferMdlPtr = nullptr;

    if (rxDmaTransactionContextPtr->DmaBufferPtr != nullptr) {
        MmFreeNonCachedMemory(
            rxDmaTransactionContextPtr->DmaBufferPtr,
            rxDmaTransactionContextPtr->DmaBufferSize);
    }
    rxDmaTransactionContextPtr->DmaBufferPtr = nullptr;
}

IMX_UART_PAGED_SEGMENT_END; //===================================================
IMX_UART_INIT_SEGMENT_BEGIN; //==================================================

_Use_decl_annotations_
NTSTATUS
DriverEntry (
    DRIVER_OBJECT* DriverObjectPtr,
    UNICODE_STRING* RegistryPathPtr
    )
{
    PAGED_CODE();

    //
    // Initialize logging
    //
    {
        WPP_INIT_TRACING(DriverObjectPtr, RegistryPathPtr);
        RECORDER_CONFIGURE_PARAMS recorderConfigureParams;
        RECORDER_CONFIGURE_PARAMS_INIT(&recorderConfigureParams);
        WppRecorderConfigure(&recorderConfigureParams);
#if DBG
        WPP_RECORDER_LEVEL_FILTER(IMX_UART_TRACING_DEFAULT) = TRUE;
#endif // DBG
    }

    WDF_DRIVER_CONFIG wdfDriverConfig;
    WDF_DRIVER_CONFIG_INIT(&wdfDriverConfig, IMXUartEvtDeviceAdd);
    wdfDriverConfig.DriverPoolTag = IMX_UART_POOL_TAG;
    wdfDriverConfig.EvtDriverUnload = IMXUartEvtDriverUnload;

    WDFDRIVER wdfDriver;
    NTSTATUS status = WdfDriverCreate(
            DriverObjectPtr,
            RegistryPathPtr,
            WDF_NO_OBJECT_ATTRIBUTES,
            &wdfDriverConfig,
            &wdfDriver);

    if (!NT_SUCCESS(status)) {
        IMX_UART_LOG_ERROR(
            "Failed to create WDF driver object. (status = %!STATUS!, DriverObjectPtr = %p, RegistryPathPtr = %p)",
            status,
            DriverObjectPtr,
            RegistryPathPtr);

        return status;
    }

    return STATUS_SUCCESS;
}

IMX_UART_INIT_SEGMENT_END; //====================================================

