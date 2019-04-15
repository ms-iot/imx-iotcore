Provisioning a Device
==============

This document describes the first boot for a device being provisioned in a factory along with the firmware customizations required to enable the flow.

Assumptions:
* The SD Card or eMMC already contains the OS/Firmware image.
* The device has an eMMC with RPMB to support secure storage for fTPM and Authenticated Variables.
* The device being provisioned has an i.MX6 Quad SoC. Other SoCs will require fusing customizations.

Missing features required for a complete provisioning implementation:
* A connection to an OEM database to assign per-device information such as MAC addresses, serial numbers, and other SMBIOS customizations.
* A connection to an OEM database to save the TPM Endorsement Key Certificates.
* Host side cross-signing of the Endorsement Key Certificate to prove that the TPM is trusted by the OEM.
* Factory integration to power off the device and start provisioning the next on success or to raise an alert on failure.
* Saving Secure Boot UEFI variables to the device to enable secure boot. The required boot critical drivers are not signed yet.
* fTPM Endorsement Primary Seed derived from secret unique identity retrieved from a Trusted/Secure CAAM after High Assurance Boot. Currently the secret is randomly generated and will be different if RPMB is cleared.

## In-factory First Boot Flow
* U-Boot SPL (SRKH and MAC address Provisioning)
  1) U-Boot SPL boots into *spl_board_provision( )* which does the following:
  1) Senses that the system is not in Secure or Trusted state (aka not High Assurance Boot).
  2) Retrieves the SKRH from SPL's device tree and fuses it onto the SoC.
  3) Requests a MAC address from the provisioning host and fuses it onto the SoC (if MAC not already fused).
  4) SPL resets the platform.
  5) After the reset *spl_board_provision( )* senses that the system is in Trusted state so boot continues.
* Early UEFI (Secure Firmware TAs)
  1) The system boots all the way through to UEFI.
  2) UEFI starts the secure firmware TAs (AuthVars & fTPM) which attempt to access the eMMC RPMB.
* OP-TEE (RPMB Provisioning)
  1) OP-TEE detects that the RPMB has not been provisioned with a secret RPMB key and prepares to write it.
  2) The *tee_otp_check_rpmb_key_write_lock( )* function checks one of the SoC general purpose fuses to make sure the SoC has not written an RPMB key before.
  3) OP-TEE sends the RPMB key to the eMMC through UEFI in plain-text, then verifies that it succeeded.
  4) The *tee_otp_set_rpmb_key_write_lock( )* function blows one of the fuse bits in one of the SoC general purpose fuse words so that *tee_otp_check_rpmb_key_write_lock( )* will fail if it's ever called again.
* Late UEFI (EK Certificate Capture, SMBIOS Provisioning)
  1) Once the TPM is finished initializing UEFI loads the provisioning driver which does the following:
  2) Checks UEFI variables to see if "DeviceProvisioned" is already set, but it is not so the driver continues.
  3) Sends a message to the provisioning host over serial to see if it responds correctly.
  4) Generates an Endorsement Key Certificate in the TPM using CreatePrimary and EvictControl TPM Commands.
  5) Retrieves the Endorsement Key Certificate from the TPM using a ReadPublic command
  6) Sends the Endorsement Key Certificate to the provisioning host.
  7) Requests the cross-signed version of the Endorsement Key Certificate from the provisioning host and saves it into a UEFI variable.
  8) Requests per-device SMBIOS customizations from the provisioning host and saves each one in its own UEFI variable.
  9) Sets "DeviceProvisioned" so that all subsequent boots will return from this driver immediately.

## Device side code and board specific customizations

### U-Boot SPL

U-Boot SPL is responsible for fusing the SRKH High Assurance Boot Key and the MAC address into SoC fuses.

1) Copy the code within `#ifdef CONFIG_SPL_BOARD_PROVISION` from board\solidrun\mx6cuboxi\mx6cuboxi.c into your own board specific code. This code includes struct srkfdt and *spl_board_provision( )*.

2) Set CONFIG_SPL_BOARD_PROVISION=y in your board defconfig

3) Build the firmware, deploy it to the device, start provision.py from the "imx-iotcore" section below, and attempt to boot. This firmware will not blow any of the fuses, it will just print out what it would blow. This can be used to verify that the SRKH and MAC addresses are correct before permanently committing them to the platform.

4) Once you've verified that the values are correct you can set CONFIG_SPL_BOARD_PROVISION_FUSES=y in your board defconfig so that *spl_board_provision( )* will blow the SRKH and MAC fuses.

### OP-TEE

The changes in OP-TEE are responsible for making sure the SoC will only ever provision a single eMMC RPMB with its secret key.

1) Add CFG_FSL_SEC=y and CFG_RPMB_KEY_WRITE_LOCK=y to your OP-TEE flags in your board makefile. [Example Makefile](build\firmware\HummingBoardEdge_iMX6Q_2GB\Makefile)

2) If any portion of the GP1 or GP2 fuse words are already reserved for another use on your board then customize which fuse bit to use for the RPMB write lock. Open `core/drivers/fsl_sec/hw_key_blob.c` and modify both `RPMB_KEY_WRITE_LOCK_FUSE_BITS` and `RPMB_KEY_WRITE_LOCK_FUSE_WORD`

3) When the fTPM tries to access the RPMB for the first time, OP-TEE will run through this flow and blow the fuse if the RPMB key write was successful. If the RPMB key is already provisioned, then this flow is skipped entirely.

### UEFI

The changes in UEFI are responsible for the following:
* Loading SMBIOS values from UEFI variables.
* Create an fTPM Endorsement Key Certificate using the default RSA template.
* Loading the Endorsement Key Certificate from the fTPM and saving it to a host computer.
* Receiving a cross-signed EK Certificate to save into UEFI variables on the device for easy access.
* Receiving per-device SMBIOS values to save into UEFI variables to be recalled on future boots.
* Set the DeviceProvisioned variable so subsequent boots will not run this driver.

1) Open `Silicon\NXP\iMX6Pkg\Drivers\PlatformSmbiosDxe\PlatformSmbiosDxe.c` and note where RetrieveSmbiosVariable is called. This function tries to open a UEFI variable and returns an allocated buffer with the value if successful. The table creation functions can be updated to retrieve additional values from UEFI variables. (*FakeSMBIOSDataInVolatileStorage( )* and *StoreSmbiosVariable( )* are used to prepare these values for retrieval until non-volatile UEFI variables are ready.)

2) Open `Silicon\NXP\iMXPlatformPkg\Drivers\Provisioning\Provisioning.inf` and note that the driver is configured to have a depex on gEfiTcg2ProtocolGuid so that the driver is forced to load after the fTPM is available.

3) Open `Silicon\NXP\iMXPlatformPkg\Drivers\Provisioning\Provisioning.c` and note the *ProvisioningInitialize( )* function. This function calls each of the functions responsible for UEFI provisioning. You can extend *ReceiveSmbiosValues( )* with additional *ReceiveBuffer( )* and *SetVariable( )* calls to save more device-specific values from the provisioning host.

### imx-iotcore

The imx-iotcore repository contains the makefile changes required to support SPL SRKH fusing, along with a simple Python script to demonstrate the role of a provisioning host.


1) Open `build\firmware\Common.mk` and observe the following section under `$(SPL_PUB_KEYED): $(UBOOT_OPTEE_FIT)`
    ```makefile
    	dtc -I dtb -O dts -o temp-dt-spl.dts dt-spl.dtb
    	echo "/ { srkh { srkh-fuse = <" >> temp-dt-spl.dts
    	hexdump -e '/4 "0x"' -e '/4 "%X""\n"' < $(SRKH_FUSE_BIN) >> temp-dt-spl.dts
    	echo ">; }; };" >> temp-dt-spl.dts
    	dtc -I dts -O dtb -o dt-spl.dtb temp-dt-spl.dts
    	rm -f temp-dt-spl.dts
    ```
    This section dumps the SPL device tree to a sources file, appends a new srkh section with the variable srkh-fuse, then fills srkh-fuse with the values output by NXP's Code Signing Tool when generating a set of High Assurance Boot Keys. It then recompiles the device tree, replaces the original, and deletes the intermediate device tree source file. This is the value used by SPL to fuse the SRKH onto the SoC and enable High Assurance Boot.

2) Open `provision.py` and customize the following values:
    * Set `ser = serial.Serial('COM4', 115200)` to match the COM port for your serial connection to the device.
    * Set `ekcertlog = "c:\\temp\\mfgek.txt"` to point to a new txt file in directory that exists so the EK Cert can be saved.
    * Set `crosscert = "c:\\temp\\cert.cer"` to point to a file that's about 1KB to simulate a cross-signed cert.
    * Note that the script is hardcoded to send some set values for MAC address and Serial Number.

3) Run `pip install PySerial` in CMD to install the serial library.

4) Run `python provision.py` to start the provisioning host.

5) Boot a connected device running the a firmware that has been configured for provisioning.

## Commands sent from the device to the host
* MFG:reqmac
  * The device is ready to receive a unique MAC address from the provisioning host.
  * The provisioning host sends over two 4-byte integers, which get fused into OCOTP_MAC0 and OCOTP_MAC1 respectively, which form a complete MAC address when combined.
  * The host must ensure that each device receives a unique MAC address from the OEM's assigned MAC addresses.
* MFG:hostcheck
  * The device wants to check if a provisioning host is accessible. If there's no response then boot will continue in 5 seconds.
  * The host is expected to respond with the ASCII bytes for HGFM [0x48, 0x47, 0x46, 0x4D] to prove it is a provisioning host.
* MFG:ekcert
  * The device has retrieved an Endorsement Key Certificate from the fTPM and wants to send it to the provisioning host.
  * The device sends the length of the buffer as a 4-byte integer.
  * The device sends the EK Certificate buffer received from the fTPM.
  * The device sends a 4-byte checksum that's the sum of all the bytes in the buffer.
  * If the host checksum fails the host raises an error.
  * If the host checksum succeeds the EK Certificate is saved on the provisioning host.
* MFG:devicecert
  * The device is ready to receive a cross-signed version of the EK Certificate from the provisioning host.
  * The host sends the length of the buffer as a 4-byte integer
  * The host sends the cross-signed certificate buffer.
  * The host sends a 4-byte checksum of all the bytes in the buffer.
  * If the device checksum fails the device sends an error to the host.
  * If the device checksum succeeds the device saves the certificate in UEFI variables as "ManufacturerDeviceCert"
* MFG:smbiossystemserial
  * The device is ready to receive a unique serial number from the provisioning host.
  * The host sends the length of the string as a 4-byte integer
  * The host sends the string.
  * The host sends a 4-byte checksum of all the bytes in the string.
  * If the device checksum fails the device sends an error to the host.
  * If the device checksum succeeds the device saves the serial number in UEFI variables as "SystemSerialNumber"
* MFG:success
  * The device successfully made it to the end of provisioning and is ready for the provisioning host to move to the next device.
  * The provisioning host may power off the device immediately and begin provisioning the next one.
* MFGF:remotehost
  * The device failed to get the correct response when checking that the provisioning host was connected.
* MFGF:ekcert
  * The device failed to retrieve the EK Certificate from the fTPM.
* MFGF:devicecert
  * The device failed to receive the cross-signed certificate from the host.
* MFGF:smbios
  * The device failed to receive customized SMBIOS values from the host.
* MFGF:provisioned
  * The device failed to save the UEFI Variable that prevents provisioning on future boots.

## Additional information

### SRKH fusing
In order to enable High Assurance Boot the Super Root Key Hash (SRKH) must be fused on your platform.

To obtain this value SPL pulls it out of the Flattened Device Tree (FDT) built into SPL.
During the build of SPL the Common.mk buildscript will retrieve the SRKH values from the file defined by SRKH_FUSE_BIN near the top of the file. This file is generated by NXP's Code Signing Tool (CST) and contains the SRKH values corresponding to the keys allowed for High Assurance Boot.

Fusing the SRKH writes the values into OCOTP_SRK0 through OCOTP_SRK7.

The flow additionally sets the SEC_CONFIG fuse to 1, putting the platform into Closed state on boot, meaning the Boot ROM will only start execution of loaded software if its signature matches the keys allowed in the SRKH.

### MAC address fusing
On the i.MX6 Quad the MAC address is split between two fuses: OCOTP_MAC0 and OCOTP_MAC1

*spl_board_provision( )* sends the string "MFG:reqmac\n" out the serial line then awaits a response from a remote host. The host must respond to the MFG:reqmac request by sending the value to be fused into OCOTP_MAC0, then the value for OCOTP_MAC1 in sequence.

### U-Boot Configuration
The following two settings must be added to your U-Boot config to blow SRKH and MAC address fuses during provisioning. These also require a board specific version of *spl_board_provision( )*.

CONFIG_SPL_BOARD_PROVISION=y
With this configuration set, U-Boot SPL will call the board specific function *spl_board_provision( )* from inside *board_init_r( )*. In the reference flow *spl_board_provision( )* is defined in `mx6cuboxi.c`

CONFIG_SPL_BOARD_PROVISION_FUSES=y
With this configuration set,*spl_board_provision( )* will actually perform permanent fusing operations on the SoC. Can be left disabled while testing the flow on boards that have already been fused.

### OP-TEE RPMB key generation and write once

When the fTPM and AuthVars TAs are running in OP-TEE they make requests to securely store non-volatile data in the RPMB partition of an onboard eMMC. In order to handle these writes and reads OP-TEE submits them to the OP-TEE supplicant who runs the storage driver on it's behalf. In the case of this BSP the OP-TEE suplicant RPMB transactions is UEFI, then once Windows is running it's the imxusdhc driver.

The RPMB relies on a write-once RPMB key that is used to encrypt data retrieved from the eMMC, and to decrypt data sent to the eMMC. In order to program this value, OP-TEE submits a request to the supplicant and sends the key in plaintext to normal-world memory for the supplicant to then burn into the eMMC. This is an operation that *MUST* only be done in the factory, otherwise an attacker could swap eMMCs then intercept the same key computed in OP-TEE and use it to access secure data on the original eMMC.

In order to ensure this key write operation only occurs one time, we blow a general purpose fuse inside of OP-TEE after an eMMC is successfully keyed. The entry point of the key write flow checks to see if the fuse is blown. If the fuse is blown the code will refuse to send the plaintext key to the supplicant again. In normal operation this path will not be hit more than once, as the key write function only occurs when OP-TEE detects that an eMMC is unkeyed, which should only happen in the factory.

The platform specific functions that must be defined are the following:
```C
TEE_Result tee_otp_check_rpmb_key_write_lock(void)
// Checks the RPMB key write-once fuse.
// Returns TEE_ERROR_BAD_STATE if the fuse that forbids RPMB key writes is blown. Will cause an OP-TEE kernel panic on purpose.
// Returns TEE_SUCCESS if the fuse that forbids RPMB key writes is not blown.


TEE_Result tee_otp_set_rpmb_key_write_lock(void)
// Upon verification that the RPMB key was programmed successfully this blows the RPMB key write-once fuse.
// Returns TEE_ERROR_BAD_STATE if the fuse blow operation fails. Will cause an OP-TEE kernel panic on purpose.
// Returns TEE_SUCCESS if the fuse blow operation succeeds.
```
The default weak implementations return TEE_SUCCESS so they're no-ops on platforms without explicit support.

### fTPM Endorsement Key Certificate

The fTPM Endorsement Key Certificate is the public key that can be used to verify the identity of a TPM. In order to trust this EK Cert it must be extracted in the factory and must be stored by the OEM. The OEM then cross-signs the EK Cert to establish a chain to a well-known root of trust. These cross-signed certificates should be saved back onto the platform for convenience, but must be also hosted externally by the OEM in-case the non-volatile storage on the device is reset. Further reading on Endorsement Keys is available from the Trusted Computing Group [here](https://www.trustedcomputinggroup.org/wp-content/uploads/Credential_Profile_EK_V2.0_R14_published.pdf)

The UEFI provisioning DXE driver defined in Provisioning.c opens a handle to the fTPM driver and requests the Endorsement Key Certificate. If the TPM does not return the Endorsement Key Certificate, the provisioning driver submits two more commands, CreatePrimary to generate an EK Cert using the default template, then EvictControl  to store it under a well-known persistent TPM handle. The driver then requests the certificate again. Once the certificate is retrieved it signals the provisioning host that it's about to send the EK Cert by sending the string "MFG:ekcert\n" over serial it then sends the EK Cert over serial along with a length and checksum. The provisioning host then saves the certificate.

Next the device sends "MFG:devicecert\n" to the provisioning host to retrieve the cross-signed version of the certificate. It then receives a length, the buffer, and a checksum from the host device, and if everything succeeded it writes the certificate into the "ManufacturerDeviceCert" UEFI variable.

### SMBIOS Customizations

Some manufacturers may want to customize specific fields in the SMBIOS table with per-device values.

The SMBIOS tables are constructed during boot in [imx-edk2-platforms PlatformSmbiosDxe.c](https://github.com/ms-iot/imx-edk2-platforms/blob/imx/Silicon/NXP/iMX6Pkg/Drivers/PlatformSmbiosDxe/PlatformSmbiosDxe.c). Most of the values are pulled from the Platform Configuration Database (PCD) which are static per board configuration. Some are generated dynamically based on values in fuses or the build time of the firmware. Values can be pulled out of UEFI variables with a fallback to default values if it's not set. See the usage of RetrieveSmbiosVariable in PlatformSmbiosDxe.c as a reference.

These UEFI variables can be populated by a provisioning host over serial from the Provisioning UEFI DXE driver.

### UEFI Variables
A provisioning GUID has been defined as the namespace for UEFI variables used in this flow.
```C
// {72096f5b-2ac7-4e6d-a7bb-bf947d673415}
EFI_GUID ProvisioningGuid =
{ 0x72096f5b, 0x2ac7, 0x4e6d, { 0xa7, 0xbb, 0xbf, 0x94, 0x7d, 0x67, 0x34, 0x15 } };
```
This is a shared value between the UEFI provisioning driver, the UEFI SMBIOS driver, and any OS services that need to pull values out such as the cross-signed EK Certificate.
