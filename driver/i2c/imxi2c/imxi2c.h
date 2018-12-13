/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Module Name:

    imxi2c.h

Abstract:

    This module contains the controller-specific type
    definitions for the SPB controller driver hardware.

Environment:

    kernel-mode only

Revision History:

*/

//
// Includes for hardware register definitions.
//

#ifndef _IMXI2C_H_
#define _IMXI2C_H_

#include "imxi2chw.h"

//
// iMX I2C controller registers.
// MITT tests require 16 kB maximum transfer size.
// for short term set to 64 kB, and consider removing the limit eventually
#define IMXI2C_MAX_TRANSFER_LENGTH 0x0000FFFF

// register structure must match the register mapping of iMX I2C controller hardware

typedef struct _IMXI2C_REGISTERS_ {

    __declspec(align(4)) HWREG<USHORT> AddressReg;     // offset 0
    __declspec(align(4)) HWREG<USHORT> FreqDivReg;     // offset 4
    __declspec(align(4)) HWREG<USHORT> ControlReg;     // offset 8
    __declspec(align(4)) HWREG<USHORT> StatusReg;      // offset Ch
    __declspec(align(4)) HWREG<USHORT> DataIOReg;      // offset 10h
}
IMXI2C_REGISTERS, *PIMXI2C_REGISTERS;

// i.MX I2C register bit fields and macros
// iMX I2C is a 16-bit block. Only half-word accesses should be performed to the block.

// I2C Address Register (I2Cx_IADR)
// 0h offset

#define IMX_I2C_ADDR_REG_RESRVD_MASK    (USHORT)0xFF00 // bits 15-7 - reserved. Always reads as 0
#define IMX_I2C_ADDR_REG_SLAV_ADR_MASK  (USHORT)0x00FE // bits 7-1 - i2c Slave address.
#define IMX_I2C_ADDR_REG_RESRVD1_MASK   (USHORT)0x0001 // bit 0 - reserved. Always reads as 0

#define IMX_I2C_ADDR_REG_SLAV_ADR_SHIFT (USHORT)0x0001
#define IMX_I2C_ADDR_REG_SLAV_ADR_SIZE  (USHORT)0x0007

// I2C Frequency Divider Register (I2Cx_IFDR)
// 4h offset

#define IMX_I2C_FREQ_DIV_REG_RERVD_MASK  (USHORT)0xFF00 // bits 15-7 - reserved. Always reads as 0
#define IMX_I2C_FREQ_DIV_REG_IC_MASK     (USHORT)0x003F // bits 5-0 - IC clock rate
#define IMX_I2C_FREQ_DIV_REG_IC_SIZE     (USHORT)0x0006

// I2C Control Register (I2Cx_I2CR)
// 8h offset

#define IMX_I2C_CTRL_REG_RERVD2_MASK (USHORT)0xFF00 // bits 15-7 - reserved. Always reads as 0
#define IMX_I2C_CTRL_REG_IEN_MASK    (USHORT)0x0080 // bit 7 - I2C enable.
#define IMX_I2C_CTRL_REG_IEN_SHIFT   (USHORT)0x0007

#define IMX_I2C_CTRL_REG_IIEN_MASK   (USHORT)0x0040 // bit 6 - I2C interrupt enable.
#define IMX_I2C_CTRL_REG_IIEN_SHIFT  (USHORT)0x0006

#define IMX_I2C_CTRL_REG_MSTA_MASK   (USHORT)0x0020 // bit 5 - Master/Slave mode select
#define IMX_I2C_CTRL_REG_MSTA_SHIFT  (USHORT)0x0005 // 1 - master

#define IMX_I2C_CTRL_REG_MTX_MASK    (USHORT)0x0010 // bit 4 - Transmit/Receive mode select
#define IMX_I2C_CTRL_REG_MTX_SHIFT   (USHORT)0x0004 // 1- transmit

#define IMX_I2C_CTRL_REG_TXAK_MASK   (USHORT)0x0008 // bit 3 - 1=Transmit will not send an acknowledgement
#define IMX_I2C_CTRL_REG_TXAK_SHIFT  (USHORT)0x0003 //         0=Transmit will send an acknowledgement

#define IMX_I2C_CTRL_REG_RSTA_MASK   (USHORT)0x0004 // bit 2 - Repeat start. Always reads as 0
#define IMX_I2C_CTRL_REG_RSTA_SHIFT  (USHORT)0x0002 // p.1901 there must be a delay of at least two module clock cycles after it sets the I2C_I2CR[RSTA] bit and before writing to the I2C_I2DR register

#define IMX_I2C_CTRL_REG_RERVD1_MASK (USHORT)0x0003 // bits 0-1 - reserved. Always reads as 0
#define IMX_I2C_CTRL_REG_RERVD1_SHIFT (USHORT)0x0000

// I2C Status Register (I2Cx_I2SR)
// Ch offset

#define IMX_I2C_STA_REG_RERVD2_MASK (USHORT)0xFF00 // bits 15-7 - reserved. Always reads as 0
#define IMX_I2C_STA_REG_ICF_MASK    (USHORT)0x0080 // bit 7 - Data transferring bit
#define IMX_I2C_STA_REG_ICF_SHIFT   (USHORT)0x0007 // 0 Transfer is in progress, 1 Transfer is complete

#define IMX_I2C_STA_REG_IAAS_MASK   (USHORT)0x0040 // bit 6 - I2C addressed as a slave bit
#define IMX_I2C_STA_REG_IAAS_SHIFT  (USHORT)0x0006 // 0 Not addressed, 1 Addressed as a slave

#define IMX_I2C_STA_REG_IBB_MASK    (USHORT)0x0020 // bit 5 - I2C bus busy bit
#define IMX_I2C_STA_REG_IBB_SHIFT   (USHORT)0x0005 // 0 Bus is idle. If a Stop signal is detected, IBB is cleared.
                                                    // 1 Bus is busy. When Start is detected, IBB is set.

#define IMX_I2C_STA_REG_IAL_MASK    (USHORT)0x0010 // bit 4 - Arbitration lost
#define IMX_I2C_STA_REG_IAL_SHIFT   (USHORT)0x0004 // Set by hardware in the following circumstances (IAL must be cleared by software by writing a "0" to it at the start of the interrupt service routine)
                                                    // Software cannot set the bit.
// 0 - No arbitration lost. 1 - Arbitration is lost. Bit is when any of conditions below occurs
// - A Start cycle is attempted when the bus is busy.
// - A Repeated Start cycle is requested in Slave mode.
// - A Stop condition is detected when the master did not request it.

#define IMX_I2C_STA_REG_RERVD1_MASK (USHORT)0x0008 // bit 3 - reserved. Always reads as 0
#define IMX_I2C_STA_REG_RERVD1_SHIFT (USHORT)0x0003

#define IMX_I2C_STA_REG_SRW_MASK    (USHORT)0x0004 // bit 2 - Slave read/write
#define IMX_I2C_STA_REG_SRW_SHIFT   (USHORT)0x0002 // 0 Slave receive, master writing to slave. 1 Slave transmit, master reading from slave

#define IMX_I2C_STA_REG_IIF_MASK    (USHORT)0x0002 // bit 1 - I2C interrupt
#define IMX_I2C_STA_REG_IIF_SHIFT   (USHORT)0x0001 // 0 No I2C interrupt pending. 1 An interrupt is pending.
                                                    // This causes a processor interrupt request (if the interrupt enable is asserted [IIEN = 1]).
                                                    // The software cannot set the bit.
// the interrupt is set at the falling edge of the ninth clock when:
// - One byte transfer is completed
// - An address is received that matches its own specific address in Slave Receive mode.
// - Arbitration is lost.

#define IMX_I2C_STA_REG_RXAK_MASK   (USHORT)0x0001 // bit 0 - Received acknowledge
#define IMX_I2C_STA_REG_RXAK_SHIFT  (USHORT)0x0000 // 0 An "acknowledge" signal was received after the completion of an 8-bit data transmission on the bus.
                                                    // 1 A "No acknowledge" signal was detected at the ninth clock.
// I2C Data I/O Register (I2Cx_I2DR)
// 10h offset

#define IMX_I2C_DATAIO_REG_RERVD_MASK  (USHORT)0xFF00 // bits 15-7 - reserved. Always reads as 0
#define IMX_I2C_DATAIO_REG_DATA_MASK   (USHORT)0x00FF // bits 7-0 - Data Byte
#define IMX_I2C_DATAIO_REG_DATA_SHIFT  (USHORT)0x0000

#endif // of _IMXI2C_H_