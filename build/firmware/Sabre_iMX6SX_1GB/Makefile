
UBOOT_CONFIG=mx6sxsabresd_nt_defconfig

EDK2_DSC=Sabre_iMX6SX_1GB
EDK2_PLATFORM=NXP/Sabre_iMX6SX_1GB
EDK2_DEBUG_RELEASE=RELEASE

UBOOT_OPTEE_ITS=uboot_optee_unsigned.its
UEFI_ITS=uefi_dtb_unsigned.its

FTPM_CRYPTO_PROVIDER= CFG_FTPM_USE_WOLF=n 
AUTHVARS_CRYPTO_PROVIDER= CFG_AUTHVARS_USE_WOLF=n

all: firmware_fit.merged firmwareversions.log

include ../Common.mk

.PHONY: $(OPTEE)
$(OPTEE):
	$(MAKE) -C $(OPTEE_ROOT) O=$(OPTEE_OUT) PLATFORM=imx-mx6sxsabresd \
	  $(OPTEE_FLAGS_IMX6SX)
