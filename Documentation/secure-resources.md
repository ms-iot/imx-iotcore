Securing Peripherals on i.MX6 using OP-TEE
========================

This document describes the process to configure an i.MX6 peripheral for secure access only using OP-TEE.  The document also describes the Windows behavior for these peripherals.

This document assumes you have familiarity with the required [build tools](../README.md), general [boot flow](build-firmware.md), and process to [build firmware](build-firmware.md).

# OP-TEE
Locking down specific peripherals for secure access occurs during the OP-TEE portion of boot, when OP-TEE configures the CSU.

By default, the CSU registers are initialized to allow access from both normal and secure world for all peripherals.

To override this default configuration, one can add an override entry to the access_control global array.  This array can be found in optee_os/core/arch/arm/plat-imx/imx6.c


    static struct csu_csl_access_control access_control[] = {
        /* TZASC1   - CSL16 [7:0] */
        /* TZASC2   - CSL16 [23:16] */
        {16, ((CSU_TZ_SUPERVISOR << 0)  | (CSU_TZ_SUPERVISOR << 16))},
    }

The first field is the CSU CSL register index for the desired device to secure.  The second field is the desired CSU CSL register value.  This value will override the default CSU initialization value.  OP-TEE provides some useful defines to create this value:

    /*
     * Grant R+W access:
     * - Just to TZ Supervisor execution mode, and
     * - Just to a single device
     */
    #define CSU_TZ_SUPERVISOR		0x22

    /*
     * Grant R+W access:
     * - To all execution modes, and
     * - To a single device
     */
    #define CSU_ALL_MODES			0xFF

Note: Each CSU CSL register is responsible for two peripheral devices.  You must set the override value carefully to ensure you are securing the intended peripheral device.

# Windows
Any access to a secure peripheral from normal world will cause a system fault.  To avoid this scenario, we have added code into the PEP driver to automatically read the CSU registers and determine if a Windows-enabled peripheral is not available for normal world use. If this is the case, Windows will hide the secured peripheral automatically.