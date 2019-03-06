Building and Updating Firmware
==============

This document describes how to set up a build environment to build the latest firmware, update the firmware on the SD Card for testing and development, and include the new firmware in the FFU builds.

## Setting up your build environment

1) Set up a Linux environment.
    * Dedicated Linux system
    * Linux Virtual Machine
    * Windows Subsystem for Linux ([WSL setup instructions](https://docs.microsoft.com/en-us/windows/wsl/install-win10))
    * Note: We validate with both Ubuntu in WSL and standalone Ubuntu machines.

1) Update and install build tools
    ```bash
    $ sudo apt-get update
    $ sudo apt-get upgrade
    $ sudo apt-get install build-essential python python-dev python-crypto python-wand device-tree-compiler bison flex swig iasl uuid-dev wget git bc libssl-dev
    $ pushd ~
    $ wget https://releases.linaro.org/components/toolchain/binaries/6.4-2017.11/arm-linux-gnueabihf/gcc-linaro-6.4.1-2017.11-x86_64_arm-linux-gnueabihf.tar.xz
    $ tar xf gcc-linaro-6.4.1-2017.11-x86_64_arm-linux-gnueabihf.tar.xz
    $ rm gcc-linaro-6.4.1-2017.11-x86_64_arm-linux-gnueabihf.tar.xz
    $ popd
    ```

1) Clone all repositories into the same directory as the imx-iotcore repo.
    ```bash
    $ git clone --recursive https://github.com/ms-iot/imx-iotcore.git
    $ git clone --recursive -b u-boot-imx https://github.com/ms-iot/u-boot.git
    $ git clone -b ms-iot https://github.com/ms-iot/optee_os.git
    $ git clone --recursive -b tcps-feature https://github.com/Microsoft/RIoT.git
    $ git clone -b imx https://github.com/ms-iot/imx-edk2-platforms.git
    $ git clone --recursive https://github.com/tianocore/edk2
    ```
    Optionally, clone the TPM reference implementation (`imx-edk2-platforms` includes a precompiled TPM binary)
    ```bash
    $ git clone --recursive https://github.com/Microsoft/ms-tpm-20-ref
    $ pushd ms-tpm-20-ref; git checkout 65b65354c6cce3212d9c512ec3ae2e23fe37c94d; popd
    ```

1) Download and extract the [Code Signing Tools (CST)](https://www.nxp.com/webapp/sps/download/license.jsp?colCode=IMX_CST_TOOL) from NXP's website. You will need to create an account on NXP's website to access this tool. Extract the tool to the same directory as all the above repositories, and rename the folder to cst:
    ```bash
    $ tar xf cst-3.1.0.tgz
    $ mv release cst
    $ rm cst-3.1.0.tgz
    ```

1) At this point your directory structure should look like the following
    ```
    - %WORKSPACE%
       |- cst
       |- edk2
       |- imx-edk2-platforms
       |- imx-iotcore
       |- ms-tpm-20-ref
       |- optee_os
       |- RIoT
       |- u-boot
    ```

1) Build firmware to test the setup. Adding "-j 20" to make will parallelize the build and speed it up significantly on WSL, but since the firmwares build in parallel it will be more difficult to diagnose any build failures. You can customize the number to work best with your system.
    ```bash
    $ cd imx-iotcore/build/firmware/HummingBoardEdge_iMX6Q_2GB
    $ make
    ```

1) After a successful build you should have several output files:
    ```bash
    firmware_fit.merged - Contains SPL, OP-TEE, and U-Boot proper
    uefi.fit - Contains the UEFI firmware
    ```

## Adding updated firmware to your FFU
1) To make the updated firmware a part of your FFU build, you must copy the firmwares to your board's Package folder in imx-iotcore.
 * Copy uefi.fit into /board/boardname/Package/BootFirmware
 * Copy firmware_fit.merged into /board/boardname/Package/BootLoader

2) When preparing to commit your changes, you should use the following make command save your OP-TEE SDK and the commit versions of your firmware automatically in your board folder.

    ```bash
    $ make update-ffu
    ```

## Deploying firmware to an SD card manually

### Bootable Firmware without installing an FFU
If you want to rapidly deploy and test firmware configurations without needing the full Windows boot, you can prepare an SD card manually to boot only the firmware stages.

The SD card must have two partitions in the following order:
* 4MB partition at the start of the disk, no filesystem. This is where U-Boot and OP-TEE get deployed.
* 50MB partition formatted fat32, optionally labeled efi. This is where UEFI gets deployed.

Here are the steps to run in an admin CMD to prepare an SD card in Windows:
  ```bat
  powershell Get-WmiObject Win32_DiskDrive
  REM Find the SD card in that list and use the number after PhysicalDrive as your disk number.
  diskpart
  list disk
  sel disk <#>
  list part
  REM Check the partitions to make sure this disk is actually your SD card.
  clean
  create partition primary size=4
  create partition primary size=50
  format quick fs=fat32 label=EFI
  assign
  list vol
  exit
  ```

### Deploying U-Boot and OP-TEE (firmware_fit.merged) for development
  On Windows you can use [DD for Windows](http://www.chrysocome.net/dd) from an administrator command prompt to deploy firmware_fit.merged.
  Be careful that you use the correct `of` and `seek`, DD will not ask for confirmation.

  ```bash
  powershell Get-WmiObject Win32_DiskDrive
  dd if=firmware_fit.merged of=\\.\PhysicalDrive<X> bs=512 seek=2
  ```
  Where `PhysicalDrive<X>` is the DeviceID of your SD card as shown by `Get-WmiObject`.

You might get the output: `Error reading file: 87 The parameter is incorrect`. This error can be ignored.

If you are working on a dedicated Linux machine (not WSL or VM) use:
```
dd if=firmware_fit.merged of=/dev/sdX bs=512 seek=2
```

### Deploying UEFI (uefi.fit) for development
Copy `uefi.fit` over to the EFI partition on your SD card.
