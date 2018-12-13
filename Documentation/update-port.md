Updating your BSP port
==========

Below are a list of changes that may have occured since any initial enablement of Windows 10 IoT Core on your i.MX platform.

## Reworked firmware build system
The firmware build system now builds entirely in WSL and Linux and uses a makefile as the only front-end. Please see the [Building and Updating Firmware](build-firmware.md) guide for more information on firmware build system setup and usage.

In order to use the Makefile, you will need to have a folder for your board in the firmware folder. To set up this firmware folder you can run NewiMX6Board.ps1 which is [documented here](newboard.md). Please note that this firmware folder name should match the EDK2-Platforms name for your board.

## FIT load for OP-TEE and U-Boot Proper inside of SPL
In order to use existing loading infrastructure, we have updated the way U-Boot proper and OP-TEE are packaged so that SPL can load and run them as a Flattened Image Tree. When built through the firmware Makefile the required `firmware_fit.merged` file will be generated if all of the features required for FIT in SPL are enabled in your U-Boot defconfig.

## FIT loading UEFI inside of U-Boot Proper
We have updated the way we run the UEFI firmware from U-Boot proper to use U-Boot's built in FIT boot path.
* The defconfig settings `CONFIG_UEFI_BOOT`, `CONFIG_UEFI_LOAD_ADDR`, and `CONFIG_UEFI_IMAGE_NAME` are no longer required and should be removed.
* The UEFI binary is no longer called `IMX6BOARD_EFI.fd` or `imxboard_efi.fd` on the EFI partition. Instead the UEFI is built into a Flattened Image Tree called `uefi.fit` and is stored on the EFI partition. The `uefi.fit` packaging is done by the `mkimage` tool and is done automatically when EDK2 is built through the firmware Makefile.
* The new boot path no longer uses a hardcoded UEFI BOOTCOMMAND, instead `CONFIG_BOOTCOMMAND` is customized directly in the defconfig. `CONFIG_USE_BOOTCOMMAND` and `CONFIG_CMD_BOOTM` must be enabled to make sure that the bootcommand is enabled, and that the `bootm` command is available.
* `CONFIG_IMX_PERSIST_INIT` has been added so that U-Boot proper does not disable the IPU before booting into UEFI, keeping the display on. UEFI assumes that the IPU is enabled and configured when the GOP driver loads.
  ```C
    CONFIG_USE_BOOTCOMMAND=y
    CONFIG_BOOTCOMMAND="fatload mmc 0:2 0x80A20000 /uefi.fit; bootm 0x80A20000"
    CONFIG_CMD_BOOTM=y
    CONFIG_IMX_PERSIST_INIT=y
  ```

* Some board header files statically define a CONFIG_BOOTCOMMAND, which will conflict with the one set in the defconfig. If your board header has a #define CONFIG_BOOTCOMMAND, then you should wrap it in an #if !defined block like below:
  ```C
  #if !defined(CONFIG_BOOTCOMMAND)
  #define CONFIG_BOOTCOMMAND \
          "run findfdt;" \
          ...
          "else run netboot; fi"
  #endif /* !defined(CONFIG_BOOTCOMMAND) */
  ```

* If you previously disabled CONFIG_DISTRO_DEFAULTS, you may need to reenable it to pull in dependencies for bootm's FIT boot: `CONFIG_DISTRO_DEFAULTS=y`

## Storing the MAC address from U-Boot for use in UEFI
Because the hardcoded UEFI boot path is no longer used, we needed an alternate method to store the MAC addresses from the U-Boot environment for later use. To do this we added a globalpage command to U-Boot, which performs the operations that were previously hardcoded.
* The command must be enabled with `CONFIG_CMD_GLOBALPAGE=y`.
* The globalpage commands must be added to your `CONFIG_BOOTCOMMAND`.
* Most i.MX6 platforms should use `globalpage init 0x10817000`. i.MX6SX and i.MX7 should use `globalpage init 0x82003000`.
* To store the MAC address use `globalpage add ethaddr`, to store a second MAC address use `globalpage add ethaddr1`.
  ```C
    CONFIG_BOOTCOMMAND="globalpage init 0x10817000; globalpage add ethaddr; fatload mmc 0:2 0x80A20000 /uefi.fit; bootm 0x80A20000"
    CONFIG_CMD_GLOBAL_PAGE=y
  ```

## Miscelaneous U-Boot defconfig settings

* `CONFIG_BOOTDELAY=-2` boots the CONFIG_BOOTCOMMAND without delay or checking serial input to interrupt. This is important because WinDBG will interrupt boot if U-Boot checks serial input.

* The list of additional U-Boot options used when booting Windows is available here: [U-Boot Configuration Options](porting-imx.md#u-boot-configuration-options)
