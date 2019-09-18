Building and Updating ARM64 Firmware
==============

This document describes how to set up a build environment to build the latest firmware, update the firmware on the SD Card for testing and development, and include the new firmware in the FFU builds.

Note: The UEFI build environment has changed for 1903 and any existing build environment must be updated.

## Setting up your build environment

1) Set up a Linux environment.
    * Dedicated Linux system
    * Linux Virtual Machine
    * Windows Subsystem for Linux ([WSL setup instructions](https://docs.microsoft.com/en-us/windows/wsl/install-win10))
    * Note: We validate with both Ubuntu in WSL and standalone Ubuntu machines.

1) Update and install build tools
    ```bash
    sudo apt-get update
    sudo apt-get upgrade
    sudo apt-get install attr build-essential python python-dev python-crypto python-wand device-tree-compiler bison flex swig iasl uuid-dev wget git bc libssl-dev zlib1g-dev python3-pip
    *** new for 1903 UEFI
    sudo apt-get install gcc g++ make python3 mono-devel
    ***
    pushd ~
    wget https://releases.linaro.org/components/toolchain/binaries/7.2-2017.11/aarch64-linux-gnu/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu.tar.xz
    tar xf gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu.tar.xz
    rm gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu.tar.xz
    popd
    ```

1) Clone all repositories into the same directory as the imx-iotcore repo.
    ```bash
    git clone --recursive https://github.com/ms-iot/imx-iotcore.git
    git clone --recursive https://github.com/ms-iot/mu_platform_nxp
    git clone -b imx_v2018.03_4.14.98_2.0.0_ga https://github.com/ms-iot/u-boot.git
    git clone -b imx_4.14.98_2.0.0_ga https://github.com/ms-iot/optee_os.git
    git clone -b imx_4.14.98_2.0.0_ga https://source.codeaurora.org/external/imx/imx-atf
    git clone -b imx_4.14.98_2.0.0_ga https://source.codeaurora.org/external/imx/imx-mkimage
    ```
    Optionally, clone the security TA repo (`mu_platform_nxp` includes precompiled TA binaries)
    ```bash
    git clone https://github.com/Microsoft/MSRSec
    ```

1) Ensure that case sensitivity is turned on for the u-boot directory when building in Windows.
   ```bash
   setfattr -n system.wsl_case_sensitive -v 1 u-boot
   ```

1) Download and extract the [Code Signing Tools (CST)](https://www.nxp.com/webapp/sps/download/license.jsp?colCode=IMX_CST_TOOL) from NXP's website. You will need to create an account on NXP's website to access this tool. Extract the tool to the same directory as all the above repositories, and rename the folder to cst:
    ```bash
    tar xf cst-3.1.0.tgz
    mv release cst
    rm cst-3.1.0.tgz
    ```

1) Download and extract the [iMX firmware](https://www.nxp.com/lgfiles/NMG/MAD/YOCTO/firmware-imx-7.9.bin) from NXP's website. This retrieves a self extracting shell script that provides HDMI firmware and DDR training firmware. 
    ```bash
    wget https://www.nxp.com/lgfiles/NMG/MAD/YOCTO/firmware-imx-8.1.bin
    bash ./firmware-imx-8.1.bin
    ````

1) At this point your directory structure should look like the following
    ```
    - %WORKSPACE%
    
       |- MSRSec
       |- cst
       |- firmware-imx-8.1
       |- imx-atf
       |- imx-iotcore
       |- imx-mkimage
       |- mu_platform_nxp
       |- optee_os
       |- u-boot
      ```

1) A makefile is available which automates the build process.
    ```bash
    cd imx-iotcore/build/firmware
    # Build firmware components for a specific board, but don't update the packages
    make -f imx8.mk IMX8_TARGET=NXPEVK_iMX8M_4GB CROSS_COMPILE=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu- imx8_build
    ```
    To update the binaries in the FFU packages:
    ```bash
    cd imx-iotcore/build/firmware
    # Build AND update the packages for a specific board
    make -f imx8.mk IMX8_TARGET=NXPEVK_iMX8M_4GB CROSS_COMPILE=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu- imx8_update-ffu
    ```
    Build/update all i.MX8 boards:
    ```bash
    cd imx-iotcore/build/firmware
    # Build each firmware (Note: will override the outputs)    
    make imx8_all
    # Update the firmware binaries in the package folders (requires re-build)
    make imx8_update-ffu
    # Stage the firmware for a commit (requires re-build)
    make imx8_commit-firmware
    ```

# Manually building firmware for iMX8M Quad EVK
1) Build the iMX8M EVK firmware. Adding "-j 20" to make will parallelize the build and speed it up significantly on WSL, but since the firmware build in parallel it will be more difficult to diagnose any build failures. You can customize the number to work best with your system.
   ```bash
   # U-Boot
   export CROSS_COMPILE=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
   export ARCH=arm64

   pushd u-boot
   make imx8mq_evk_nt_defconfig
   make
   popd


   # Arm Trusted Firmware

   export CROSS_COMPILE=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
   export ARCH=arm64

   pushd imx-atf
   make PLAT=imx8mq SPD=opteed bl31
   popd

   # OP-TEE OS

   export -n CROSS_COMPILE
   export -n ARCH
   export CROSS_COMPILE64=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
   pushd optee_os

   make PLATFORM=imx PLATFORM_FLAVOR=mx8mqevk \
     CFG_TEE_CORE_DEBUG=n CFG_TEE_CORE_LOG_LEVEL=2  CFG_UART_BASE=0x30890000 \
     CFG_RPMB_FS=y CFG_RPMB_TESTKEY=y CFG_RPMB_WRITE_KEY=y CFG_REE_FS=n  \
     CFG_IMXCRYPT=y CFG_CORE_HEAP_SIZE=131072

   # debug
   # make PLATFORM=imx PLATFORM_FLAVOR=mx8mqevk \
   #  CFG_TEE_CORE_DEBUG=y CFG_TEE_CORE_LOG_LEVEL=3  CFG_UART_BASE=0x30890000 \
   #  CFG_RPMB_FS=y CFG_RPMB_TESTKEY=y CFG_RPMB_WRITE_KEY=y CFG_REE_FS=n \
   #  CFG_TA_DEBUG=y CFG_TEE_CORE_TA_TRACE=1 CFG_TEE_TA_LOG_LEVEL=2 \
   #  CFG_IMXCRYPT=y CFG_CORE_HEAP_SIZE=131072

   ${CROSS_COMPILE64}objcopy -O binary ./out/arm-plat-imx/core/tee.elf ./out/arm-plat-imx/tee.bin
   popd

   # OP-TEE Trusted Applications

   export TA_DEV_KIT_DIR=../../../../optee_os/out/arm-plat-imx/export-ta_arm64
   export TA_CROSS_COMPILE=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
   export TA_CPU=cortex-a53

   pushd MSRSec/TAs/optee_ta
   make CFG_ARM64_ta_arm64=y CFG_FTPM_USE_WOLF=y CFG_AUTHVARS_USE_WOLF=y

   # debug
   # CFG_TEE_TA_LOG_LEVEL=4 CFG_TA_DEBUG=y make

   popd

   cp MSRSec/TAs/optee_ta/out/AuthVars/2d57c0f7-bddf-48ea-832f-d84a1a219301.ta  mu_platform_nxp/Microsoft/OpteeClientPkg/Bin/AuthvarsTa/Arm64/Test/
   cp MSRSec/TAs/optee_ta/out/AuthVars/2d57c0f7-bddf-48ea-832f-d84a1a219301.elf mu_platform_nxp/Microsoft/OpteeClientPkg/Bin/AuthvarsTa/Arm64/Test/

   cp MSRSec/TAs/optee_ta/out/fTPM/bc50d971-d4c9-42c4-82cb-343fb7f37896.ta mu_platform_nxp/Microsoft/OpteeClientPkg/Bin/fTpmTa/Arm64/Test/
   cp MSRSec/TAs/optee_ta/out/fTPM/bc50d971-d4c9-42c4-82cb-343fb7f37896.elf mu_platform_nxp/Microsoft/OpteeClientPkg/Bin/fTpmTa/Arm64/Test/


   # Imx-mkimage

   export CROSS_COMPILE=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
   export ARCH=arm64

   pushd imx-mkimage/iMX8M

   cp ../../firmware-imx-8.1/firmware/ddr/synopsys/lpddr4_pmu_train_*.bin .
   cp ../../firmware-imx-8.1/firmware/hdmi/cadence/signed_hdmi_imx8m.bin .
   cp ../../optee_os/out/arm-plat-imx/tee.bin .
   cp ../../imx-atf/build/imx8mq/release/bl31.bin .
   cp ../../u-boot/u-boot-nodtb.bin  .
   cp ../../u-boot/spl/u-boot-spl.bin .
   cp ../../u-boot/arch/arm/dts/fsl-imx8mq-evk.dtb .
   cp ../../u-boot/tools/mkimage .

   mv mkimage mkimage_uboot

   cd ..
   make SOC=iMX8M flash_hdmi_spl_uboot

   popd

   # UEFI
   # note: On Windows Ubuntu, ignore Python errors during build specifically like 
   # "ERROR - Please upgrade Python! Current version is 3.6.7. Recommended minimum is 3.7."

   # setup
   pushd mu_platform_nxp
   export GCC5_AARCH64_PREFIX=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
   pip3 install -r requirements.txt --upgrade

   python3 NXP/MCIMX8M_EVK_4GB/PlatformBuild.py --setup
   # if error here about NugetDependency.global_cache_path, then make sure mono-devel package is installed
   # using apt-get as listed in "Update and install build tools" above.

   cd MU_BASECORE
   make -C BaseTools
   cd ..

   popd

   # clean
   pushd mu_platform_nxp
   rm -r Build
   rm -r Config
   popd

   # build
   pushd mu_platform_nxp


   export GCC5_AARCH64_PREFIX=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-

   python3 NXP/MCIMX8M_EVK_4GB/PlatformBuild.py -V TARGET=RELEASE \
     PROFILE=DEV MAX_CONCURRENT_THREAD_NUMBER=20

   # debug
   # python3 NXP/MCIMX8M_EVK_4GB/PlatformBuild.py -V TARGET=DEBUG \
   #   PROFILE=DEV MAX_CONCURRENT_THREAD_NUMBER=20

   cd Build/MCIMX8M_EVK_4GB/RELEASE_GCC5/FV
   cp ../../../../../imx-iotcore/build/firmware/its/uefi_imx8_unsigned.its .
   ../../../../../u-boot/tools/mkimage -f uefi_imx8_unsigned.its -r uefi.fit

   popd
    ```
    
1) After a successful build you should have several output files:
    ```bash
    imx-mkimage/iMX8M/flash.bin - Contains SPL, ATF, OP-TEE, and U-Boot proper
    mu_platform_nxp/Build/MCIMX8M_EVK_4GB/RELEASE_GCC5/FV/uefi.fit - Contains the UEFI firmware
    ```

## Adding updated firmware to your ARM64 FFU
1) To make the updated firmware a part of your FFU build, you must copy the firmwares to your board's Package folder in imx-iotcore.
 * Copy uefi.fit into /board/boardname/Package/BootFirmware
 * Copy flash.bin into /board/boardname/Package/BootLoader
    ```bash
    cp imx-mkimage/iMX8M/flash.bin imx-iotcore/build/board/NXPEVK_iMX8M_4GB/Package/BootLoader/flash.bin
    cp mu_platform_nxp/Build/MCIMX8M_EVK_4GB/RELEASE_GCC5/FV/uefi.fit imx-iotcore/build/board/NXPEVK_iMX8M_4GB/Package/BootFirmware/uefi.fit
    ```


# Building firmware for iMX8M Mini EVK board
1) Building the iMX8M Mini EVK firmware is similar to the above iMX8M Quad EVK board with only a few minor changes to build parameters. Be careful to make sure you are building for the correct board.  Follow the instructions above for the iMX8M Quad EVK to setup and clone your build environment.

    ```bash
   # U-Boot
   export CROSS_COMPILE=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
   export ARCH=arm64

   pushd u-boot
   make imx8mm_evk_nt_defconfig
   make
   popd


   # Arm Trusted Firmware

   export CROSS_COMPILE=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
   export ARCH=arm64

   pushd imx-atf
   make PLAT=imx8mm SPD=opteed bl31
   popd

   # OP-TEE OS

   export -n CROSS_COMPILE
   export -n ARCH
   export CROSS_COMPILE64=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
   pushd optee_os

   make PLATFORM=imx PLATFORM_FLAVOR=mx8mmevk \
     CFG_TEE_CORE_DEBUG=n CFG_TEE_CORE_LOG_LEVEL=2 \
     CFG_RPMB_FS=y CFG_RPMB_TESTKEY=y CFG_RPMB_WRITE_KEY=y CFG_REE_FS=n  \
     CFG_IMXCRYPT=y CFG_CORE_HEAP_SIZE=131072

   # debug
   # make PLATFORM=imx PLATFORM_FLAVOR=mx8mmevk \
   #  CFG_TEE_CORE_DEBUG=y CFG_TEE_CORE_LOG_LEVEL=3 \
   #  CFG_RPMB_FS=y CFG_RPMB_TESTKEY=y CFG_RPMB_WRITE_KEY=y CFG_REE_FS=n \
   #  CFG_TA_DEBUG=y CFG_TEE_CORE_TA_TRACE=1 CFG_TEE_TA_LOG_LEVEL=2 \
   #  CFG_IMXCRYPT=y CFG_CORE_HEAP_SIZE=131072

   ${CROSS_COMPILE64}objcopy -O binary ./out/arm-plat-imx/core/tee.elf ./out/arm-plat-imx/tee.bin
   popd

   # OP-TEE Trusted Applications

   export TA_DEV_KIT_DIR=../../../../optee_os/out/arm-plat-imx/export-ta_arm64
   export TA_CROSS_COMPILE=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
   export TA_CPU=cortex-a53

   pushd MSRSec/TAs/optee_ta
   make CFG_ARM64_ta_arm64=y CFG_FTPM_USE_WOLF=y CFG_AUTHVARS_USE_WOLF=y

   # debug
   # CFG_TEE_TA_LOG_LEVEL=4 CFG_TA_DEBUG=y make

   popd

   cp MSRSec/TAs/optee_ta/out/AuthVars/2d57c0f7-bddf-48ea-832f-d84a1a219301.ta  mu_platform_nxp/Microsoft/OpteeClientPkg/Bin/AuthvarsTa/Arm64/Test/
   cp MSRSec/TAs/optee_ta/out/AuthVars/2d57c0f7-bddf-48ea-832f-d84a1a219301.elf mu_platform_nxp/Microsoft/OpteeClientPkg/Bin/AuthvarsTa/Arm64/Test/

   cp MSRSec/TAs/optee_ta/out/fTPM/53bab89c-b864-4d7e-acbc-33c07a9c1b8d.ta mu_platform_nxp/Microsoft/OpteeClientPkg/Bin/fTpmTa/Arm64/Test/
   cp MSRSec/TAs/optee_ta/out/fTPM/53bab89c-b864-4d7e-acbc-33c07a9c1b8d.elf mu_platform_nxp/Microsoft/OpteeClientPkg/Bin/fTpmTa/Arm64/Test/


   # Imx-mkimage

   export CROSS_COMPILE=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
   export ARCH=arm64

   pushd imx-mkimage/iMX8M

   cp ../../firmware-imx-8.1/firmware/ddr/synopsys/lpddr4_pmu_train_*.bin .
   cp ../../optee_os/out/arm-plat-imx/tee.bin .
   cp ../../imx-atf/build/imx8mm/release/bl31.bin .
   cp ../../u-boot/u-boot-nodtb.bin  .
   cp ../../u-boot/spl/u-boot-spl.bin .
   cp ../../u-boot/arch/arm/dts/fsl-imx8mm-evk.dtb .
   cp ../../u-boot/tools/mkimage .

   mv mkimage mkimage_uboot

   cd ..
   make SOC=iMX8MM flash_spl_uboot

   popd

   # UEFI
   # note: On Windows Ubuntu, ignore Python errors during build specifically like 
   # "ERROR - Please upgrade Python! Current version is 3.6.7. Recommended minimum is 3.7."

   # setup
   pushd mu_platform_nxp
   export GCC5_AARCH64_PREFIX=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
   pip3 install -r requirements.txt --upgrade

   python3 NXP/MCIMX8M_MINI_EVK_2GB/PlatformBuild.py --setup
   # if error here about NugetDependency.global_cache_path, then make sure mono-devel package is installed
   # using apt-get as listed in "Update and install build tools" above.

   cd MU_BASECORE
   make -C BaseTools
   cd ..

   popd

   # clean
   pushd mu_platform_nxp
   rm -r Build
   rm -r Config
   popd

   # build
   pushd mu_platform_nxp


   export GCC5_AARCH64_PREFIX=~/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-

   python3 NXP/MCIMX8M_MINI_EVK_2GB/PlatformBuild.py -V TARGET=RELEASE \
     PROFILE=DEV MAX_CONCURRENT_THREAD_NUMBER=20

   # debug
   # python3 NXP/MCIMX8M_MINI_EVK_2GB/PlatformBuild.py -V TARGET=DEBUG \
   #   PROFILE=DEV MAX_CONCURRENT_THREAD_NUMBER=20

   cd Build/MCIMX8M_MINI_EVK_2GB/RELEASE_GCC5/FV
   cp ../../../../../imx-iotcore/build/firmware/its/uefi_imx8_unsigned.its .
   ../../../../../u-boot/tools/mkimage -f uefi_imx8_unsigned.its -r uefi.fit

   popd
    ```
    
1) After a successful build you should have several output files:
    ```bash
    imx-mkimage/iMX8M/flash.bin - Contains SPL, ATF, OP-TEE, and U-Boot proper
    mu_platform_nxp/Build/MCIMX8M_MINI_EVK_2GB/RELEASE_GCC5/FV/uefi.fit - Contains the UEFI firmware
    ```

## Adding updated firmware to your ARM64 FFU
1) To make the updated firmware a part of your FFU build, you must copy the firmwares to your board's Package folder in imx-iotcore.
 * Copy uefi.fit into /board/boardname/Package/BootFirmware
 * Copy flash.bin into /board/boardname/Package/BootLoader
    ```bash
    cp imx-mkimage/iMX8M/flash.bin imx-iotcore/build/board/NXPEVK_iMX8M_MINI_2GB/Package/BootLoader/flash.bin
    cp mu_platform_nxp/Build/MCIMX8M_MINI_EVK_2GB/RELEASE_GCC5/FV/uefi.fit imx-iotcore/build/board/NXPEVK_iMX8M_MINI_2GB/Package/BootFirmware/uefi.fit
    ```

## Deploying firmware to an SD card manually

### Deploying U-Boot, ATF, OP-TEE (flash.bin) and UEFI (uefi.fit) for development
  On Windows you can use [DD for Windows](http://www.chrysocome.net/dd) from an administrator command prompt to deploy flash.bin and uefi.fit.
  Be careful that you use the correct `of` and `seek`, DD will not ask for confirmation.

  ```bash
  powershell Get-WmiObject Win32_DiskDrive
  dd if=flash.bin of=\\.\PhysicalDrive<X> bs=512 seek=66

  dd if=uefi.fit of=\\.\PhysicalDrive<X> bs=1024 seek=2176

  ```
  Where `PhysicalDrive<X>` is the DeviceID of your SD card as shown by `Get-WmiObject`.

You might get the output: `Error reading file: 87 The parameter is incorrect`. This error can be ignored.

If you are working on a dedicated Linux machine (not WSL or VM) use:
```
dd if=flash.bin of=/dev/sdX bs=512 seek=66

dd if=uefi.fit of=/dev/sdX bs=1024 seek=2176
```
