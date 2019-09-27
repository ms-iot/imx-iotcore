Booting WinPE and Flashing eMMC
==============

The purpose of this document is show how to boot Windows from eMMC. To boot a device from eMMC, you first need to flash a Windows image to eMMC. Since eMMC is soldered to the board, the only way to write to it is to boot some kind of manufacturing OS on the device, then write the image from this manufacturing OS. The manufacturing OS is booted from removable storage such as USB or SD. In this document, we will walk through the process of booting Windows from eMMC on an HummingBoard as an example of an ARM32 board. We will also list commands for an MCIMX8M-EVK board to show an example of an ARM64 board. The tools and techniques can be adapted to other hardware designs.

For the manufacturing OS, we will use [Windows PE (WinPE)](https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/winpe-intro), which stands for Windows Preinstallation Environment. Windows PE is a small Windows image that can boot without persistent storage, and has tools to help install Windows such as `diskpart` and `dism`.

The high-level process we will follow is,
1. Identify where the bootloader will live
1. Prepare an FFU to be flashed to eMMC
1. Prepare a WinPE image, which will flash the FFU to eMMC

# Identifying boot loader location

First, we need to identify where the bootloader will live. i.MX6/7 can boot from a number of sources including eMMC/SD, NOR flash, SPI, I2C, and USB. For more information about i.MX6/7 early boot, see [Firmware Boot Documentation](boot.md) and the "System Boot" chapter of the processor reference manual. The reference BSP only implements boot from SD card. It is possible to port the reference material to boot from a different source, though for this document we will assume the initial boot device is SD. The initial boot device is set by processor fuses.

To avoid bricking your device, we recommend putting the first stage bootloader on media that can be reprogrammed via external means if necessary, such as an SD card, SPI flash with external programing circuitry, or I2C flash with external programming circuitry. By external programming circuitry, we mean that you can write new contents to the storage device without booting the i.MX processor.

Another strategy is to place the bootloader on eMMC and have a second, read-only eMMC part containing a recovery boot image which can be selected via hardware muxing. This way, if the primary eMMC part becomes corrupted, you can press a button or connect a jumper and boot the device from the backup eMMC part, which then allows you to recover the main eMMC part.

For HummingBoard, we will place the firmware on an SD card and the main OS on eMMC. This will allow us to recover the device if the bootloader or OS somehow becomes corrupted, by removing and reprogramming the SD card.

# Prepare an FFU to be flashed to eMMC

The FFU that gets flashed to MMC does not have any special requirements. If the firmware is going to be located on a different storage device, it does not need to contain the firmware packages. For HummingBoard, we can use the same FFU that gets flashed to the SD card.

# Creating the WinPE Image

We need to create an image that can boot off removable media, does not require persistent storage, and contains dism.exe, which is the tool that writes an FFU to storage. WinPE is designed for this purpose. To create a bootable WinPE image, we need to
1. inject i.MX drivers into the image
1. copy the WinPE image to an SD card
1. copy firmware to the SD card

The script [make-winpe.cmd](../build/tools/make-winpe.cmd) does all of the above for an ARM32 board and can configure the WinPE image to flash an FFU to a storage device at boot. For an i.MX8 board use [make-winpe-i.MX8.cmd](../build/tools/make-winpe-i.MX8.cmd) instead.

You must install the following software:

1. [ADK for Windows 10](https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/download-winpe--windows-pe)
1. [Windows PE add-on for the ADK](https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/download-winpe--windows-pe)
1. [dd for windows](http://www.chrysocome.net/dd), which must be placed on your PATH or in the current directory

First, we create a WinPE image on our machine. In this example, we specify the `/ffu` option because we want to deploy an FFU to eMMC. You must have already built the FFU. The script must be run as administrator, and it will create files in the directory in which it runs.

```cmd
mkdir winpe
cd winpe
:: For HummingBoard run:
path/to/imx-iotcore/build/tools/make-winpe.cmd /builddir d:\build\Binaries\release\ARM /firmware d:\build\FFU\HummingBoardEdge_iMX6Q_2GB\Package\BootLoader\firmware_fit.merged /uefi d:\build\FFU\HummingBoardEdge_iMX6Q_2GB\Package\BootFirmware\uefi.fit /ffu d:\build\FFU\HummingBoardEdge_iMX6Q_2GB\HummingBoardEdge_iMX6Q_2GB_TestOEMInput.xml.Release.ffu
:: For MCIMX8M-EVK run:
path/to/imx-iotcore/build/tools/make-winpe-i.MX8.cmd /builddir d:\build\Binaries\release\ARM64 /firmware d:\build\FFU\NXPEVK_iMX8M_4GB\Package\BootLoader\firmware_fit.merged /uefi d:\build\FFU\NXPEVK_iMX8M_4GB\Package\BootFirmware\uefi.fit /ffu d:\build\FFU\NXPEVK_iMX8M_4GB\NXPEVK_iMX8M_4GB_TestOEMInput.xml.Release.ffu
```

Then, we apply the image to an SD card. Insert an SD card into your machine, then determine the physical disk number by running

```cmd
diskpart
> list disk
> exit
```

The output will look something like,

```
DISKPART> list disk

  Disk ###  Status         Size     Free     Dyn  Gpt
  --------  -------------  -------  -------  ---  ---
  Disk 0    Online          931 GB      0 B        *
  Disk 1    Online          931 GB      0 B
  Disk 2    Online          953 GB      0 B        *
  Disk 3    No Media           0 B      0 B
  Disk 4    No Media           0 B      0 B
  Disk 5    No Media           0 B      0 B
  Disk 6    No Media           0 B      0 B
* Disk 7    Online           14 GB      0 B
```

In this example, the physical disk number is 7.

We now apply the WinPE image to the SD card.

```cmd
make-winpe.cmd /apply 7
```

It will format the SD card, copy the WinPE image contents, and write the firmware to the reserved sector at the beginning of the card.

You can now insert the SD card in your HummingBoard and boot. It will boot into WinPE, then flash the FFU to eMMC, then reboot. Before rebooting, it renames the `EFI` folder at the root of the SD card to `_efi`, which causes UEFI to skip the SD card when it's looking for a filesystem to boot from. It will find the `EFI` directory on eMMC instead, and boot from there. If you wish to boot into WinPE again, you can rename `_efi` back to `EFI`.

