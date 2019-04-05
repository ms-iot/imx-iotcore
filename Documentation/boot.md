Firmware Boot Documentation
==============

This document describes the boot sequence on i.MX6 from power-on to the first Windows component (bootmgr). There are several components involved: on-chip ROM code, U-Boot SPL, U-Boot proper, OP-TEE, and UEFI.

  1. The on-chip ROM code
     1. Loads SPL into OCRAM.
     2. If High Assurance Boot is enabled boot ROM halts if SPL signature is invalid.
     3. Jumps into SPL.
  2. SPL
     1. Captures and hides the secret device identity when High Assurance Boot is enabled.
     2. Verifies Flat Image Tree containing U-Boot proper and OP-TEE.
     3. Loads OP-TEE and U-Boot proper.
     <!--1. Passes the device identity and a key generated from identity and OP-TEE hash into OP-TEE.-->
     4. Jumps into OP-TEE.
  3. OP-TEE
     1. OP-TEE runtime initialization.
     2. Switches to normal world then jumps into U-Boot proper.
  4. U-Boot proper loads UEFI then jumps to UEFI.
  5. UEFI loads and starts bootmgr.

SPL and U-Boot are not retained in memory after boot, while parts of OP-TEE and UEFI remain in memory while the OS runs. The OS calls into OP-TEE and UEFI at runtime to perform certain functions, such as real-time clock operations and processor power management.

# 1. On-chip ROM code

Execution begins in on-chip ROM code which is burned into the chip. The ROM code reads its configuration from on-chip fuses. Fuses control options such as boot source, JTAG settings, high-assurance boot (HAB), and TrustZone configuration. Boot source is the media from which the next boot stage will be loaded. This can be EIM, SATA, serial ROM, SD/eSD, MMC/eMMC, or NAND flash. Only SD and eMMC are supported by the reference firmware, and the rest of this document describes the boot flow from SD/eMMC. The on-chip ROM code reads the boot binary from the boot source into memory, performs high-assurance boot verification, and jumps to it.

The on-chip ROM code

  1. Reads fuses to determine boot source
  1. Reads fuses to determine HAB state
  1. Loads boot header from MMC sector 2 to OCRAM
  1. Parses boot header and loads rest of boot binary (SPL) into OCRAM
      * Load address defined by `CONFIG_SPL_TEXT_BASE` in `include/configs/imx6_spl.h`
  1. Parses CSF and does HAB verification
  1. Runs DCD commands from boot header
  1. Jumps to SPL entry point

Memory layout just before jump to SPL:

    DRAM not yet initialized

    SRAM
    +---------------------------+ 0x00940000 (end of SRAM)
    | reserved by boot ROM      |
    +---------------------------+ 0x00938000
    |                           |
    |                           |
    |                           |
    |                           |
    | SPL                       |
    +---------------------------+ 0x00908000 (CONFIG_SPL_TEXT_BASE)
    |                           |
    +---------------------------+ 0x00907000
    | reserved by boot ROM      |
    +---------------------------+ 0x00900000 (start of SRAM)

# 2. SPL

SPL is a binary produced by the U-Boot build whose purpose is to prepare the system for execution of full U-Boot (U-Boot proper) from DRAM. SPL is the first piece of code that can be changed, as opposed to on-chip ROM code which is burned into the chip and cannot be changed. SPL builds from the same sources as full U-Boot, but is designed to be as small as possible to fit in OCRAM. The included U-Boot has modifications to load OP-TEE.

The reference implementation of SPL

  1. Begins execution at `arch/arm/cpu/armv7/start.S : reset`
  1. Does low-level CPU init
      1. Errata
      1. CP15 and system control registers
  1. Initializes DDR
  1. Initializes critical hardware
      1. Pin muxing
      1. Clocks
      1. Timer
      1. Console UART
  1. Enables L1 cache
      1. Sets up page tables above stack. There must be 16k of available memory above the stack to hold the page tables.
      1. Stack top defined as `CONFIG_SPL_STACK` in `include/configs/imx6_spl.h`
  1. Initializes CAAM security hardware
      1. Initializes RNG capabilities of the CAAM
  1. Attempt to read and hide a unique secret device identity from the SoC.
      1. This will only succeed if the system has been fused for High Assurance Boot
      <!-- 1. Store the unique secret device identity for OP-TEE. -->
  1. Load U-Boot proper and OP-TEE binaries using a Flattened Image Tree (FIT)
      1. Loads the FIT header from MMC sector `CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR`. The default for ARCH_MX6 is defined in `common/spl/Kconfig`, but a defconfig can override the value.
          * A FIT is a single binary which both stores the U-Boot and OP-TEE images, and encodes their load addresses and entry points
          * The FIT source file (image_source.its) describes the structure of the FIT to the U-Boot mkimage tool which is responsible for assembling the image. The load and entry addresses for U-Boot (`CONFIG_SYS_TEXT_BASE`) and OP-TEE (`CFG_TEE_LOAD_ADDR`) are updated automatically when the firmware is built
         1. Verifies that the signature of the config block in the FIT matches the public key baked in SPL's Device Tree Blob (DTB) at image creation time.
         2. Verifies that the hashes for U-Boot and OP-TEE match the values stored in the signed FIT config.
         <!-- 1. Derive a key pair based on the Unique Device Secret and the hash measurement of OP-TEE to pass a device identity to OP-TEE for attestation. -->
         3. Loads U-Boot and OP-TEE to memory based on offsets stored in the FIT
         4. Identifies that the OP-TEE image is bootable using the FIT, disables caches and interrupts, then jumps into OP-TEE.

Memory layout just before jump to OP-TEE:

    DRAM
    +---------------------------+
    |                           |
    |                           |
    +---------------------------+
    |                           |
    | U-Boot proper             |
    +---------------------------+ 0x17800000 (CONFIG_SYS_TEXT_BASE)
    |                           |
    |                           |
    +---------------------------+
    |                           |
    | OP-TEE                    |
    +---------------------------+ 0x10A00000 (CFG_TEE_LOAD_ADDR)
    |                           |
    |                           |
    +---------------------------+ 0x10000000 (start of DRAM)

    SRAM
    +---------------------------+ 0x00940000
    | heap (grows down)         | 0x0093C000 (CONFIG_SPL_STACK)
    |                           |
    | stack (grows down)        |
    |                           |
    |                           |
    | bss                       |
    | data                      |
    | text                      |
    | SPL                       |
    |                           |
    +---------------------------+ 0x00908000 (CONFIG_SPL_TEXT_BASE)
    |                           |
    |                           |
    | page tables (16k)         |
    +---------------------------+ 0x00900000

## 3. OP-TEE

OP-TEE is a trusted operating system that runs in ARM TrustZone. It implements a trusted execution environment that can host trusted applications, and it implements the ARM Power State Coordination Interface (PSCI).

The reference implementation of OP-TEE

  1. Begins execution at `core/arch/arm/kernel/generic_entry_a32.S : _start`
      1. OP-TEE's load address is `CFG_TEE_LOAD_ADDR` defined in `optee_os/core/arch/arm/plat-imx/platform_config.h`
  1. Does low-level CPU initialization
      1. Disallows unaligned access
      1. Configures non-secure world (NS) access to CPU features
  1. Configures and enables SCU
  1. Configures i.MX6 CSU
  1. Enables debug console UART
  1. Enables MMU and L1 cache
  1. Sets up secure monitor code
  1. Initializes GIC
  1. Initializes TEE core
  1. Initializes drivers and services
  1. Jumps to nonsecure world at the address passed in LR at entry, which should be the U-Boot proper entry point in DRAM

Memory layout just before jump to U-Boot proper:

    DRAM
    +---------------------------+
    |                           |
    |                           |
    +---------------------------+
    |                           |
    | U-Boot proper             |
    +---------------------------+ 0x17800000 (CONFIG_SYS_TEXT_BASE)
    |                           |
    |                           |
    |                           |
    +---------------------------+ 0x12A00000 (CFG_DDR_TEETZ_RESERVED_SIZE +
    |                           |             CFG_DDR_TEETZ_RESERVED_START)
    | OP-TEE shared memory      |
    +---------------------------+ 0x12800000 (CFG_SHMEM_START)
    |                           |
    |                           |
    |                           |
    | OP-TEE private secure     |
    | memory                    |
    +---------------------------+ 0x10A00000 (CFG_TEE_LOAD_ADDR)
    |                           |
    |                           |
    +---------------------------+ 0x10000000

    SRAM
    +---------------------------+ 0x00940000
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    +---------------------------+ 0x00900000

## 4. U-Boot Proper

U-Boot proper is the full U-Boot binary with scripting support, filesystem support, command support, and hardware support. U-Boot proper executes in normal world and initializes hardware, then loads UEFI and jumps to UEFI.

The reference implementation of U-Boot proper

  1. Begins execution at `arch/arm/cpu/armv7/start.S : reset`
      1. Load address is `CONFIG_SYS_TEXT_BASE` defined in `configs/mx6cuboxi_nt_defconfig`
  1. Does low-level CPU initialization
  1. Executes `arch/arm/lib/crt0.S : _main`
  1. Executes `common/board_f.c : board_init_f`
      1. Hardware initialization
      1. Muxing
      1. Clocks
      1. Console UART
  1. Relocates to top of DRAM
  1. Enables L1 cache
  1. Does hardware initialization (USB, ENET, PCI, SPI, I2C, PMIC, Thermal, etc.)
  1. Runs the boot command. The boot command is a script defined by `CONFIG_BOOTCOMMAND` which is defined in `configs/mx6cuboxi_nt_defconfig`. This script
      1. Initializes a global page and stashes the MAC address known to U-Boot for use in UEFI.
      1. Loads uefi.fit from a specified MMC device / FAT partition to a specified address in DRAM.
      1. Calls bootm on uefi.fit in memory, which will load UEFI to its BaseAddress.
      1. Bootm then disables caches and interrupts and jumps to UEFI's entry point, which is specified as load and entry in `imx-iotcore/build/firmware/HummingBoardEdge_iMX6Q_2GB/uefi.its`. This address must match the values defined by `BaseAddress` in `edk2-platforms/Silicon/NXP/iMX6Pkg/iMX6CommonFdf.inc`

Memory map just before jumping to UEFI:

    DRAM
    +---------------------------+
    |                           |
    | relocated U-Boot and      |
    | stack, heap               |
    +---------------------------+
    |                           |
    |                           |
    |                           |
    +---------------------------+
    |                           |
    | U-Boot proper             |
    +---------------------------+ 0x17800000 (CONFIG_SYS_TEXT_BASE)
    |                           |
    |                           |
    |                           |
    +---------------------------+ 0x12A00000 (CFG_DDR_TEETZ_RESERVED_SIZE +
    |                           |             CFG_DDR_TEETZ_RESERVED_START)
    | OP-TEE shared memory      |
    +---------------------------+ 0x12800000 (CFG_SHMEM_START)
    |                           |
    |                           |
    |                           |
    | OP-TEE private secure     |
    | memory                    |
    +---------------------------+ 0x10A00000 (CFG_TEE_LOAD_ADDR)
    |                           |
    | UEFI                      |
    +---------------------------+ 0x10820000 (BaseAddress, load/entry in uefi.its)
    |                           |
    |                           |
    +---------------------------+ 0x10000000

    SRAM
    +---------------------------+ 0x00940000
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    +---------------------------+ 0x00900000

## 5. UEFI

UEFI prepares for Windows and starts the Windows boot manager. The Windows boot manager (bootmgr) and bootloader (winload) are written as UEFI applications, and must run within the UEFI environment.

The reference implementation of UEFI

  1. Begins execution at `ArmPlatformPkg/PrePi/Arm/ModuleEntryPoint.S : _ModuleEntryPoint`
      1. Load address is `BaseAddress` defined in each platform's fdf file such as `edk2-platforms/Platform/NXP/Sabre_iMX6Q_1GB/Sabre_iMX6Q_1GB.fdf`
  2. Does hardware initialization
  3. Unless `CONFIG_NOT_SECURE_UEFI=1` is set, the authenticated variable store Trusted Application (TA) is loaded by `imx-edk2-platforms/Platform/Microsoft/OpteeClientPkg/Drivers/AuthVarsDxe.c`. This TA is responsible for storing non-volatile variables in eMMC RPMB.
  4. The Authvar TA may also contain Secure Boot keys. If the keys are pressent UEFI will enable Secure Boot and verify the signatures on all subsequent components as they are loaded.
  5. Unless `CONFIG_NOT_SECURE_UEFI=1` is set a firmware TPM TA is also loaded by `imx-edk2-platforms/Platform/Microsoft/OpteeClientPkg/Library/Tpm2DeviceLibOptee/Tpm2DeviceLibOptee.c`. The TPM also uses RPMB for non-volatile secure storage. UEFI measures each subsequent component as it is loaded and saves these values in Platform Configuration Registers (PCRs) in the TPM. Windows will use these measurements to verify the system is secure and unlock BitLocker encrypted drives.
  6. Sets up structures for handoff to Windows
  7. Loads and runs bootmgr

Bootmgr then orchestrates the process of loading Windows.

Memory map just before jumping to bootmgr:

    DRAM
    +---------------------------+
    |                           |
    | UEFI stack                |
    +---------------------------+
    |                           |
    |                           |
    +---------------------------+ 0x12A00000 (CFG_DDR_TEETZ_RESERVED_SIZE +
    |                           |             CFG_DDR_TEETZ_RESERVED_START)
    | OP-TEE shared memory      |
    +---------------------------+ 0x12800000 (CFG_SHMEM_START)
    |                           |
    |                           |
    |                           |
    | OP-TEE private secure     |
    | memory                    |
    +---------------------------+ 0x10A00000 (CFG_TEE_LOAD_ADDR)
    |                           |
    | UEFI                      |
    +---------------------------+ 0x10820000 (BaseAddress, uefi_addr)
    | Global data               |
    +---------------------------+ 0x10817000 (PcdGlobalDataBaseAddress)
    | TPM2 control area         |
    +---------------------------+ 0x10814000 (PcdTpm2AcpiBufferBase)
    |                           |
    +---------------------------+ 0x10800000
    | Frame Buffer              |
    +---------------------------+ 0x10000000 (PcdFrameBufferBase)

    SRAM
    +---------------------------+ 0x00940000
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    +---------------------------+ 0x00900000

# SD/eMMC Layout

SD/eMMC is laid out as follows:

    +---------------------------+ Sector 0
    | partition table           |
    |                           |
    +---------------------------+ Sector 2
    | SPL_signed.imx            |
    |    IMX bootloader header  |
    |    SPL binary             |
    |    DTB with image.fit key |
    |    CSF data (for HAB)     |
    |                           |
    +---------------------------+
    |                           |
    |                           |
    +---------------------------+ Sector 136 (CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR)
    | image.fit                 |
    |    OPTEE                  |
    |    U-Boot Proper          |
    |      DTB with uefi.fit key|
    +---------------------------+
    |                           |
    |                           |
    +---------------------------+
                  .
                  .
                  .
    +---------------------------+ Partition 2 (FAT)
    | uefi.fit                  |
    |                           |
    +---------------------------+
