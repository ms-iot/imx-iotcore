# Signing Firmware for i.MX6 and i.MX7
This document is a high level overview of the various signing keys used on i.MX6 and i.MX7 devices to construct the High Assurance Boot chain from Boot ROM all the way to Windows IoT Core.

## High Assurance Boot
High Assurance Boot is an NXP security feature to ensure that the Boot ROM will only load code that has been signed with the correct private key. This is accomplished by creating a public/private keypair with NXP's Code Signing Tool (CST), burning the Super Root Key Hash of the public key into SoC fuses, then signing the SPL binary with the manufacturer's private key. The private keys used for HAB **MUST BE** kept secure and secret because they are the root of trust for your device and any firmware signed with these private keys will be allowed to run.

Testing keys for development have already been generated and are available in the [test_keys_no_security folder](../build/firmware/test_keys_no_security). By default all SPLs built in imx-iotcore are signed with these keys.
 * These keys are for development only and **MUST NOT** be used for production!
 * To enable High Assurance Boot the Super Root Key Hash of these public keys must be burned into the SoC. This is **permanent** and no other first stage firmware except those signed with this key will be allowed to run.

The test keys folder is selected using the KEY_ROOT define in [Common.mk](../build/firmware/Common.mk) and the specific keys in the folder are chosen in [u-boot_sign.csf-template](../build/firmware/u-boot_sign.csf-template). The makefile copies u-boot_sign.csf-template into u-boot_sign.csf then fills out some build specific information. Then u-boot_sign.csf is used with CST to sign the SPL binary for High Assurance Boot.

Documentation on generating new keys can be found inside the download for the NXP Code Signing Tool: https://www.nxp.com/webapp/sps/download/license.jsp?colCode=IMX_CST_TOOL

NXP High Assurance Boot (HABv4) Documentation: http://cache.freescale.com/files/32bit/doc/app_note/AN4581.pdf

## SPL/U-Boot Flattened Image Tree Loading
SPL is used to load the Flattened Image Tree (FIT) containing OP-TEE and U-Boot Proper. U-Boot proper is used to load the FIT containing UEFI.

The FIT is generated using the mkimage tool and the Image Tree Source (.its) files in the firmware/its folder.
The per-board firmware makefiles choose which .its files to use using the UBOOT_OPTEE_ITS and UEFI_ITS variables.

Each .its file contains a list of images to load and a configuration block. SPL and U-Boot use the configuration block to verify the hashes of the stored images when loading them. There are two versions of each .its file, *unsigned.its and *ecc_signed.its. The unsigned.its files don't include a signature in the configuration block, the *ecc_signed.its include an Elliptic-curve Cryptography based signature in the configuration block. The key used by each .its file is configurable by changing the key-name-hint to a different filename.

The FIT test key included in the repository was generated using the following OpenSSL command:
```
openssl ecparam -genkey -name prime256v1 -out ecc_prime256v1_dev_pem.key
```

When Common.mk uses mkimage on an .its file with a signature in the configuration block it writes out the public key into the Device Tree Blob (dtb) of the firmware that is expected to load the FIT.

As an example this is what happens when uefi.fit is built:

1. The makefile is building uefi.fit with uefi_ecc_signed.its so it signs the FIT with the private key and injects the public key into U-Boot's dt.dtb file.
2. When U-Boot attempts to load any FIT, in this case uefi.fit, it will verify that the FIT has a configuration block.
3. Once U-Boot has verified that uefi.fit has a configuration block it checks to make sure that the configuration block was signed with the private key corresponding to the public key that was built into its own Device Tree Blob.

If the *unsigned.its files are used no public keys are injected into the U-Boot or SPL DTBs. This causes them to not check the presence or signature of the configuration block in the loaded FITs, breaking the High Assurance Boot chain.

## UEFI Secure Boot Keys
Secure Boot is enabled during UEFI by the presence of several variables in the Authenticated Variable store.

Further Reading:
* [Microsoft documentation for Secure Boot](https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/windows-secure-boot-key-creation-and-management-guidance)
* [Microsoft documentation for enabling Secure Boot and BitLocker on IoT Core](https://docs.microsoft.com/en-us/windows/iot-core/secure-your-device/securebootandbitlocker)

## OP-TEE TA Signing Key
OP-TEE uses a signing key to verify that all TAs it loads are authorized to run. This key is defined by TA_SIGN_KEY. The default key is optee_os/keys/default_ta.pem.

A different key can be selected by setting the environment variable TA_SIGN_KEY to a different pem file before building OP-TEE. This will require a rebuild of all TAs expected to run on the new build of OP-TEE.

OP-TEE runs a script to extract the public key from the TA_SIGN_KEY pem into optee_os\core\ta_pub_key.c. It then uses the public key information in the shdr_verify_signature function to verify that TAs are correctly signed while loading them.