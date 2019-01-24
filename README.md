Windows 10 IoT Core for NXP i.MX Processors
==============

**Important! Please read this section first.**

**This code is provided as a public preview, it is still under development and notwithstanding the license attached to this code, should not be used in any commercial application at this time. For any questions, please post to the [NXP community](https://community.nxp.com/community/imx).**

**This code is available under the [MIT](LICENSE) license except where stated otherwise such as the imxnetmini driver and OpteeClientLib.**

## Board List

| SoC Type | Board Vendor | Board Name | Board Package Name|
|-----|-----|-----|-----|
|i.MX 6Quad | SolidRun | HummingBoard Edge | HummingBoardEdge_iMX6Q_2GB |
|i.MX 6Quad | NXP | i.MX 6Quad SABRE | Sabre_iMX6Q_1GB |
|i.MX 6Quad | Boundry Devices | i.MX 6Quad SABRELITE | SabreLite_iMX6Q_1GB |
|i.MX 6Quad | VIA | VAB-820 | VAB820_iMX6Q_1GB |
|i.MX 6QuadPlus | NXP | i.MX 6QuadPlus SABRE | Sabre_iMX6QP_1GB |
|i.MX 6DualLite | SolidRun | HummingBoard Edge | HummingBoardEdge_iMX6DL_1GB |
|i.MX 6Solo | SolidRun | HummingBoard Edge | HummingBoardEdge_iMX6S_512MB |
|i.MX 6SoloX | UDOO | Neo Full | UdooNeo_iMX6SX_1GB |
|i.MX 7Dual | CompuLab | IoT Gateway, CL-SOM-iMX7+SBC-iMX7 | ClSomImx7_iMX7D_1GB |
|i.MX 7Dual | NXP | i.MX 7Dual SABRE | Sabre_iMX7D_1GB |

A table of the currently enabled features for each board can be found [here](Documentation/board-feature-list.md). For hardware issues, please contact the hardware vendor.

## Building the BSP

### Cloning the Repository
This repository uses submodules and should be cloned with `git clone --recurse-submodules`

### Required Tools
The following tools are required to build the driver packages and IoT Core FFU: Visual Studio 2017, Windows Kits (ADK/SDK/WDK), and the IoT Core OS Packages.

#### Visual Studio 2017
* Make sure that you **install Visual Studio 2017 before the WDK** so that the WDK can install a required plugin.
* Download from https://www.visualstudio.com.
* During install select **Desktop development with C++**.
* During install select the following in the Individual components tab. If these options are not available try updating VS2017 to the latest release:
  * **VC++ 2017 version 15.9 v14.16 Libs for Spectre (ARM)**
  * **VC++ 2017 version 15.9 v14.16 Libs for Spectre (ARM64)**
  * **VC++ 2017 version 15.9 v14.16 Libs for Spectre (X86 and x64)**
  * **Visual C++ compilers and libraries for ARM**
  * **Visual C++ compilers and libraries for ARM64**

#### Windows Kits from Windows 10, version 1809
* **IMPORTANT: Make sure that any previous versions of the ADK and WDK have been uninstalled!**
* Install [ADK 1809](https://developer.microsoft.com/en-us/windows/hardware/windows-assessment-deployment-kit)
* Install [WDK 1809](https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit)
  * Make sure that you allow the Visual Studio Extension to install after the WDK install completes.
* If the WDK installer says it could not find the correct SDK version, install [SDK 1809](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk)

#### IoT Core OS Packages
* Visit the [Windows IoT Core Downloads](https://www.microsoft.com/en-us/software-download/windows10IoTCore#!) page and download "Windows 10 IoT Core Packages – Windows 10 IoT Core, version 1809 (LTSC)".
* Open the iso and install ```Windows_10_IoT_Core_ARM_Packages.msi```

### One-Time Environment Setup
Test certificates must be installed to generate driver packages on a development machine.
1. Open an Administrator Command Prompt.
2. Navigate to your newly cloned repo and into the folder `imx-iotcore\build\tools`.
3. Launch `StartBuildEnv.bat`.
4. Run `SetupCertificate.bat` to install the test certificates.
5. Make sure that submodules have been cloned. If you cloned with `--recurse-submodules` then this step won't output anything.
    ```
    git submodule init
    git submodule update
    ```

### FFU Generation

1. Launch Visual Studio 2017 as Administrator.
2. Open the solution iMXPlatform.sln (imx-iotcore\build\solution\iMXPlatform).
3. Change the build type from Debug to Release.
4. To build press Ctrl-Shift-B or choose Build -> Build Solution from menu. This will compile all driver packages then generate the FFU.
5. Depending on the speed of the build machine FFU generation may take around 10-20 minutes.
6. After a successful build the new FFU will be located in `imx-iotcore\build\solution\iMXPlatform\Build\FFU\HummingBoardEdge_iMX6Q_2GB\`.
7. The FFU contains firmware components for the HummingBoard Edge with the Quad Core SOM. This firmware is automatically applied to the SD Card during the FFU imaging process.

#### Building the FFU for other boards
In order to build an FFU for another board you'll need to modify GenerateFFU.bat in the Build Scripts folder of the Solution Explorer. Comment out the default HummingBoardEdge_iMX6Q_2GB build with REM and uncomment any other boards you want to build.
```bat
REM cd /d %BATCH_HOME%
REM echo "Building HummingBoardEdge_iMX6Q_2GB FFU"
REM call BuildImage HummingBoardEdge_iMX6Q_2GB HummingBoardEdge_iMX6Q_2GB_TestOEMInput.xml

cd /d %BATCH_HOME%
echo "Building Sabre_iMX6Q_1GB FFU"
call BuildImage Sabre_iMX6Q_1GB Sabre_iMX6Q_1GB_TestOEMInput.xml
```

### Deploy the FFU
 - Follow the instructions in the [IoT Core Manufacturing Guide](https://docs.microsoft.com/en-us/windows-hardware/manufacture/iot/create-a-basic-image#span-idflashanimagespanflash-the-image-to-a-memory-card) to flash the FFU to an SD Card using the Windows IoT Core Dashboard.

### Installing to an eMMC
 - Follow the instructions in the [Booting WinPE and Flashing eMMC](Documentation/winpe-mmc.md) document.

### Adding a New Board
 - Follow the instructions in the [Adding a New Board](Documentation/newboard.md) document.

### Adding a New Driver
 - Follow the instructions in the [Adding a New Driver](Documentation/adding-drivers.md) document.

### Building the FFU with the IoT ADK AddonKit
1. Build the GenerateBSP project to create a BSP folder in the root of the repository.
2. Clone the [IoT ADK AddonKit](https://github.com/ms-iot/iot-adk-addonkit).
3. Follow the [Create a basic image instructions](https://docs.microsoft.com/en-us/windows-hardware/manufacture/iot/create-a-basic-image) from the IoT Core Manufacturing guide with the following changes.
* When importing a BSP use one of the board names from the newly generated BSP folder in the imx-iotcore repo.
    ```
    Import-IoTBSP HummingBoardEdge_iMX6Q_2GB <Path to imx-iotcore\BSP>
    ```
* When creating a product use the same board name from the BSP import.
    ```
    Add-IoTProduct ProductA HummingBoardEdge_iMX6Q_2GB
    ```


# Building Firmware from Source

Building custom firmware into an FFU requires additional steps:

* [Building and Updating Firmware](Documentation/build-firmware.md)
* [Firmware Boot Documentation](Documentation/boot.md)
* [Testing your BSP](Documentation/tests.md)
* [Creating Windows PE images and booting from eMMC](Documentation/winpe-mmc.md)

The firmware code can be found in the following repos:

* U-Boot: https://github.com/ms-iot/u-boot.git
* OP-TEE: https://github.com/ms-iot/optee_os.git
* UEFI:
  * https://github.com/tianocore/edk2.git
  * https://github.com/ms-iot/imx-edk2-platforms.git
* Firmware TPM2.0:
  * https://github.com/Microsoft/ms-tpm-20-ref

### Directories

BSP - Generated at build time. Contains Board Support Packages for the [IoT ADK AddonKit](https://github.com/ms-iot/iot-adk-addonkit).

build - Contains Board Packages, build scripts, and the VS2017 solution file.

driver - Contains driver sources.

documentation - Contains usage documentation.

hal - Contains hal extension sources.

## Info

For more information about Windows 10 IoT Core, see our online documentation [here](http://windowsondevices.com)

We are working hard to improve Windows 10 IoT Core and deeply value any feedback we get.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
