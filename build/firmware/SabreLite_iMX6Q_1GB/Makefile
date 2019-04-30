
UBOOT_CONFIG=mx6sabrelite_nt_defconfig

EDK2_DSC=SabreLite_iMX6Q_1GB
EDK2_PLATFORM=BoundaryDevices/SabreLite_iMX6Q_1GB
EDK2_DEBUG_RELEASE=RELEASE
EDK2_FLAGS=-D CONFIG_NOT_SECURE_UEFI=1

FTPM_CRYPTO_PROVIDER= CFG_FTPM_USE_WOLF=n 
AUTHVARS_CRYPTO_PROVIDER= CFG_AUTHVARS_USE_WOLF=n

UBOOT_OPTEE_ITS=uboot_optee_unsigned.its
UEFI_ITS=uefi_unsigned.its

all: firmware_fit.merged firmwareversions.log

include ../Common.mk

.PHONY: $(OPTEE)
$(OPTEE):
	$(MAKE) -C $(OPTEE_ROOT) O=$(OPTEE_OUT) PLATFORM=imx-mx6qsabrelite \
	  $(OPTEE_FLAGS_IMX6) \
