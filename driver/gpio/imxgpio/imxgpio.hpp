// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//   imxgpio.hpp
//
// Abstract:
//
//   This file contains types and definitions for i.MX Series GPIO
//   controller driver.
//
// Environment:
//
//  Kernel mode only
//

#ifndef _IMXGPIO_HPP_
#define _IMXGPIO_HPP_ 1

// Signals in i.MX datasheets follow the pattern GPIO<bank+1>_IO<n>
// where banks are 1-based indexed
// eg.: GPIO5_IO07: 8th signal in 5th GPIO bank
// Despite that, this macro expects a 0-based bank index because
// that is how GpioClx numbers GPIO banks
#define IMX_MAKE_PIN_0(BANK, IO)  (((BANK) * 32) + (IO))

// This macro expects a 1-based bank index, it is mainly used to
// facilitate defining datasheet like signals by allowing the use
// of the same bank indices used by the datasheet
#define IMX_MAKE_PIN_1(BANK, IO)  ((((BANK) - 1) * 32) + (IO))

enum : ULONG {
    IMX_GPIO_ALLOC_TAG = 'G6XM'
};

enum {
    // i.MX6 Dual/Quad specifics
    IMX6_GPIO_PINS_PER_BANK = 32,
    IMX6_GPIO_BANK_COUNT = 7,
    IMX6_GPIO_PIN_COUNT = IMX_MAKE_PIN_1(7, 13) + 1, // 206 pins
    IMX6_GPIO_BANK_STRIDE = 0x4000, 
    IMX6_GPIO_PULL_SHIFT = 12, 
    IMX6_GPIO_PULL_MASK = (0b1111 << IMX6_GPIO_PULL_SHIFT)
};

enum {
    // i.MX6 ULL specifics
    IMX6ULL_GPIO_PINS_PER_BANK = 32,
    IMX6ULL_GPIO_BANK_COUNT = 5,
    IMX6ULL_GPIO_PIN_COUNT = IMX_MAKE_PIN_1(4, 27) + 1, // 124 pins
    IMX6ULL_GPIO_BANK_STRIDE = 0x4000,
    IMX6ULL_GPIO_PULL_SHIFT = 12,
    IMX6ULL_GPIO_PULL_MASK = (0b1111 << IMX6ULL_GPIO_PULL_SHIFT)
}; 

enum {
    // i.MX7 Dual specifics
    IMX7_GPIO_PINS_PER_BANK = 32,
    IMX7_GPIO_BANK_COUNT = 7,
    IMX7_GPIO_PIN_COUNT = IMX_MAKE_PIN_1(7, 15) + 1, // 208 pins
    IMX7_GPIO_BANK_STRIDE = 0x10000,
    IMX7_GPIO_PULL_SHIFT = 4,
    IMX7_GPIO_PULL_MASK = (0b111 << IMX7_GPIO_PULL_SHIFT)
};

enum {
    // i.MX8M specifics
    IMX8M_GPIO_PINS_PER_BANK = 32,
    IMX8M_GPIO_BANK_COUNT = 5,
    IMX8M_GPIO_PIN_COUNT = IMX_MAKE_PIN_1(5, 29) + 1, // 157 pins
    IMX8M_GPIO_BANK_STRIDE = 0x10000,
    IMX8M_GPIO_PULL_SHIFT = 6,
    IMX8M_GPIO_PULL_MASK = (0b1 << IMX8M_GPIO_PULL_SHIFT)
};

enum {
    // i.MX8MM specifics
    IMX8MM_GPIO_PINS_PER_BANK = 32,
    IMX8MM_GPIO_BANK_COUNT = 5,
    IMX8MM_GPIO_PIN_COUNT = IMX_MAKE_PIN_1(5, 29) + 1, // 157 pins
    IMX8MM_GPIO_BANK_STRIDE = 0x10000,
    IMX8MM_GPIO_PULL_SHIFT = 6,
    IMX8MM_GPIO_PULL_MASK = (0b101 << IMX8MM_GPIO_PULL_SHIFT)
};

// Determine the maximum size of pin count and bank count for structure creation at compile time.
#define IMX_GPIO_BYTES_PER_BANK 32
#define IMX_GPIO_PINS_PER_BANK 32
#define IMX_GPIO_PINCOUNT_MAX 208 // IMX7_GPIO_PIN_COUNT
#define IMX_GPIO_BANKCOUNT_MAX 7  // IMX7_GPIO_BANK_COUNT

static_assert(
    IMX_GPIO_PINS_PER_BANK <= 32,
    "Driver supports max of 32 pins per bank");

//
// GPIO PULL Up/Down defines
//   All bits = 0 - Pull up/down off
//
enum IMX_GPIO_PULL {
    IMX_GPIO_PULL_DISABLE = 0x0, // 0b0000
    IMX6_GPIO_PULL_DOWN = 0x3, // 0b0011 PUS=00 (100K PD), PUE=1, PKE=1
    IMX6_GPIO_PULL_UP = 0xB,   // 0b1011 PUS=10 (100K PU), PUE=1, PKE=1
    IMX7_GPIO_PULL_DOWN = 0x1, // 0b001 PS=00 (100K PD), PE=1
    IMX7_GPIO_PULL_UP = 0x7,   // 0b111 PS=11 (100K PU), PE=1
    IMX8M_GPIO_PULL_UP = 0x1,  // 0b001 PUE=1 (no pull down implemented for iMX8M)
    IMX8MM_GPIO_PULL_UP = 0x5,   // 0b101 PE=1 (Pull enabled) PUE=1 (Pull up)
    IMX8MM_GPIO_PULL_DOWN = 0x4, // 0b100 PE=1 (Pull enabled) PUE=0 (Pull down)
    IMX_GPIO_PULL_INVALID = 0xFFFFFFFE,
    IMX_GPIO_PULL_DEFAULT = 0xFFFFFFFF  // Set to HW default
};

enum IMX_GPIO_INTERRUPT_CONFIG {
    IMX_GPIO_INTERRUPT_CONFIG_LOW_LEVEL = 0x0,
    IMX_GPIO_INTERRUPT_CONFIG_HIGH_LEVEL = 0x1,
    IMX_GPIO_INTERRUPT_CONFIG_RISING_EDGE = 0x2,
    IMX_GPIO_INTERRUPT_CONFIG_FALLING_EDGE = 0x3
};

#define IMX_GPIO_INTERRUPT_CONFIG_MASK 0x03

enum IMX_GPIO_FUNCTION {
    IMX_GPIO_FUNCTION_ALT0 = 0x0,
    IMX_GPIO_FUNCTION_ALT1 = 0x1,
    IMX_GPIO_FUNCTION_ALT2 = 0x2,
    IMX_GPIO_FUNCTION_ALT3 = 0x3,
    IMX_GPIO_FUNCTION_ALT4 = 0x4,
    IMX_GPIO_FUNCTION_ALT5 = 0x5,
    IMX_GPIO_FUNCTION_ALT6 = 0x6,
    IMX_GPIO_FUNCTION_ALT7 = 0x7,
    IMX_GPIO_FUNCTION_MASK = 0x7,
    IMX_GPIO_FUNCTION_DEFAULT = 0xFFFFFFFF
};

#define IMX_GPIO_FUNCTION_MASK 0b0111

#define IMX_GPIO_INVALID_PIN ((ULONG) -1)

enum IMX_VENDOR_DATA_TAG : UCHAR {
    IMX_TAG_EMPTY = 0x0,
    IMX_TAG_PADCTL = 0x01,
};

struct IMX_VENDOR_DATA_ELEMENT {
    IMX_VENDOR_DATA_TAG Tag;
    union {
        ULONG PadCtl;
    };
}; // struct IMX_VENDOR_DATA_ELEMENT

#include <pshpack1.h>  //====================================================

#define IMX_IOMUXC_REGISTER_SIZE 0x950
struct IMX_IOMUXC_REGISTERS {
    ULONG Reg[IMX_IOMUXC_REGISTER_SIZE / sizeof(ULONG)];
}; // struct IMX_IOMUXC_REGISTERS

struct IMX_GPIO_BANK_REGISTERS {
    ULONG Data;                 // GPIOx_DR
    ULONG Direction;            // GPIOx_GDIR
    ULONG PadStatus;            // GPIOx_PSR
    ULONG InterruptConfig1;     // GPIOx_ICR1
    ULONG InterruptConfig2;     // GPIOx_ICR2
    ULONG InterruptMask;        // GPIOx_IMR
    ULONG InterruptStatus;      // GPIOx_ISR
    ULONG EdgeSelect;           // GPIOx_EDGE_SEL
}; // struct IMX_GPIO_BANK_REGISTERS

#include <poppack.h> //======================================================

// Captures a logical pin's IOMUXC_SW_PAD_CTL_* data and IOMUXC_SW_MUX_CTL_* data
struct IMX_PIN_DATA {
    ULONG PadGpioAbsolutePin;
    ULONG PadCtlByteOffset;
    ULONG PadCtlDefault;
    ULONG PadMuxByteOffset;
    ULONG PadMuxDefault;
    ULONG PadInputTableIndex;
}; // IMX_PIN_DATA

// Captures a logical pin's IOMUXC_*_SELECT_INPUT data
struct IMX_PIN_INPUT_DATA {
    ULONG PadGpioAbsolutePin;
    ULONG PadAltModeValue;
    ULONG PadSelInpByteOffset;
    ULONG PadSelInpValue;
}; // IMX_PIN_INPUT_DATA

class IMX_GPIO {
public: // NONPAGED

    static GPIO_CLIENT_PRE_PROCESS_CONTROLLER_INTERRUPT
        PreProcessControllerInterrupt;

    static GPIO_CLIENT_MASK_INTERRUPTS MaskInterrupts;
    static GPIO_CLIENT_UNMASK_INTERRUPT UnmaskInterrupt;
    static GPIO_CLIENT_QUERY_ACTIVE_INTERRUPTS QueryActiveInterrupts;
    static GPIO_CLIENT_CLEAR_ACTIVE_INTERRUPTS ClearActiveInterrupts;
    static GPIO_CLIENT_QUERY_ENABLED_INTERRUPTS QueryEnabledInterrupts;
    static GPIO_CLIENT_RECONFIGURE_INTERRUPT ReconfigureInterrupt;

    static GPIO_CLIENT_READ_PINS_MASK ReadGpioPinsUsingMask;
    static GPIO_CLIENT_WRITE_PINS_MASK WriteGpioPinsUsingMask;

    static GPIO_CLIENT_START_CONTROLLER StartController;
    static GPIO_CLIENT_STOP_CONTROLLER StopController;

    static GPIO_CLIENT_CONNECT_FUNCTION_CONFIG_PINS ConnectFunctionConfigPins;
    static GPIO_CLIENT_DISCONNECT_FUNCTION_CONFIG_PINS DisconnectFunctionConfigPins;

    static GPIO_CLIENT_ENABLE_INTERRUPT EnableInterrupt;
    static GPIO_CLIENT_DISABLE_INTERRUPT DisableInterrupt;

private: // NONPAGED

    enum class _SIGNATURE {
        UNINITIALIZED = '6xmi',
        CONSTRUCTED = '6XMI',
        DESTRUCTED = 0
    } signature;

    struct _BANK_INTERRUPT_CONFIG_REGISTERS {

        _BANK_INTERRUPT_CONFIG_REGISTERS() :
            ICR1(0),
            ICR2(0),
            IMR(0)
        { }

        ULONG ICR1;
        ULONG ICR2;
        ULONG IMR;
    }; // struct _BANK_INTERRUPT_CONFIG_REGISTERS

    static NTSTATUS GpioPullModeToImxPullMode(
        UCHAR pullConfiguration,
        IMX_GPIO_PULL *pullMode
        );

    NTSTATUS configureInterrupt (
        BANK_ID BankId,
        PIN_NUMBER PinNumber,
        KINTERRUPT_MODE InterruptMode,
        KINTERRUPT_POLARITY Polarity);

    NTSTATUS resetPinFunction (
        ULONG AbsolutePinNumber
        );

    NTSTATUS setPullMode (
        ULONG AbsolutePinNumber,
        IMX_GPIO_PULL PullMode
        );

    NTSTATUS setPinPadCtl (
        ULONG AbsolutePinNumber,
        ULONG PadCtl
        );

    const IMX_PIN_INPUT_DATA* findPinAltInputSetting (
        ULONG AbsolutePinNumber,
        IMX_GPIO_FUNCTION Function
        );

    _IRQL_requires_max_(APC_LEVEL)
    static NTSTATUS readDeviceProperties (
        DEVICE_OBJECT* PdoPtr,
        _Out_ UINT32* SocTypePtr
        );

    static bool isBankValid (
        BANK_ID BankId
        );

    static bool isBankAndPinValid (
        BANK_ID BankId, 
        PIN_NUMBER BankPinNumber
        );

    static inline PIN_NUMBER getPhysicalPinNumber (
        BANK_ID BankId, 
        PIN_NUMBER PinNumber
        )
    {
        return PinNumber + (BankId % 2) * 16;
    }

    static inline BANK_ID getPhysicalBankId (
        BANK_ID BankId
        )
    {
        return BankId / 2;
    }

    static inline BANK_ID getPhysicalPinShift (
        BANK_ID BankId
        )
    {
        return (BankId % 2) * 16;
    }

    inline bool isPinMapped (
        PIN_NUMBER AbsolutePinNumber
        )
    {
        return (AbsolutePinNumber == this->gpioAbsolutePinDataMap[AbsolutePinNumber].PadGpioAbsolutePin);
    }

    IMX_GPIO_BANK_REGISTERS* gpioBankAddr[IMX_GPIO_BANKCOUNT_MAX]; // Map of GPIO banks to addresses
    IMX_IOMUXC_REGISTERS* iomuxcRegsPtr;    // pointer to IMX MUXC register base
    ULONG iomuxcRegsLength;
    const IMX_PIN_INPUT_DATA* const gpioPinInputSelectTable;     // pointer to SOC specific pin info
    ULONG gpioPinInputSelectTableLength;

    // shadow copies of IMX GPIO DR registers and IMX GPIO IC registers
    ULONG banksDataReg[IMX_GPIO_BANKCOUNT_MAX];
    ULONG banksDirectionReg[IMX_GPIO_BANKCOUNT_MAX];
    _BANK_INTERRUPT_CONFIG_REGISTERS banksInterruptConfig[IMX_GPIO_BANKCOUNT_MAX];

    WDFDEVICE wdfDevice;

    // device context configuration data
    IMX_PIN_DATA gpioAbsolutePinDataMap[IMX_GPIO_PINCOUNT_MAX];
    ULONG gpioInputSelectOwningPin[IMX_IOMUXC_REGISTER_SIZE/sizeof(ULONG)];
    ULONG gpioInputSelectDefaultValue[IMX_IOMUXC_REGISTER_SIZE/sizeof(ULONG)];
    ULONG gpioDirectionDefaultValue[IMX_GPIO_BANKCOUNT_MAX];

    ULONG openIoPins[IMX_GPIO_BANKCOUNT_MAX];
    ULONG openInterruptPins[IMX_GPIO_BANKCOUNT_MAX];

    // Global platform specific values set during IMX_GPIO::PrepareController
    static UINT32 cpuRev;
    static UINT32 bankStride;
    static UINT32 bankCount;
    static USHORT pinCount;
    static UINT32 pullShift;
    static UINT32 pullMask;
    static UINT32 pullUp;
    static UINT32 pullDown;
    static const IMX_PIN_DATA* sparsePinMap;
    static UINT32 sparsePinMapLength;
    static const IMX_PIN_INPUT_DATA* inputSelectMap;
    static UINT32 inputSelectMapLength;

public: // PAGED

    static GPIO_CLIENT_QUERY_CONTROLLER_BASIC_INFORMATION
        QueryControllerBasicInformation;

    static GPIO_CLIENT_PREPARE_CONTROLLER PrepareController;
    static GPIO_CLIENT_RELEASE_CONTROLLER ReleaseController;

    static GPIO_CLIENT_CONNECT_IO_PINS ConnectIoPins;
    static GPIO_CLIENT_DISCONNECT_IO_PINS DisconnectIoPins;

    static EVT_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;
    static EVT_WDF_DRIVER_UNLOAD EvtDriverUnload;

    static NTSTATUS configureCPUType();

private: // PAGED

    static EVT_WDF_WORKITEM EvtSampleInterruptStatusWorkItem;

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS setPinFunction (
        ULONG AbsolutePinNumber,
        IMX_GPIO_PULL PullMode,
        IMX_GPIO_FUNCTION Function
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    IMX_GPIO (
        _In_ IMX_IOMUXC_REGISTERS* IomuxcRegsPtrVal,
        ULONG IomuxcRegsLength,
        _In_ VOID* GpioBankAddr[],
        const IMX_PIN_INPUT_DATA* GpioPinInputMapVal,
        ULONG GpioPinInputLength
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    ~IMX_GPIO ();

    IMX_GPIO (const IMX_GPIO&) = delete;
    IMX_GPIO& operator= (const IMX_GPIO&) = delete;
}; // class IMX_GPIO

extern "C" DRIVER_INITIALIZE DriverEntry;

#endif // _IMXGPIO_HPP_
