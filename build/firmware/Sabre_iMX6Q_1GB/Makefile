
UBOOT_CONFIG=mx6sabresd_nt_defconfig

EDK2_DSC=Sabre_iMX6Q_1GB
EDK2_PLATFORM=NXP/Sabre_iMX6Q_1GB
EDK2_DEBUG_RELEASE=RELEASE

FTPM_CRYPTO_PROVIDER= CFG_FTPM_USE_WOLF=n 
AUTHVARS_CRYPTO_PROVIDER= CFG_AUTHVARS_USE_WOLF=n

UBOOT_OPTEE_ITS=uboot_optee_unsigned.its
UEFI_ITS=uefi_unsigned.its

all: firmware_fit.merged firmwareversions.log

include ../Common.mk

.PHONY: $(OPTEE)
$(OPTEE):
	$(MAKE) -C $(OPTEE_ROOT) O=$(OPTEE_OUT) PLATFORM=imx-mx6qsabresd \
	  $(OPTEE_FLAGS_IMX6)
