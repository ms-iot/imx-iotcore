.ONESHELL:
SHELL = bash
.SHELLFLAGS = -ec

CROSS_COMPILE ?=$(HOME)/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
PYTHON3?=python3
ARCH=arm64
VERSIONS=firmwareversions.log

ifeq ($(IMX8_TARGET),NXPEVK_iMX8M_4GB)
U_BOOT_CONFIG=imx8mq_evk_nt_defconfig
U_BOOT_DTS=fsl-imx8mq-evk
U_BOOT_SOC=iMX8M
ATF_CONFIG=imx8mq
EDK2_PLATFORM=MCIMX8M_EVK_4GB
OPTEE_PLATFORM=mx8mqevk
endif

ifeq ($(IMX8_TARGET),NXPEVK_iMX8M_Mini_2GB)
U_BOOT_CONFIG=imx8mm_evk_nt_defconfig
U_BOOT_DTS=fsl-imx8mm-evk
U_BOOT_SOC=iMX8MM
ATF_CONFIG=imx8mm
EDK2_PLATFORM=MCIMX8M_MINI_EVK_2GB
OPTEE_PLATFORM=mx8mmevk
endif

ifeq (,$(IMX8_TARGET))
$(error IMX8_TARGET not set)
endif

IMX8_REPO_ROOT?=$(abspath ../../../)

.PHONY: imx8_u-boot imx8_optee imx8_atf imx8_uefi imx8_tas imx8_mkimage imx8_update-ffu imx8_build $(VERSIONS)

imx8_u-boot:
	cd $(IMX8_REPO_ROOT)/u-boot
	export ARCH=$(ARCH)
	export CROSS_COMPILE=$(CROSS_COMPILE)
	
	$(MAKE) $(U_BOOT_CONFIG) 
	$(MAKE)
	
imx8_atf:
	cd $(IMX8_REPO_ROOT)/imx-atf
	export ARCH=$(ARCH)
	export CROSS_COMPILE=$(CROSS_COMPILE)
	rm -rf build
	$(MAKE) PLAT=$(ATF_CONFIG) SPD=opteed bl31
	
imx8_optee:
	cd $(IMX8_REPO_ROOT)/optee_os
ifeq ($(IMX8_TARGET),NXPEVK_iMX8M_4GB)
	$(MAKE) PLATFORM=imx PLATFORM_FLAVOR=$(OPTEE_PLATFORM) \
     CFG_TEE_CORE_DEBUG=n CFG_TEE_CORE_LOG_LEVEL=2  CFG_UART_BASE=0x30890000 \
     CFG_RPMB_FS=y CFG_RPMB_TESTKEY=y CFG_RPMB_WRITE_KEY=y CFG_REE_FS=n  \
     CFG_IMXCRYPT=y CFG_CORE_HEAP_SIZE=131072 \
	 CROSS_COMPILE64=$(CROSS_COMPILE) \
	 CROSS_COMPILE= \

else
	$(MAKE) PLATFORM=imx PLATFORM_FLAVOR=$(OPTEE_PLATFORM) \
     CFG_TEE_CORE_DEBUG=n CFG_TEE_CORE_LOG_LEVEL=2 \
     CFG_RPMB_FS=y CFG_RPMB_TESTKEY=y CFG_RPMB_WRITE_KEY=y CFG_REE_FS=n  \
     CFG_IMXCRYPT=y CFG_CORE_HEAP_SIZE=131072 \
	 CROSS_COMPILE64=$(CROSS_COMPILE) \
	 CROSS_COMPILE= \

endif
	 
	$(CROSS_COMPILE)objcopy -O binary ./out/arm-plat-imx/core/tee.elf ./out/arm-plat-imx/tee.bin
	
imx8_tas: imx8_optee
	pushd $(IMX8_REPO_ROOT)/MSRSec/TAs/optee_ta
	$(MAKE) -j1 CFG_ARM64_ta_arm64=y CFG_FTPM_USE_WOLF=y CFG_AUTHVARS_USE_WOLF=y CFG_TEE_TA_LOG_LEVEL=4 CFG_TA_DEBUG=y \
	TA_DEV_KIT_DIR=../../../../optee_os/out/arm-plat-imx/export-ta_arm64 \
	TA_CROSS_COMPILE=$(CROSS_COMPILE) \
	TA_CPU=cortex-a53 \
	
	popd
	cp $(IMX8_REPO_ROOT)/MSRSec/TAs/optee_ta/out/AuthVars/2d57c0f7-bddf-48ea-832f-d84a1a219301.ta  $(IMX8_REPO_ROOT)/mu_platform_nxp/Microsoft/OpteeClientPkg/Bin/AuthvarsTa/Arm64/Test/
	cp $(IMX8_REPO_ROOT)/MSRSec/TAs/optee_ta/out/AuthVars/2d57c0f7-bddf-48ea-832f-d84a1a219301.elf $(IMX8_REPO_ROOT)/mu_platform_nxp/Microsoft/OpteeClientPkg/Bin/AuthvarsTa/Arm64/Test/
	cp $(IMX8_REPO_ROOT)/MSRSec/TAs/optee_ta/out/fTPM/bc50d971-d4c9-42c4-82cb-343fb7f37896.ta $(IMX8_REPO_ROOT)/mu_platform_nxp/Microsoft/OpteeClientPkg/Bin/fTpmTa/Arm64/Test/
	cp $(IMX8_REPO_ROOT)/MSRSec/TAs/optee_ta/out/fTPM/bc50d971-d4c9-42c4-82cb-343fb7f37896.elf $(IMX8_REPO_ROOT)/mu_platform_nxp/Microsoft/OpteeClientPkg/Bin/fTpmTa/Arm64/Test/

IMX8_MKIMAGE_DEPS_DDR=$(wildcard $(IMX8_REPO_ROOT)/firmware-imx-8.1/firmware/ddr/synopsys/lpddr4_pmu_train_*.bin)
IMX8_MKIMAGE_DEPS_HDMI=$(wildcard $(IMX8_REPO_ROOT)/firmware-imx-8.1/firmware/hdmi/cadence/signed_hdmi_imx8m.bin)
IMX8_MKIMAGE_DEPS_OPTEE=$(IMX8_REPO_ROOT)/optee_os/out/arm-plat-imx/tee.bin
IMX8_MKIMAGE_DEPS_ATF=$(IMX8_REPO_ROOT)/imx-atf/build/$(ATF_CONFIG)/release/bl31.bin
IMX8_MKIMAGE_DEPS_U-BOOT=$(IMX8_REPO_ROOT)/u-boot/u-boot-nodtb.bin $(IMX8_REPO_ROOT)/u-boot/spl/u-boot-spl.bin $(IMX8_REPO_ROOT)/u-boot/arch/arm/dts/$(U_BOOT_DTS).dtb $(IMX8_REPO_ROOT)/u-boot/tools/mkimage

imx8_mkimage: imx8_optee imx8_u-boot imx8_atf $(IMX8_MKIMAGE_DEPS_DDR) $(IMX8_MKIMAGE_DEPS_HDMI)
	cd $(IMX8_REPO_ROOT)/imx-mkimage/iMX8M
	rm -f *.bin *.dtb
	cp $(IMX8_MKIMAGE_DEPS_DDR) .
ifeq ($(IMX8_TARGET),NXPEVK_iMX8M_4GB)
	cp $(IMX8_MKIMAGE_DEPS_HDMI) .
endif
	cp $(IMX8_MKIMAGE_DEPS_OPTEE) .
	cp $(IMX8_MKIMAGE_DEPS_ATF) .
	cp $(IMX8_MKIMAGE_DEPS_U-BOOT) .
	mv mkimage mkimage_uboot	
	cd ..
ifeq ($(IMX8_TARGET),NXPEVK_iMX8M_Mini_2GB)
	$(MAKE) SOC=$(U_BOOT_SOC) flash_spl_uboot
else
	$(MAKE) SOC=$(U_BOOT_SOC) flash_hdmi_spl_uboot
endif

imx8_uefi: imx8_u-boot imx8_tas its/uefi_imx8_unsigned.its
	export GCC5_AARCH64_PREFIX=$(CROSS_COMPILE)
	
	cd $(IMX8_REPO_ROOT)/mu_platform_nxp
	$(PYTHON3) -m pip install -r requirements.txt --upgrade
	$(PYTHON3) NXP/$(EDK2_PLATFORM)/PlatformBuild.py --setup

	cd MU_BASECORE
	$(MAKE) -C BaseTools 
	cd ..
	
	$(PYTHON3) NXP/$(EDK2_PLATFORM)/PlatformBuild.py -V TARGET=RELEASE \
     PROFILE=DEV MAX_CONCURRENT_THREAD_NUMBER=20
	
	cd Build/$(EDK2_PLATFORM)/RELEASE_GCC5/FV
	cp $(CURDIR)/its/uefi_imx8_unsigned.its .
	$(IMX8_REPO_ROOT)/u-boot/tools/mkimage -f uefi_imx8_unsigned.its -r uefi.fit

imx8_build: imx8_uefi imx8_mkimage

VERSION_REPOS= \
    imx-iotcore \
    mu_platform_nxp \
    u-boot \
    optee_os \
    imx-atf \
    imx-mkimage \
	MSRSec

$(VERSIONS):
	@echo Logging version information of firmware repositories
	rm -f $@
	$(foreach REPO,$(VERSION_REPOS), \
	echo $(REPO) commit information: >> $@ && \
	pushd $(IMX8_REPO_ROOT)/$(REPO) && \
	  git config --get remote.origin.url >> $(CURDIR)/$@ && \
	  git rev-parse HEAD >> $(CURDIR)/$@ && \
	  echo $(REPO) diff information: >> $(CURDIR)/$@ && \
	  git diff HEAD >> $(CURDIR)/$@ && \
	  echo $(REPO) diff end >> $(CURDIR)/$@ && \
	popd; )

imx8_update-ffu: imx8_build $(VERSIONS)
	cp $(VERSIONS) $(CURDIR)/../board/$(IMX8_TARGET)/Package/BootLoader/
	cp $(IMX8_REPO_ROOT)/imx-mkimage/iMX8M/flash.bin $(CURDIR)/../board/$(IMX8_TARGET)/Package/BootLoader/flash.bin
	cp $(IMX8_REPO_ROOT)/mu_platform_nxp/Build/$(EDK2_PLATFORM)/RELEASE_GCC5/FV/uefi.fit $(CURDIR)/../board/$(IMX8_TARGET)/Package/BootFirmware/uefi.fit

imx8_commit-firmware: imx8_update-ffu
	git add $(CURDIR)/../board/$(IMX8_TARGET)/Package/BootLoader
	git add $(CURDIR)/../board/$(IMX8_TARGET)/Package/BootFirmware
	@echo "Successfully copied files to package and staged for commit"
