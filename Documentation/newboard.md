Adding New Boards to the BSP
================

This document walks through the steps required to add a new board configuration for FFU image builds.

## Initialize a new board configuration
  1. Open PowerShell and run `imx-iotcore\build\tools\NewiMX6Board.ps1 <NewBoardName>`
     - Note: `<NewBoardName>` should follow the schema of BoardName_SoCType_MemoryCapacity. See `imx-iotcore\build\board` for examples.
     - The following instructions assume an example board named **ContosoBoard_iMX6Q_1GB**
     - If the script is blocked by execution policy, you may need to invoke powershell from an administrator command prompt to bypass the powershell script execution policy: `powershell.exe -executionpolicy bypass .\NewiMX6Board.ps1 <NewBoardName>`
  2. This step will create a new board configuration in `imx-iotcore\build\board\` and a new firmware folder in `imx-iotcore\build\firmware`.
## Setup the solution in Visual Studio
   1. Open up the Solution Explorer view (Ctrl + Alt + L).
   2. Right click the Board Packages folder and select Add Existing Project.
   3. Select ```imx-iotcore\build\board\ContosoBoard_iMX6Q_1GB\Package\MyBoardPackage.vcxproj```
   4. Right click your new project => Build Dependencies => Project Dependencies then select **HalExtiMX6Timers**, **imxusdhc**, and **mx6pep**.
      * For an i.MX7 project select **HalExtiMX7Timers** and **imxusdhc**.
   5. Right click the GenerateTestFFU project => Build Dependencies => Project Dependencies then select your new project from the list.
## Update the firmware for your board
  1. Port the firmware to your board following the [i.MX Porting Guide](porting-imx.md)
  2. You must modify `imx-iotcore\build\firmware\ContosoBoard_iMX6Q_2GB\Makefile` with the appropriate values for all CONFIG options. This is used by the makefile to configure each firmware build.
      ```makefile
      # Select the defconfig file to use in U-Boot
      UBOOT_CONFIG=mx6sabresd_nt_defconfig

      # Select the DSC file name to use in EDK2
      EDK2_DSC=Sabre_iMX6Q_1GB
      # Select the subdirectory of the Platform folder for this board
      EDK2_PLATFORM=NXP/Sabre_iMX6Q_1GB
      # Select DEBUG or RELEASE build of EDK2
      EDK2_DEBUG_RELEASE=RELEASE

      # Select the FIT Image Tree Source file used to bundle and sign U-Boot and OP-TEE
      UBOOT_OPTEE_ITS=uboot_optee_unsigned.its
      # Select the FIT Image Tree Source file used to bundle and sign UEFI
      UEFI_ITS=uefi_unsigned.its

      all: firmware_fit.merged firmwareversions.log

      include ../Common.mk

      .PHONY: $(OPTEE)
      # Select the PLATFORM for OP-TEE to build
      $(OPTEE):
            $(MAKE) -C $(OPTEE_ROOT) O=$(OPTEE_OUT) PLATFORM=imx-mx6qsabresd \
            $(OPTEE_FLAGS_IMX6)
      ```
  3. This new firmware folder and updated makefile will allow you to use the common firmware makefile to build your firmwares. The makefile can be invoked from `imx-iotcore\build\firmware`. This can be run directly from WSL, on a Linux host, or in CMD by prepending make with "wsl"

      WSL and Linux:
      ```bash
        cd imx-iotcore/build/firmware
        make ContosoBoard_iMX6Q_1GB
      ```
      CMD and PowerShell:
      ```bash
        cd imx-iotcore\build\firmware
        wsl make ContosoBoard_iMX6Q_1GB
      ```
## Build the FFU in Visual Studio
  1. Edit **GenerateFFU.bat** in Build Scripts and comment out the HummingBoard build target using REM. This will speed up FFU generation time since it will only build the FFU for your board.
  2. Select the Release or Debug build target, then right click and build GenerateTestFFU.
  3. After FFU generation completes, your FFU will be available in ```imx-iotcore\build\solution\iMXPlatform\Build\FFU\ContosoBoard_iMX6Q_1GB``` and can be flashed to an SD card following the instructions in the [IoT Core Manufacturing Guide](https://docs.microsoft.com/en-us/windows-hardware/manufacture/iot/create-a-basic-image#span-idflashanimagespanflash-the-image-to-a-memory-card)

## Board Package Project Meanings
The board package projects are used to build the following packages:
* **Platform Firmware:** BootFirmware, BootLoader
* **Platform Identity:** SystemInformation
* **Filesystem Layout:** DeviceLayoutProd, OEMDevicePlatform
* **UpdateOS Drivers:** SVPlatExtensions

The board packages have a dependency on HalExtiMX6Timers, mx6pep, and imxusdhc because those are the minimum set of boot critical drivers for i.MX6, so the UpdateOS Drivers package SVPlatExtensions requires them.