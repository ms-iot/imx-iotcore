# Set the make shell to bash and use one shell so EDK2 can set up its environment
# In ONESHELL only the first line of a rule can set the prefix.
.ONESHELL:
SHELL = bash
.SHELLFLAGS = -ec

# Paths to dependent repositories
REPO_ROOT=../../..
UBOOT_ROOT ?= $(REPO_ROOT)/../u-boot
OPTEE_ROOT ?= $(REPO_ROOT)/../optee_os
EDK2_PLATFORMS_ROOT ?= $(REPO_ROOT)/../imx-edk2-platforms
EDK2_ROOT ?= $(REPO_ROOT)/../edk2
FTPM_ROOT ?= $(REPO_ROOT)/../ms-tpm-20-ref/Samples/ARM32-FirmwareTPM/optee_ta/fTPM
CST_ROOT ?= $(REPO_ROOT)/../cst
KEY_ROOT = ../test_keys_no_security

# Firmware output directories
UBOOT_OUT=$(CURDIR)/u-boot
OPTEE_OUT=$(CURDIR)/optee_os
FTPM_OUT=$(CURDIR)/fTPM
RIOT_OUT=$(CURDIR)/RIoT
EDK2_OUT=$(REPO_ROOT)/../Build/$(EDK2_DSC)

# Paths to output binaries
SPL_PUB_KEYED=$(UBOOT_OUT)/spl/u-boot-spl-keyed.bin
UBOOT=$(UBOOT_OUT)/u-boot-ivt.img
OPTEE=$(OPTEE_OUT)/core/tee.bin
SPL_HAB=spl_HAB.imx
EDK2=$(EDK2_OUT)/$(EDK2_DEBUG_RELEASE)_GCC5/FV/IMXBOARD_EFI.fd

# Flat Image Trees
UBOOT_OPTEE_FIT=image.fit
UEFI_FIT=uefi.fit

VERSIONS=firmwareversions.log

# Toolchain checks to see if EDK2 Basetools or U-Boot need to build
EDK2BASETOOLS=$(REPO_ROOT)/../edk2/Conf/BuildEnv.sh
MKIMAGE=$(UBOOT_OUT)/tools/mkimage

# Signing parameters for mkimage FIT generation
UBOOT_OPTEE_KEY_PARAMS = -k $(KEY_ROOT) -r -K dt-spl.dtb
UEFI_KEY_PARAMS = -k $(KEY_ROOT) -r -K dt.dtb
SRKH_FUSE_BIN = ../test_keys_no_security/crts/SRK_1_2_3_4_fuse.bin

FTPM_FLAGS= \
	CFG_TEE_TA_LOG_LEVEL=2 \
	CFG_TA_DEBUG=y \

OPTEE_FLAGS= \
	CFG_PSCI_ARM32=y \
	CFG_REE_FS=n \
	CFG_RPMB_FS=y \
	CFG_RPMB_RESET_FAT=n \
	CFG_RPMB_WRITE_KEY=y \
	CFG_RPMB_TESTKEY=y \
	CFG_TA_HELLO_WORLD=n \
	CFG_TA_RPC=y \
	CFG_WITH_USER_TA=y \
	CFG_PAGED_USER_TA=n \
	CFG_WITH_PAGER=n \
	platform-cflags-optimization=-Os \
	CFG_TEE_CORE_DEBUG=n \
	CFG_TEE_CORE_LOG_LEVEL=2 \
	CFG_BOOT_SECONDARY_REQUEST=y \
	CFG_BOOT_SYNC_CPU=n \
	CFG_CYREP=y \
	CFG_SCTLR_ALIGNMENT_CHECK=n \
	CFG_DT=n \
	CFG_NS_ENTRY_ADDR= \

OPTEE_FLAGS_IMX6= \
	$(OPTEE_FLAGS) \
	CFG_SM_PLATFORM_HANDLER=y \
	CFG_PL310_SIP_PROTOCOL=y \
	CFG_PL310_LOCKED=n \
	CFG_TZC380=y \
	CFG_TZDRAM_START=0x10A00000 \
	CFG_TZDRAM_SIZE=0x01E00000 \
	CFG_SHMEM_START=0x12800000 \
	CFG_SHMEM_SIZE=0x00200000 \

OPTEE_FLAGS_IMX6ULL= \
	$(OPTEE_FLAGS) \
	CFG_PL310_SIP_PROTOCOL=y \
	CFG_PL310_LOCKED=n \
	CFG_TZDRAM_START=0x80A00000 \
	CFG_TZDRAM_SIZE=0x01E00000 \
	CFG_SHMEM_START=0x82800000 \
	CFG_SHMEM_SIZE=0x00200000 \

OPTEE_FLAGS_IMX6SX= \
	$(OPTEE_FLAGS) \
	CFG_SM_PLATFORM_HANDLER=y \
	CFG_PL310_SIP_PROTOCOL=y \
	CFG_PL310_LOCKED=n \
	CFG_TZDRAM_START=0x80000000 \
	CFG_TZDRAM_SIZE=0x01E00000 \
	CFG_SHMEM_START=0x81E00000 \
	CFG_SHMEM_SIZE=0x00200000 \

OPTEE_FLAGS_IMX7= \
	$(OPTEE_FLAGS) \
	CFG_SECONDARY_INIT_CNTFRQ=y \
	CFG_TZDRAM_START=0x80000000 \
	CFG_TZDRAM_SIZE=0x01E00000 \
	CFG_SHMEM_START=0x81E00000 \
	CFG_SHMEM_SIZE=0x00200000 \

export CROSS_COMPILE ?= $(HOME)/gcc-linaro-6.4.1-2017.11-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

export TA_CROSS_COMPILE=$$CROSS_COMPILE
export TA_DEV_KIT_DIR=$(OPTEE_OUT)/export-ta_arm32

# Capture the sector offset where SPL looks for the U-Boot/OPTEE Flat Image Tree
firmware_fit.merged: SPL_FIT_ADDRESS = $(shell grep "CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR " \
				$(UBOOT_OUT)/spl/u-boot.cfg | cut -d' ' -f 3)

# Concatenate SPL and the U-Boot/OP-TEE FIT into a single binary that will be flashed to sector 2 of the disk.
firmware_fit.merged: $(UBOOT_OPTEE_FIT) $(SPL_HAB)
	rm -f $@
	dd if=$(SPL_HAB) of=$@ bs=512 2>&1
	dd if=$(UBOOT_OPTEE_FIT) of=$@ bs=512 seek=$$(printf "%d - 2\n" ${SPL_FIT_ADDRESS} | bc) 2>&1
	echo Done creating unsigned merged image

# Build a Flat Image Tree for UEFI and U-Boot/OP-TEE after all individual firmwares have built
#  mkimage and the dtb files are used to create a verified boot chain where each FIT is signed.
#  and the public key for the signed FIT is stored in the DTB of the component that launches it.
#
# 1) mkimage builds and signs uefi.fit and stores the public key into the U-Boot DTB (dt.dtb)
# 2) u-boot-nodtb.bin is concatenated with dt.dtb into u-boot-keyed.bin
# 3) U-Boot and OP-TEE load addresses are scraped from build configuration files, and placed in a temp its
# 4) mkimage builds and signs image.fit and stores the public key into the SPL DTB (dt-spl.dtb)
# 5) SPL DTB is recombined with SPL in the $(SPL_PUB_KEYED) rule, which depends on $(UBOOT_OPTEE_FIT)

$(UBOOT_OPTEE_FIT): FIT_UBOOT_LOAD_ADDRESS = $(shell grep "CONFIG_SYS_TEXT_BASE" \
				$(UBOOT_OUT)/u-boot.cfg | cut -d' ' -f 3)
$(UBOOT_OPTEE_FIT): SPL_LOAD_ADDRESS = $(shell grep "CONFIG_SPL_TEXT_BASE" \
				$(UBOOT_OUT)/spl/u-boot.cfg | cut -d' ' -f 3)
$(UBOOT_OPTEE_FIT): FIT_OPTEE_LOAD_ADDRESS = $(shell cat $(OPTEE_OUT)/core/tee-init_load_addr.txt)
$(UBOOT_OPTEE_FIT): FIT_OPTEE_ENTRY_POINT = $(shell cat $(OPTEE_OUT)/core/tee-init_load_addr.txt)

$(UBOOT_OPTEE_FIT): $(UBOOT) $(OPTEE) uefi ../its/$(UBOOT_OPTEE_ITS) ../its/$(UEFI_ITS)
	@cp $(UBOOT_OUT)/dts/dt.dtb dt.dtb
	cp -f ../its/$(UEFI_ITS) $(UEFI_ITS)
	$(UBOOT_OUT)/tools/mkimage -f $(UEFI_ITS) $(UEFI_KEY_PARAMS) -r uefi.fit
	rm -f $(UEFI_ITS)
	cat $(UBOOT_OUT)/u-boot-nodtb.bin dt.dtb > $(UBOOT_OUT)/u-boot-keyed.bin
	rm -f dt.dtb
	rm -f $@
	cp -f ../its/$(UBOOT_OPTEE_ITS) uboot_optee.its.tmp
	sed -i -e 's/FIT_UBOOT_LOAD_ADDRESS/$(FIT_UBOOT_LOAD_ADDRESS)/g' uboot_optee.its.tmp
	sed -i -e 's/FIT_OPTEE_LOAD_ADDRESS/$(FIT_OPTEE_LOAD_ADDRESS)/g' uboot_optee.its.tmp
	sed -i -e 's/FIT_OPTEE_ENTRY_POINT/$(FIT_OPTEE_ENTRY_POINT)/g' uboot_optee.its.tmp
	echo "U-Boot load at:$(FIT_UBOOT_LOAD_ADDRESS)"
	echo "OPTEE load at:$(FIT_OPTEE_LOAD_ADDRESS)"
	cp u-boot/dts/dt-spl.dtb dt-spl.dtb
	$(UBOOT_OUT)/tools/mkimage -f uboot_optee.its.tmp $(UBOOT_OPTEE_KEY_PARAMS) -r $(UBOOT_OPTEE_FIT)
	rm -f uboot_optee.its.tmp

.PHONY: $(UBOOT) $(UBOOT_OUT)/SPL
$(UBOOT_OUT)/SPL: $(UBOOT)
$(UBOOT):
	CROSS_COMPILE=$(CROSS_COMPILE) ARCH=arm $(MAKE) -C $(UBOOT_ROOT) $(UBOOT_CONFIG) O=$(UBOOT_OUT)
	CROSS_COMPILE=$(CROSS_COMPILE) ARCH=arm $(MAKE) -C $(UBOOT_ROOT) O=$(UBOOT_OUT)


# $(SPL_HAB) is a bootable SPL binary with High Assurance Boot signing information concatenated to the end
$(SPL_HAB): SPL_LOAD_ADDRESS = $(shell grep "CONFIG_SPL_TEXT_BASE" \
				$(UBOOT_OUT)/spl/u-boot.cfg | cut -d' ' -f 3)

$(SPL_HAB): $(UBOOT_OPTEE_FIT) $(UBOOT) spl_csf.bin
	cat $(UBOOT_OUT)/SPL spl_csf.bin > $@

# spl_csf.bin is a signature of the SPL binary generated by NXP's Code Signing Tool for High Assurance Boot
spl_csf.bin: $(UBOOT_OPTEE_FIT) u-boot_sign.csf
	$(CST_ROOT)/linux64/bin/cst --o $@ --i u-boot_sign.csf \
	|| (echo "Code signing tool failed! Check that CST has been extracted to the same parent directory as all the firmware repositories into a folder called cst. Please see the instructions in the firmware building guide"; exit 1)

# Rebuild SPL image because $(SPL_PUB_KEYED) now contains the public key for the U-Boot/OP-TEE FIT
# Capture the blocks information from the new SPL.log to use in the Code Signing Tool input parameters
u-boot_sign.csf: $(UBOOT_OPTEE_FIT) ../u-boot_sign.csf-template $(SPL_PUB_KEYED)
	$(UBOOT_OUT)/tools/mkimage -n $(UBOOT_OUT)/spl/u-boot-spl.cfgout -T imximage -e $(SPL_LOAD_ADDRESS) -d $(SPL_PUB_KEYED) $(UBOOT_OUT)/SPL > $(UBOOT_OUT)/SPL.log
	awk ' /HAB Blocks:/ {printf "Blocks = " $$3 " " $$4 " " $$5 " \42$(UBOOT_OUT)/SPL\42\n"} ' $(UBOOT_OUT)/SPL.log | \
	  cat ../u-boot_sign.csf-template - | \
	  sed -e 's#\$$KEY_ROOT#$(KEY_ROOT)#g' > $@

# The dt-spl.dtb has been filled with the public key from the U-Boot/OP-TEE fit, so add it into a clean u-boot-spl binary.
# Hexdump the SRKH fuse bin and add its values into an srkh node in SPL's DTB.
$(SPL_PUB_KEYED): $(UBOOT_OPTEE_FIT)
	touch $(UBOOT_OUT)/spl/u-boot-spl-pad.bin
	dtc -I dtb -O dts -o temp-dt-spl.dts dt-spl.dtb
	echo "/ { srkh { srkh-fuse = <" >> temp-dt-spl.dts
	hexdump -e '/4 "0x"' -e '/4 "%X""\n"' < $(SRKH_FUSE_BIN) >> temp-dt-spl.dts
	echo ">; }; };" >> temp-dt-spl.dts
	dtc -I dts -O dtb -o dt-spl.dtb temp-dt-spl.dts
	rm -f temp-dt-spl.dts
	cat $(UBOOT_OUT)/spl/u-boot-spl-nodtb.bin $(UBOOT_OUT)/spl/u-boot-spl-pad.bin dt-spl.dtb > $@
	rm -f dt-spl.dtb

# Build the EDK2 UEFI target for the board specified by $(EDK2_DSC) and $(EDK2_PLATFORM)
# Copy the output binary into the board firmware folder for FIT generation
uefi: $(EDK2BASETOOLS)
	@+pushd $(REPO_ROOT)/..
	export WORKSPACE=$$PWD
	export PACKAGES_PATH=$$PWD/edk2:$$PWD/imx-edk2-platforms
	export GCC5_ARM_PREFIX=arm-linux-gnueabihf-
	export PATH=~/gcc-linaro-6.4.1-2017.11-x86_64_arm-linux-gnueabihf/bin:$$PATH
	. edk2/edksetup.sh --reconfig
	build -a ARM -t GCC5 -p Platform/$(EDK2_PLATFORM)/$(EDK2_DSC).dsc -b $(EDK2_DEBUG_RELEASE) $(EDK2_FLAGS)
	popd
	cp $(EDK2) .

.PHONY: uefi_fit

# Generate an unsigned UEFI FIT
uefi_fit: uefi | $(MKIMAGE)
	cp -f ../its/$(UEFI_ITS) $(UEFI_ITS)
	$(UBOOT_OUT)/tools/mkimage -f $(UEFI_ITS) -r uefi.fit
	rm -f $(UEFI_ITS)

# Ensure that mkimage is available to build UEFI fits in isolation.
$(MKIMAGE):
	CROSS_COMPILE=$(CROSS_COMPILE) ARCH=arm $(MAKE) -C $(UBOOT_ROOT) $(UBOOT_CONFIG) O=$(UBOOT_OUT)
	CROSS_COMPILE=$(CROSS_COMPILE) ARCH=arm $(MAKE) -C $(UBOOT_ROOT) O=$(UBOOT_OUT) tools

# Build system tools for EDK2. Must be built once before EDK2 is built.
$(EDK2BASETOOLS) edk2_basetools:
	@pushd $(REPO_ROOT)/..
	export WORKSPACE=$$PWD
	export PACKAGES_PATH=$$PWD/edk2:$$PWD/imx-edk2-platforms
	export GCC5_ARM_PREFIX=arm-linux-gnueabihf-
	export PATH=~/gcc-linaro-6.4.1-2017.11-x86_64_arm-linux-gnueabihf/bin:$$PATH
	pushd edk2
	git clean -fdx
	popd
	. edk2/edksetup.sh --reconfig
	$(MAKE) -C edk2/BaseTools -j 1

.PHONY: u-boot optee fTPM uefi edk2_basetools
u-boot: $(UBOOT)
optee: $(OPTEE)

fTPM: optee
	@if [ ! -d $(FTPM_ROOT) ] ; \
	then \
	echo "fTPM directory $(abspath $(FTPM_ROOT)) not found" ; \
	exit 1 ; \
	fi
	@if [ ! -d $(FTPM_ROOT)/../../../../external/wolfssl/wolfcrypt ] ; \
	then \
	echo "fTPM is missing WolfSSL at $(abspath $(FTPM_ROOT)/../../../../external/wolfssl/wolfcrypt), did you initialize the git submodules?" ; \
	exit 1 ; \
	fi
	$(MAKE) -C $(FTPM_ROOT) TA_CPU=cortex-a9 O=$(FTPM_OUT) $(FTPM_FLAGS)
	cp -f $(FTPM_OUT)/bc50d971-d4c9-42c4-82cb-343fb7f37896.* $(EDK2_PLATFORMS_ROOT)/Platform/Microsoft/OpteeClientPkg/Bin/fTpmTa/Arm/Test

# Copy binaries from build output to package location and 'git add' them
.PHONY: update-ffu
update-ffu: $(VERSIONS)
	board=`basename $$PWD` && \
	dest=../../board/$$board/Package/BootLoader && \
	uefidest=../../board/$$board/Package/BootFirmware && \
	cp firmware_fit.merged $(VERSIONS) $$dest && \
	cp uefi.fit $$uefidest && \
	cp $(UBOOT_OUT)/u-boot $$dest/u-boot.elf && \
	cp $(UBOOT_OUT)/spl/u-boot-spl $$dest/u-boot-spl.elf && \
	cp $(OPTEE_OUT)/core/tee.elf $$dest && \
	tar -cf $(REPO_ROOT)/build/board/$$board/export-ta_arm32.tar.bz2 \
	    -C $(OPTEE_OUT) export-ta_arm32 && \
	git add $$dest && \
	git add $$uefidest && \
	git add ../../board/$$board/export-ta_arm32.tar.bz2
	@echo "Successfully copied files to package and staged for commit"

.PHONY: $(VERSIONS)
$(VERSIONS):
	@echo Logging version information of firmware repositories
	rm -f $@
	echo U-Boot commit information: > $@
	pushd $(UBOOT_ROOT) && \
	  git config --get remote.origin.url >> $(CURDIR)/$@ && \
	  git rev-parse HEAD >> $(CURDIR)/$@ && \
	  echo U-Boot diff information: >> $(CURDIR)/$@ && \
	  git diff HEAD >> $(CURDIR)/$@ && \
	  echo U-Boot diff end >> $(CURDIR)/$@
	popd
	echo OPTEE commit information: >> $@
	pushd $(OPTEE_ROOT) && \
	  git config --get remote.origin.url >> $(CURDIR)/$@ && \
	  git rev-parse HEAD >> $(CURDIR)/$@ && \
	  echo OPTEE diff information: >> $(CURDIR)/$@ && \
	  git diff HEAD >> $(CURDIR)/$@ && \
	  echo OPTEE diff end >> $(CURDIR)/$@
	popd
	@echo Logging version information of EDK2 repositories
	echo EDK2 commit information: >> $@
	pushd $(EDK2_ROOT) && \
	  git config --get remote.origin.url >> $(CURDIR)/$@ && \
	  git rev-parse HEAD >> $(CURDIR)/$@ && \
	  echo EDK2 diff information: >> $(CURDIR)/$@ && \
	  git diff HEAD >> $(CURDIR)/$@ && \
	  echo EDK2 diff end >> $(CURDIR)/$@
	popd
	echo EDK2-Platforms commit information: >> $@
	pushd $(EDK2_PLATFORMS_ROOT) && \
	  git config --get remote.origin.url >> $(CURDIR)/$@ && \
	  git rev-parse HEAD >> $(CURDIR)/$@ && \
	  echo EDK2-Platforms diff information: >> $(CURDIR)/$@ && \
	  git diff HEAD >> $(CURDIR)/$@ && \
	  echo EDK2-Platforms diff end >> $(CURDIR)/$@
	popd

# Make sure that dirs have case sensitivity on in Windows Subsystem for Linux.
$(UBOOT): verify_case_sensitivity_$(UBOOT_OUT)
$(OPTEE): verify_case_sensitivity_$(OPTEE_OUT)

.PHONY: verify_case_sensitivity verify_case_sensitivity_$(UBOOT_OUT) verify_case_sensitivity_$(OPTEE_OUT)
verify_case_sensitivity: verify_case_sensitivity_$(UBOOT_OUT) verify_case_sensitivity_$(OPTEE_OUT)
CASE_DIR=$(subst verify_case_sensitivity_,,$@)
CASE_DIR_WIN=$(shell wslpath -m $(CASE_DIR))
CASE_SENSITIVITY_CMD=fsutil.exe file queryCaseSensitiveInfo $(CASE_DIR_WIN) | grep -o
MOUNT_BASE_DIRECTORY=$(shell echo $(CASE_DIR) | cut -d "/" -f1-3)
MOUNT_CASE_SENSITIVITY_CMD=mount | grep "$(MOUNT_BASE_DIRECTORY)" | grep -o
IS_ADMIN_CMD=net.exe session 2>&1 >/dev/null | grep -o

verify_case_sensitivity_$(UBOOT_OUT) verify_case_sensitivity_$(OPTEE_OUT):
ifeq (,$(findstring Microsoft,$(shell cat /proc/sys/kernel/osrelease)))
	@echo Not running WSL, no case sensitivity check needed.
else
	@echo Checking case sensitivity of $(CASE_DIR)
	mkdir -p $(CASE_DIR)

	if test "`$(MOUNT_CASE_SENSITIVITY_CMD) "case=force"`"
	then
	  echo Auto mount case sensitivity is enabled
	  exit 0;
	fi

	if test "`$(CASE_SENSITIVITY_CMD) "is enabled"`"
	then
	  echo Directory already has case sensitivity enabled
	  exit 0;
	fi;

	echo Directory does not have case sensitivity enabled. Attempting to enable it.
	cd $(UBOOT_OUT) > /dev/null
	if test "`$(IS_ADMIN_CMD) "Access is denied"`"
	then
	  echo WSL not running as Administrator, launching an escalated PowerShell session to set case sensitivity!
	  powershell.exe "Start-Process powershell -ArgumentList 'cd $(CASE_DIR_WIN);fsutil.exe file setCaseSensitiveInfo . enable;' -Verb RunAs"
	else
	  echo Setting case sensitivity enabled on $(UBOOT_OUT)
	  fsutil.exe file setCaseSensitiveInfo . enable
	fi
endif

.PHONY: clean
clean:
	rm -rf $(UBOOT_OUT) $(OPTEE_OUT) $(RIOT_OUT) $(FTPM_OUT) $(EDK2_OUT) \
	  $(UBOOT_OPTEE_FIT) $(UBOOT_OPTEE_FIT) firmware_fit.merged \
	  $(SPL_HAB) spl_csf.bin u-boot_sign.csf \
	  IMXBOARD_EFI.fd $(UEFI_FIT) \
	  $(VERSIONS)
