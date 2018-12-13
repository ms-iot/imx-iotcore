
UBOOT_CONFIG=udoo_neo_nt_defconfig

EDK2_DSC=UdooNeo_iMX6SX_1GB
EDK2_PLATFORM=Udoo/UdooNeo_iMX6SX_1GB
EDK2_DEBUG_RELEASE=RELEASE
EDK2_FLAGS=-D CONFIG_NOT_SECURE_UEFI=1

UBOOT_OPTEE_ITS=uboot_optee_unsigned.its
UEFI_ITS=uefi_dtb_unsigned.its

all: firmware_fit.merged firmwareversions.log

include ../Common.mk

.PHONY: $(OPTEE)
$(OPTEE):
	$(MAKE) -C $(OPTEE_ROOT) O=$(OPTEE_OUT) PLATFORM=imx-mx6sxudooneofull \
	  $(OPTEE_FLAGS_IMX6SX)
