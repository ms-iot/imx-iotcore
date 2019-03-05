
UBOOT_CONFIG=mx6vab820_nt_defconfig

EDK2_DSC=VAB820_iMX6Q_1GB
EDK2_PLATFORM=VIA/VAB820_iMX6Q_1GB
EDK2_DEBUG_RELEASE=RELEASE

UBOOT_OPTEE_ITS=uboot_optee_unsigned.its
UEFI_ITS=uefi_unsigned.its

all: firmware_fit.merged firmwareversions.log

include ../Common.mk

.PHONY: $(OPTEE)
$(OPTEE):
	$(MAKE) -C $(OPTEE_ROOT) O=$(OPTEE_OUT) PLATFORM=imx-mx6qvab820 \
	  $(OPTEE_FLAGS_IMX6)
