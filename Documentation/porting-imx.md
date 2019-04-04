i.MX Porting Guide
================

This document walks through bringing up Windows on new i.MX6 and i.MX7 boards. The firmware and drivers can be ported to new boards by changing settings that differ from board to board. This includes

 - SoC type (i.MX6 Quad/QuadPlus/Dual/DualLite/Solo/SoloX, i.MX7 Dual)
 - MMDC initialization
 - DDR size
 - Console UART selection
 - Boot device selection
 - Pin muxing
 - Exposed devices
 - Off-SoC peripherals

The overall process is to bring up each of the firmware components in sequence, then create packages, and finally create an FFU configuration. By the end, there will be a new board configuration in the repository that builds an FFU for your board. It is important to create new configurations for your board instead of modifying existing ones, so that you can easily integrate code changes from our repositories. We encourage you to submit your changes via pull request to our repositories so that we can make code changes without breaking your build.

This guide is structured in two parts:
 1. Create Windows Compatible U-Boot and OPTEE configurations for your own board
 1. Add your board to the firmware build system
 1. Resolve compilation errors to build your firmwares into a firmware_fit.merged.
 1. Iterate on builds of SPL, U-BOOT, OP-TEE, UEFI until Windows boots with minimum support.
 1. Bring up devices one at a time

Before starting, you should read the [boot flow](boot.md) document to get an idea of the boot process.

In the discussion that follows, replace `yourboard` with a concise name of your board.

# Limitations and Known Issues

The imx-iotcore reference BSP has the following limitations:
 - SD/eMMC is the only supported boot media
 - HDMI on IPU1 DI0 is the only supported display

# 1. U-Boot

The first step is to bring up U-Boot SPL. We use U-Boot in a specific way to implement certain security features, so even if a U-Boot configuration already exists for your board, you will need to create a new configuration for booting Windows. The operation of SPL is described [here](boot-firmware.md). Your board must follow the same general flow, with only board-specific changes.

1. Clone our reference U-Boot repository.

        git clone --recursive -b u-boot-imx https://github.com/ms-iot/u-boot.git

1. Copy `configs/mx6cuboxi_nt_defconfig` to `configs/yourboard_nt_defconfig` (For iMX7 start with mx7sabresd_nt_defconfig or clsomimx7)
1. Edit `configs/yourboard_nt_defconfig`
  1. Change `CONFIG_TARGET_MX6CUBOXI=y` to `CONFIG_TARGET_YOURBOARD=y`. If your board is not already supported by U-Boot, you will need to add it to U-Boot. We walk through this process below.
  1. Change `MX6QDL` in the line `CONFIG_SYS_EXTRA_OPTIONS="IMX_CONFIG=arch/arm/mach-imx/spl_sd.cfg,MX6QDL"`
  to the appropriate value for your board. Possible values are listed in `arch/arm/mach-imx/mx6/Kconfig` and `arch/arm/mach-imx/mx7/Kconfig`.

If your board is already supported by U-Boot, you'll still need to make sure that the correct configuration options are set.

## U-Boot Configuration Options

Here are some important configuration options for booting Windows.

 * `CONFIG_BAUDRATE=115200` - Sets the UART baud rate to 115200.
 * `CONFIG_BOOTCOMMAND="globalpage init 0x10817000; globalpage add ethaddr; fatload mmc 0:2 0x80A20000 /uefi.fit; bootm 0x80A20000"` - This is the command that automatically runs on startup. It will store the MAC address into the global page for UEFI, then load uefi.fit from mmc and boot the fit to start UEFI. The globalpage commands should be omitted if `CONFIG_CMD_GLOBAL_PAGE` has not been selected.
 * `CONFIG_DISTRO_DEFAULTS=y` - Enables default boot scripting which is used by CONFIG_BOOTCOMMAND.
 * `CONFIG_BOOTDELAY=-2` - Disables the delay before U-Boot runs the bootcommand. The value -2 means that it will not check the serial port for interrupts unlike a delay of 0. This is important because WinDBG continuously sends characters through the UART which will U-Boot to stop in the console if it checks.
 * `CONFIG_FIT=y` - Allow booting of Flattened Image Trees (FIT) which store both binaries and their metadata.
 * `CONFIG_OF_LIBFDT=y` - Adds to U-Boot the library responsible for working with Flattened Device Trees (FDT), of which FITs are a subset.
 * `CONFIG_IMX_PERSIST_INIT=y` - Prevents U-Boot proper from disabling the display and PCIe when booting into UEFI.
 * `CONFIG_CMD_FAT=y` - Enables FAT filesystem commands and is used by the boot script to load UEFI.
 * `CONFIG_CMD_MMC=y` - Enables MMC commands and is used by the boot script to load UEFI.
 * `CONFIG_CMD_PART=y` - Enables part command which is used by UEFI boot script.
 * `CONFIG_HUSH_PARSER=y` - Necessary to enable script parsing.
 * `CONFIG_SECURE_BOOT=y` - Enables the i.MX6 crypto driver.
 * `CONFIG_DEFAULT_DEVICE_TREE="devicetreename"` - Selects a device tree for the platform (eg imx6qdl-mx6cuboxi). Important for the FIT build path that it exists, but the tree can be empty.
 * `CONFIG_SPL=y` - Enables the Secondary Program Loader framework which is required to load and run OP-TEE as soon as possible.
 * `CONFIG_SPL_FIT=y` - Allows SPL to read FITs (U-Boot and OPTEE binaries)
 * `CONFIG_SPL_FIT_SIGNATURE_STRICT=y` - Halt if loadables or firmware don't pass FIT signature verification.
 * `CONFIG_SPL_LOAD_FIT=y` - SPL will attempt to load a FIT to memory
 * `CONFIG_SPL_OF_LIBFDT=y` - Adds to SPL the library responsible for working with Flattened Device Trees (FDT), of which FITs are a subset.
 * `CONFIG_SPL_LEGACY_IMAGE_SUPPORT=n` - Prevents SPL from loading legacy images (which cannot support future security features)
 * `CONFIG_SPL_BOARD_INIT=y` - Enables board specific implementation of `void spl_board_init(void)`
 * `CONFIG_SPL_CRYPTO_SUPPORT=y` - Enables the crypto driver in SPL.
 * `CONFIG_SPL_FSL_CAAM=y` - Enables the CAAM driver in SPL.
 * `CONFIG_SPL_HASH_SUPPORT=y` - Enable hashing drivers in SPL.
 * `CONFIG_SPL_ENABLE_CACHES=y` - Enables caches in SPL, required for the RIoT Tiny SHA256 implementation.
 * `CONFIG_SPL_ECC=y` - Enable support for Elliptic-curve cryptography in SPL using code from the RIoT submodule.
 * `CONFIG_USE_TINY_SHA256=y` - Select the smaller SHA256 implementation from the RIoT submodule instead of U-Boot's default implementation.
 * `CONFIG_SPL_OF_CONTROL=y` - Enable run-time configuration via Device Tree in SPL
 * `CONFIG_SPL_OPTEE_BOOT=y` - Instructs SPL to load and jump to OP-TEE. Required to boot Windows.
 * `CONFIG_SPL_MMC_SUPPORT=y` - Enables MMC support in SPL. Required to load OP-TEE and U-Boot proper.
 * `CONFIG_SPL_USE_ARCH_MEMCPY=n` - Disables use of optimized memcpy routine. Saves space in SPL.
 * `CONFIG_SYS_L2CACHE_OFF=y` - Saves space in SPL by not including L2 initialization and maintenance routines. L2 is not necessary for performance. L2 is enabled by Windows later on.
 * `CONFIG_USE_TINY_PRINTF=y` - Saves space in SPL by selecting minimal printf implementation.



Here are some more configuration options that aren't boot critical for Windows.

 * `CONFIG_CMD_GLOBAL_PAGE=y` - Enables the globalpage command seen in CONFIG_BOOTCOMMAND. This is used to pass the MAC addresses to UEFI.
 * `CONFIG_FIT_VERBOSE=y` - Enables high verbosity when loading and parsing a Flattened Image Tree. Helpful for debugging boot.

## Adding a new board to U-Boot

There a number of resources that guide you through porting U-Boot to new boards and you are encouraged to consult those resources. This section may not be exhaustive.

1. Edit `arch/arm/mach-imx/<mx6 or mx7>/Kconfig` and add a config option for your board.

        config TARGET_YOURBOARD
            bool "Your i.MX board"
            select MX6QDL #(This should change to match CONFIG_SYS_EXTRA_OPTIONS from your defconfig)
            select BOARD_LATE_INIT
            select SUPPORT_SPL

2. Create and initialize a board directory

        mkdir -p board/yourcompany/yourboard
        cp board/solidrun/mx6cuboxi/* board/yourcompany/yourboard/
        mv board/yourcompany/yourboard/mx6cuboxi.c board/yourcompany/yourboard/yourboard.c

3. Edit `board/yourcompany/yourboard/Makefile` and replace `mx6cuboxi.c` with `yourboard.c`

4. Edit `board/yourcompany/yourboard/Kconfig` and set appropriate values for your board. Note that the build system expects `SYS_CONFIG_NAME` to correspond to the name of a header file in `include/configs`.

        if TARGET_YOURBOARD

        config SYS_BOARD
                default "yourboard"

        config SYS_VENDOR
                default "yourcompany"

        config SYS_CONFIG_NAME
                default "yourboard"

        endif

5. Create a config header for your board

        cp include/configs/mx6cuboxi.h include/configs/yourboard.h

6. Edit `include/configs/yourboard.h` as necessary for your board. You may need to add, remove, or change options depending on what's available on your board. Some notable settings are:
 * `CONFIG_MXC_UART_BASE` - set this to the base address of the UART instance that should be used for debug and console output.
 * `CONFIG_SYS_FSL_ESDHC_ADDR` - set this to the base address of the SDHC instance that U-Boot resides on.
 * `CONFIG_EXTRA_ENV_SETTINGS` - This should be set as follows to enable booting UEFI. If `CONFIG_UEFI_BOOT` is defined, you should include `config_uefi_bootcmd.h` and set `CONFIG_EXTRA_ENV_SETTINGS` to `BOOTENV`. You must replace the 0 in mmcdev=0 with the mmc number your device boots from.

        #ifdef CONFIG_UEFI_BOOT
        #include <config_uefi_bootcmd.h>

        #define CONFIG_EXTRA_ENV_SETTINGS \
                "mmcdev=0\0" \
                BOOTENV
        #else

7. Edit `board/yourcompany/yourboard/yourboard.c` and add, change, and remove code as appropriate for your board. Some configurations that will need to change are pin muxing, MMC initialization, DDR size, and DRAM timing parameters.

8. Build your board. Be prepared to spend some time fixing compilation errors as you get your board into buildable shape.

        make yourboard_nt_defconfig
        make

Note: SPL must be less than 44k to fit into the allocated space.

# OP-TEE

OP-TEE is an operating system that runs in ARM TrustZone and provides a Trusted Execution Environment (TEE). OP-TEE is required to boot Windows. OP-TEE does the following:

 - Provides a trusted execution environment for trusted applications
 - Switches to normal world
 - Configures and enables L2 cache when requested by Windows
 - Enables secondary cores when requested by Windows
 - Implements shutdown and reboot
 - Implements power management functionality through PSCI

OP-TEE is mostly board-independent. Right now, the only configuration that needs to be changed is the console UART. In the future, there may be other board-specific configurations that need to change as trusted I/O is implemented.

1. Clone our reference implementation of OP-TEE.

        git clone https://github.com/ms-iot/optee_os.git

2. Add a platform flavor for your board. Edit `core/arch/arm/plat-imx/conf.mk` and add your board to the appropriate flavorlist, e.g.

        mx6q-flavorlist = mx6qsabrelite mx6qsabresd mx6qhmbedge mx6qyourboard
        ---or---
        mx6dl-flavorlist = mx6dlsabresd mx6dlhmbedge mx6dlyourboard
        ---or---
        mx7-flavorlist = mx7dsabresd mx7dclsom mx7yourboard

3. Edit `core/arch/arm/plat-imx/platform_config.h` and set `CONSOLE_UART_BASE` to the appropriate value for your platform.

4. Follow the next section to set up a firmare build folder for your system. This will select the correct flags and make OP-TEE for you. If you need more debug output, you can customize OPTEE_FLAGS in the [Common Makefile](..//build/firmware/Common.mk) set CFG_TEE_CORE_DEBUG=y and CFG_TEE_CORE_LOG_LEVEL=4.

# Setting up your build enviroment to build firmware_fit.merged

In order to build and load both OPTEE and U-Boot you will need to create a Flattened Image Tree (FIT) binary to flash onto your device. The build enviroment for FIT images is integrated into the build infrastructure. This will sign SPL for [high assurance boot](build-firmware.md#signing-and-high-assurance-boot-hab), and combine SPL, U-Boot, and OP-TEE into a single `firmware_fit.merged` file that can be tested manually, or built into an FFU image as part of a BSP.

1. Copy `imx-iotcore/build/firmware/hummingboard` to `imx-iotcore/build/firmware/yourboard`
2. Edit `imx-iotcore/build/firmware/yourboard/Makefile` and change the `UBOOT_CONFIG` and the OP-TEE `PLATFORM` for your board.
3. Run `make` in `imx-iotcore/build/firmware/yourboard` and verify that `firmware_fit.merged` is generated.

Flash firmware_fit.merged to your SD card. If you are using Linux, run

        dd if=firmware_fit.merged of=/dev/sdX bs=512 seek=2

If you are using Windows, use [dd for Windows](http://www.chrysocome.net/dd).

        dd if=firmware_fit.merged of=\\.\PhysicalDriveX bs=512 seek=2

## Testing SPL

When you have firmware_fit.merged building, you should run SPL. If successful, SPL will initialize DRAM, intialize MMC, load OP-TEE and U-Boot proper from MMC, then jump to OP-TEE.

Open a serial terminal to your board at 8N1 115200. Insert the SD card into your board and boot. You should see output similar to the following:

        U-Boot SPL 2018.05-rc1-00004-g5a771d5 (May 25 2018 - 13:16:09 -0700)
        Booting from SD card
        Trying to boot from MMC1

If SPL was able to load and start OPTEE, the next few lines will be

        I/TC:
        I/TC:  OP-TEE version: v0.4.0 #1 Fri May 25 20:22:16 UTC 2018 arm
        I/TC:  Initialized


## Testing OP-TEE

When you have built OP-TEE successfully, you should run it and see that it gets as far as normal world. This will also test SPL. SPL will not jump to OP-TEE unless it also successfully loads U-Boot proper so your FIT image will need to contain both OP-TEE and U-Boot proper.

        dd if=firmware_fit.merged of=/dev/sdX bs=512 seek=2
        ---or---
        dd if=firmware_fit.merged of=\\.\PhysicalDriveX bs=512 seek=2

Boot your device. You should see output similar to the following:

    U-Boot SPL 2018.05-rc1-00004-g5a771d5 (May 25 2018 - 13:16:09 -0700)
    Booting from SD card
    Trying to boot from MMC1
    DEBUG:   [0x0] TEE-CORE:add_phys_mem:524: CFG_SHMEM_START type NSEC_SHM 0x12800000 size 0x00200000
    DEBUG:   [0x0] TEE-CORE:add_phys_mem:524: CFG_TA_RAM_START type TA_RAM 0x10c00000 size 0x01c00000
    .
    .
    .
    INFO:    TEE-CORE: OP-TEE version: 2.3.0-480-gf68edcc #4 Thu Feb  1 00:41:33 UTC 2018 arm
    DEBUG:   [0x0] TEE-CORE:mobj_mapped_shm_init:592: Shared memory address range: 10b00000, 11500000
    DEBUG:   [0x0] TEE-CORE:protect_tz_memory:201: pa 0x10a00000 size 0x01e00000 needs TZC protection
    FLOW:    [0x0] TEE-CORE:protect_tz_memory:221: Unaligned pa 0x10a00000 size 0x01000000
    FLOW:    [0x0] TEE-CORE:protect_tz_memory:221: Unaligned pa 0x10a00000 size 0x00800000
    FLOW:    [0x0] TEE-CORE:protect_tz_memory:221: Unaligned pa 0x10a00000 sizE:protect_tz_memory:240: Protecting pa 0x12000000 size 0x00800000
    INFO:    TEE-CORE: Initialized
    DEBUG:   [0x0] TEE-CORE:init_primary_helper:680: Primary CPU switching to normal world boot

You may also see output from U-Boot.

## Testing U-Boot

U-Boot should already be building from earlier and included in your `firmware_fit.merged` file. U-Boot will run after OP-TEE and attempt to load UEFI. Since UEFI is not present yet, it should fail the script and go to the U-Boot prompt. U-Boot initializes devices then executes the commands in `CONFIG_BOOTCOMMAND`. If it does not attempt to load UEFI, then `CONFIG_BOOTCOMMAND` is probably not set correctly. To see the actual value of `CONFIG_BOOTCOMMAND`, you can inspect `u-boot.cfg` or run `printenv` at the U-Boot prompt and look at the `bootcmd` variable.

During initial bringup it may be helpful to disable all devices in U-Boot except UART and eMMC.

You can create a debug build of U-Boot with the following command:

        make KCFLAGS=-DDEBUG

This is very helpful for debugging, but will cause the size of the binaries to increase. SPL may grow too big, so you may have to use a release build of SPL and a debug build of `u-boot-ivt.img`. It is OK to mix a release build of SPL with a debug build of full U-Boot.

A successful run of U-Boot should have a similar output to the following:

    U-Boot 2018.01-00125-gfb1579e (Jan 31 2018 - 16:54:39 -0800)

    CPU:   Freescale i.MX6Q rev1.5 996 MHz (running at 792 MHz)
    CPU:   Extended Commercial temperature grade (-20C to 105C) at 41C
    Reset cause: WDOG
    Board: MX6 HummingBoard2 (som rev 1.5)
    DRAM:  2 GiB
    MMC:   FSL_SDHC: 0, FSL_SDHC: 1
    Using default environment

    auto-detected panel HDMI
    Display: HDMI (1024x768)
    In:    serial
    Out:   serial
    Err:   serial
    Net:   FEC
    starting USB...
    USB0:   Port not available.
    USB1:   USB EHCI 1.00
    scanning bus 1 for devices... 2 USB Device(s) found
           scanning usb for storage devices... 0 Storage Device(s) found
    switch to partitions #0, OK
    mmc0 is current device
    ** Unable to read file imx6board_efi.fd **
    ** Unrecognized filesystem type **
    Error: failed to load UEFI
    =>

# UEFI

UEFI is required to boot Windows. UEFI provides a runtime environment for the Windows bootloader, access to storage, hardware initialization, ACPI tables, and a description of the memory map. First we construct a minimal UEFI with only eMMC and debugger support. Then, we add devices one-by-one to the system.

1. Clone our reference implementation of EDK2. It is split between edk2 and edk2-platforms. See the Readme here: https://github.com/ms-iot/imx-edk2-platforms
1. Copy `Platform\SolidRun\HUMMINGBOARD_EDGE_IMX6Q_2GB` to `Platform\<Your Company Name>\YOURBOARD_IMX6_XGB`.
1. Rename the `.dsc` and `.fdf` files to match the folder name.

## DSC and FDF file

Edit the `.dsc` file and change the following settings as appropriate for your board:

 * `DRAM_SIZE` - set to `DRAM_512MB`, `DRAM_1GB`, or `DRAM_2GB`
 * `IMX_FAMILY` - set to `IMX6DQ`, `IMX6SX`, or `IMX6SDL`
 * `IMX_CHIP_TYPE` - set to `QUAD`, `DUAL`, or `SOLO`
 * `giMXPlatformTokenSpaceGuid.PcdSdhc[1,2,3,4]Enable` - enable the right SDHC instance for your platform. For example,

        giMXPlatformTokenSpaceGuid.PcdSdhc2Enable|TRUE

 * `giMXPlatformTokenSpaceGuid.PcdSerialRegisterBase` - set to the base address of the UART instance that you want UEFI output to go to.
 * `giMXPlatformTokenSpaceGuid.PcdKdUartInstance` - set this to 1, 2, 3, 4, or 5 (6 and 7 are also available on i.MX7). This is the UART instance that Windows will use for kernel debugging. You will also need to reference `giMXPlatformTokenSpaceGuid.PcdKdUartInstance` in your board's `AcpiTables.inf` file. U-Boot must initialize the UART, including baud rate and pin muxing. Windows will not reinitialize the UART.

## Board-specific Initialization

The file `Platform\<Your Company Name>\YOURBOARD_IMX6_XGB\Library\iMX6BoardLib\iMX6BoardInit.c` contains board-specific initialization code, which includes:

 - Pin Muxing
 - Clock initialization
 - PHY initialization

Much of the same functionality exists in U-Boot. The content in this file should be minimized and board-specific initialization should be done in U-Boot. The goal is to eventually eliminate this file.

Start with an empty `ArmPlatformInitialize()` function, and add code as necessary when you bring up each device. **Prefer to add code to U-Boot instead**. This will keep pin muxing, clock initialization, and PHY initialization all in one place.

## SMBIOS

Edit `Platform\<Your Company Name>\YOURBOARD_IMX6_XGB\Drivers\PlatformSmbiosDxe\PlatformSmbiosDxe.c` and set values appropriate for your board. Settings to change are:

        mBIOSInfoType0Strings
        mSysInfoType1Strings
        mBoardInfoType2Strings
        mEnclosureInfoType3Strings
        mProcessorInfoType4Strings
        mMemDevInfoType17.Size

## ACPI Tables

For initial bringup, you should start with a minimal DSDT that contains only the devices required to boot. You should then add devices one-by-one, and test each device as you bring it up.

Edit `Platform\<Your Company Name>\YOURBOARD_IMX6_XGB\AcpiTables\DSDT.asl` and remove all but the following entries:

        include("Dsdt-Common.inc")
        include("Dsdt-Platform.inc")
        include("Dsdt-Gpio.inc")
        include("Dsdt-Sdhc.inc")

### SDHC

Edit `Dsdt-Sdhc.inc` and ensure that the SDHC instance on which your boot media resides is present and enabled. To simplify bringup you should disable the other SDHC instances. A minimal SDHC device node looks like:

        //
        // uSDHC2: SDIO Slot
        //
        Device (SDH2)
        {
           Name (_HID, "FSCL0008")
           Name (_UID, 0x2)

           Method (_STA) // Status
           {
               Return(0xf) // Enabled
           }

           Name (_S1D, 0x1)
           Name (_S2D, 0x1)
           Name (_S3D, 0x1)
           Name (_S4D, 0x1)

           Method (_CRS, 0x0, NotSerialized) {
               Name (RBUF, ResourceTemplate () {
                   MEMORY32FIXED(ReadWrite, 0x02194000, 0x4000, )
                   Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 55 }
               })
               Return(RBUF)
           }

           Name (_DSD, Package()
           {
               ToUUID ("DAFFD814-6EBA-4D8C-8A91-BC9BBF4AA301"),
               Package ()
               {
                   Package (2) { "BaseClockFrequencyHz", 198000000 },  // SDHC Base/Input Clock: 198MHz
                   Package (2) { "Regulator1V8Exist", 0 },             // 1.8V Switching External Circuitry: Not-Implemented
                   Package (2) { "SlotCount", 1 },                     // Number of SD/MMC slots connected on the bus: 1
                   Package (2) { "RegisterBasePA", 0x02194000 }        // Register base physical address
               }
           })

           //
           // Child node to represent the only SD/MMC slot on this SD/MMC bus
           // In theory an SDHC can be connected to multiple SD/MMC slots at
           // the same time, but only 1 device will be selected and active at
           // a time
           //
           Device (SD0)
           {
               Method (_ADR) // Address
               {
                 Return (0) // SD Slot 0
               }

               Method (_RMV) // Remove
               {
                 Return (0) // Removable
               }
           }
        }

`_RMV` is an ACPI method that returns whether the slot is removable or not where 1 indicates removable while 0 means non-removable. eMMC slots should be marked as non-removable, while SD slots should be also marked as non-removable if it can be used as a boot media not as a secondary storage.

`_DSM` is an ACPI method that is used by the SD bus to perform very specialized and platform dependent tasks. It is currently used by Windows to perform SD bus power control On/Off which is required during 3V3/1V8 SD voltage switching sequence. For bring-up, the `_DSM` is not required and in that case the `Regulator1V8Exist` field should be set to 0 to indicate that SD voltage switching is not implemented/supported.

```c
Package (2) { "Regulator1V8Exist", 0 }, // 1.8V Switching External Circuitry: Not implemented
```
### PWM

For the best experience using the PWM WinRT APIs from UWP apps some additional device properties need to be set. Documentation on these device interface properties can be found here in the [Setting device interface properties](https://docs.microsoft.com/en-us/windows-hardware/drivers/spb/pulse-width-controller%20driver#setting-device-interface-properties) section of the PWM DDI MSDN article.

For an example of setting the PWM deivce interface properties statically from an inf file, see the [Virtual PWM driver sample](https://github.com/Microsoft/Windows-iotcore-samples/tree/develop/Drivers/VirtualPWM).

For an example on how to read the ACPI _DSD from within a kernel driver, see the [i.MX SDHC driver here](../driver/sd/imxusdhc).

#### PWM References:

- [PWM DDI](https://docs.microsoft.com/en-us/windows-hardware/drivers/spb/pulse-width-controller%20driver)
- [PWM Driver Reference](https://docs.microsoft.com/en-us/windows/desktop/devio/pwm-api)
- [PWM WinRT APIs](https://docs.microsoft.com/en-us/uwp/api/windows.devices.pwm)
- [Virtual PWM Driver Sample](https://github.com/Microsoft/Windows-iotcore-samples/tree/develop/Drivers)

## Security TAs

UEFI includes a pair of OP-TEE Trusted Applications (TAs) which implement a firmware TPM, and a UEFI authenticated variable store. These binaries should generally not require re-compiling; however, if your OP-TEE has been changed (including build flags) it may introduce incompatibilities. See [Updating the TAs](./build-firmware.md#Updating-the-TAs) for instructions on adding new TAs to your firmware binaries.

They are included in UEFI by default but can be omitted with the CONFIG_NOT_SECURE_UEFI=1 flag. The TAs require OP-TEE to have access to secure storage (eMMC's RPMB). Windows will not support Bitlocker, Secure Boot, or persistent firmware variable storage without these TAs enabled.

## Building UEFI

For a detailed guide on how to build the i.MX UEFI firmware image, please refer to [Building and Updating Firmware](./build-firmware.md).

## Testing UEFI

To test UEFI, you will need an SD card with a FAT partition. The easiest way to get an SD card with the right partition layout is to flash the HummingBoard FFU, then replace the firmware components.

1. Build a HummingBoard FFU by following the instructions on the [Readme](../README.md).
2. Flash the HummingBoard FFU to your SD card

        dism /apply-image /imagefile:HummingBoardTestOEMInput.xml.Release.ffu /applydrive:\\.\PhysicalDriveX /skipPlatformCheck

3. Use the `dd` command to flash `firmware_fit.merged` to the SD card.
4. Replace `uefi.fit` on the EFIESP partition of the SD card with your `uefi.fit`.

Power on the system. You should see UEFI run after U-Boot, and UEFI should attempt to load Windows.

# Booting Windows

As long as the serial console and SDHC device node are configured correctly in UEFI, the Windows kernel should get loaded. Once you see the kernel looking for a debugger connection, you can close the serial terminal and start WinDBG.

        windbg.exe -k com:port=COM3,baud=115200

If you hit an `INACCESSIBLE_BOOT_DEVICE` bugcheck, it means there's a problem with the storage driver. Run `!devnode 0 1` to inspect the device tree, and see what the status of the SD driver is. You can dump the log from the SD driver by running:

        !rcdrkd.rcdrlogdump imxusdhc.sys

After you have a minimal booting Windows image, the next step is to bring up and test each device.
