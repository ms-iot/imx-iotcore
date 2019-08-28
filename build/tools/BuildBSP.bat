:: Copyright (c) Microsoft Corporation. All rights reserved.
:: Licensed under the MIT License.

@echo off
setlocal enableextensions disabledelayedexpansion

pushd %~dp0\..\..

if "%1"=="" (
    echo "Missing SOC argument to BuildBSP.bat"
    echo "Usage: BuildBSP.bat <SOC> <boardname>"
    echo "eg: BuildBSP.bat iMX6 HummingBoardEdge_iMX6Q_2GB"
    exit /b 1
)

if "%2"=="" (
    echo "Missing boardname argument to BuildBSP.bat"
    echo "Usage: BuildBSP.bat <SOC> <boardname>"
    echo "eg: BuildBSP.bat iMX6 HummingBoardEdge_iMX6Q_2GB"
    exit /b 1
)

if "%Configuration%"=="" (
    echo configuration environment variable is not set
    exit /b 1
)

if "%PLATFORM%"=="" (
    echo platform environment variable is not set
    exit /b 1
)

set SOC=%1
set BOARDNAME=%2
set REPO_ROOT=%cd%
set BSP_ROOT=%cd%\BSP\%BOARDNAME%
set PKG_ROOT=%BSP_ROOT%\Packages
set BUILD_ROOT=%REPO_ROOT%\build\solution\iMXPlatform\Build\%PLATFORM%\%Configuration%
set INFO_ROOT=%REPO_ROOT%\build\board\%BOARDNAME%\Package
set FAILURE=

echo BSP_ROOT is %BSP_ROOT%
echo BUILD_ROOT is %BUILD_ROOT%
echo PKG_ROOT is %PKG_ROOT%
echo REPO_ROOT is %REPO_ROOT%
echo OutDir is %OutDir%

mkdir %BSP_ROOT%\OEMInputSamples 2>NUL
mkdir %BSP_ROOT%\Packages 2>NUL
copy build\board\%BOARDNAME%\%BOARDNAME%_TestOEMInput.xml %BSP_ROOT%\OEMInputSamples\TestOEMInput.xml >NUL
copy build\board\%BOARDNAME%\%BOARDNAME%_ProductionOEMInput.xml %BSP_ROOT%\OEMInputSamples\RetailOEMInput.xml >NUL
copy build\board\%BOARDNAME%\%BOARDNAME%_FMFileList.xml %BSP_ROOT%\Packages\%BOARDNAME%FMFileList.xml >NUL
copy build\board\%BOARDNAME%\InputFMs\%BOARDNAME%_DeviceFM.xml %BSP_ROOT%\Packages >NUL

if "%SOC%"=="iMX6" (
    goto ARM32
)

if "%SOC%"=="iMX7" (
    goto ARM32
)

if "%SOC%"=="iMX8" (
    goto ARM64
)

echo Unrecognized SOC type: %SOC%
exit /b 1

:ARM32
echo Building ARM32 BSP

:: Copy Driver Packages
echo Copying Driver Packages to %PKG_ROOT%
mkdir %PKG_ROOT%\Audio >NUL 2>NUL
if "%SOC%"=="iMX6" (
    mkdir %PKG_ROOT%\Audio >NUL 2>NUL
    copy %BUILD_ROOT%\imx6aud\* %PKG_ROOT%\Audio\ >NUL
    if errorlevel 1 (set FAILURE=imx6aud & goto ERROR)
    copy %REPO_ROOT%\driver\audio\controller\imx6aud\imx6aud.wm.xml %PKG_ROOT%\Audio\ >NUL
)
if "%SOC%"=="iMX7" (
    copy %BUILD_ROOT%\imxaud\* %PKG_ROOT%\Audio\ >NUL
    if errorlevel 1 (set FAILURE=imxaud & goto ERROR)
    copy %REPO_ROOT%\driver\audio\controller\imxaud\imxaud.wm.xml %PKG_ROOT%\Audio\ >NUL
)

mkdir %PKG_ROOT%\Display >NUL 2>NUL
copy %BUILD_ROOT%\imx6dod\* %PKG_ROOT%\Display\ >NUL
if errorlevel 1 (set FAILURE=imx6dod & goto ERROR)
copy %REPO_ROOT%\driver\video\imx6dod\imx6dod.wm.xml %PKG_ROOT%\Display\ >NUL

mkdir %PKG_ROOT%\GPIO >NUL 2>NUL
copy %BUILD_ROOT%\imxgpio\* %PKG_ROOT%\GPIO\ >NUL
if errorlevel 1 (set FAILURE=imxgpio & goto ERROR)
copy %REPO_ROOT%\driver\gpio\imxgpio\imxgpio.wm.xml %PKG_ROOT%\GPIO\ >NUL

mkdir %PKG_ROOT%\I2C >NUL 2>NUL
copy %BUILD_ROOT%\imxi2c\* %PKG_ROOT%\I2C\ >NUL
if errorlevel 1 (set FAILURE=imxi2c & goto ERROR)
copy %REPO_ROOT%\driver\i2c\imxi2c\imxi2c.wm.xml %PKG_ROOT%\I2C\ >NUL

mkdir %PKG_ROOT%\Net >NUL 2>NUL
copy %BUILD_ROOT%\imxnetmini\* %PKG_ROOT%\Net\ >NUL
if errorlevel 1 (set FAILURE=imxnetmini & goto ERROR)
copy %REPO_ROOT%\driver\net\ndis\imxnetmini\imxnetmini.wm.xml %PKG_ROOT%\Net\ >NUL

mkdir %PKG_ROOT%\UART >NUL 2>NUL
copy %BUILD_ROOT%\imxuart\* %PKG_ROOT%\UART\ >NUL
if errorlevel 1 (set FAILURE=imxuart & goto ERROR)
copy %REPO_ROOT%\driver\serial\imxuart\imxuart.wm.xml %PKG_ROOT%\UART\ >NUL

mkdir %PKG_ROOT%\SPI >NUL 2>NUL
copy %BUILD_ROOT%\imxecspi\* %PKG_ROOT%\SPI\ >NUL
if errorlevel 1 (set FAILURE=imxecspi & goto ERROR)
copy %REPO_ROOT%\driver\spi\imxecspi\imxecspi.wm.xml %PKG_ROOT%\SPI\ >NUL

mkdir %PKG_ROOT%\PWM >NUL 2>NUL
copy %BUILD_ROOT%\imxpwm\* %PKG_ROOT%\PWM\ >NUL
if errorlevel 1 (set FAILURE=imxpwm & goto ERROR)
copy %REPO_ROOT%\driver\pwm\imxpwm\imxpwm.wm.xml %PKG_ROOT%\PWM\ >NUL

mkdir %PKG_ROOT%\USDHC >NUL 2>NUL
copy %BUILD_ROOT%\imxusdhc\* %PKG_ROOT%\USDHC\ >NUL
if errorlevel 1 (set FAILURE=imxusdhc & goto ERROR)
copy %REPO_ROOT%\driver\sd\imxusdhc\imxusdhc.wm.xml %PKG_ROOT%\USDHC\ >NUL

mkdir %PKG_ROOT%\Power >NUL 2>NUL
copy %BUILD_ROOT%\mx6pep\* %PKG_ROOT%\Power\ >NUL
if errorlevel 1 (set FAILURE=mx6pep & goto ERROR)
copy %REPO_ROOT%\driver\power\imx6pep\sys\mx6pep.wm.xml %PKG_ROOT%\Power\ >NUL

mkdir %PKG_ROOT%\OPTEE >NUL 2>NUL
copy %BUILD_ROOT%\OpteeTrEE\* %PKG_ROOT%\OPTEE\ >NUL
if errorlevel 1 (set FAILURE=OpteeTrEE & goto ERROR)
copy %REPO_ROOT%\driver\TrEE\TrEE\OpteeTrEE.wm.xml %PKG_ROOT%\OPTEE\ >NUL

:: Copy Codec Drivers
echo Copying Codec Driver Packages to %PKG_ROOT%\Audio
copy %BUILD_ROOT%\wm8962codec\* %PKG_ROOT%\Audio\ >NUL
if errorlevel 1 (set FAILURE=wm8962 & goto ERROR)
copy %REPO_ROOT%\driver\audio\codec\wm8962\wm8962codec.wm.xml %PKG_ROOT%\Audio\ >NUL

copy %BUILD_ROOT%\Wm8731Lcodec\* %PKG_ROOT%\Audio\ >NUL
if errorlevel 1 (set FAILURE=wm8731L & goto ERROR)
copy %REPO_ROOT%\driver\audio\codec\wm8731L\wm8731Lcodec.wm.xml %PKG_ROOT%\Audio\ >NUL

:: Copy HAL Extension Packages
echo Copying HAL Extension Packages to %PKG_ROOT%
if "%SOC%"=="iMX6" (
    mkdir %PKG_ROOT%\HalExtTimers >NUL 2>NUL
    copy %BUILD_ROOT%\HalExtiMX6Timers\* %PKG_ROOT%\HalExtTimers\ >NUL
    if errorlevel 1 (set FAILURE=HalExtiMX6Timers & goto ERROR)
    copy %REPO_ROOT%\hals\halext\HalExtiMX6Timers\HalExtiMX6Timers.wm.xml %PKG_ROOT%\HalExtTimers\ >NUL

    mkdir %PKG_ROOT%\HalExtDma >NUL 2>NUL
    copy %BUILD_ROOT%\HalExtiMXDma\* %PKG_ROOT%\HalExtDma\ >NUL
    if errorlevel 1 (set FAILURE=HalExtiMXDma & goto ERROR)
    copy %REPO_ROOT%\hals\halext\HalExtiMXDma\HalExtiMXDma.wm.xml %PKG_ROOT%\HalExtDma\ >NUL
)
if "%SOC%"=="iMX7" (
    mkdir %PKG_ROOT%\HalExtTimers >NUL 2>NUL
    copy %BUILD_ROOT%\HalExtiMX7Timers\* %PKG_ROOT%\HalExtTimers\ >NUL
    if errorlevel 1 (set FAILURE=HalExtiMX7Timers & goto ERROR)
    copy %REPO_ROOT%\hals\halext\HalExtiMX7Timers\HalExtiMX7Timers.wm.xml %PKG_ROOT%\HalExtTimers\ >NUL
)

:: Copy Firmware Packages
echo Copying Firmware Packages to %PKG_ROOT%
mkdir %PKG_ROOT%\BootFirmware\BootFirmware >NUL 2>NUL
copy %INFO_ROOT%\BootFirmware\BootFirmware.wm.xml %PKG_ROOT%\BootFirmware\ >NUL
copy %INFO_ROOT%\BootFirmware\uefi.fit %PKG_ROOT%\BootFirmware\BootFirmware\ >NUL

mkdir %PKG_ROOT%\BootLoader\BootLoader >NUL 2>NUL
copy %INFO_ROOT%\BootLoader\BootLoader.wm.xml %PKG_ROOT%\BootLoader\ >NUL
copy %INFO_ROOT%\BootLoader\firmware_fit.merged %PKG_ROOT%\BootLoader\BootLoader\ >NUL

:: Copy the UpdateOS Package XML
mkdir %PKG_ROOT%\SVPlatExtensions >NUL 2>NUL
copy %INFO_ROOT%\SVPlatExtensions\BSP.svupdateOS.wm.xml %PKG_ROOT%\SVPlatExtensions\svupdateOS.wm.xml >NUL

:: Copy Platform Information Packages
echo Copying Platform Information Packages to %PKG_ROOT%
mkdir %PKG_ROOT%\SystemInformation >NUL 2>NUL
copy %INFO_ROOT%\SystemInformation\* %PKG_ROOT%\SystemInformation\ >NUL

mkdir %PKG_ROOT%\DevicePlatform\OEMDevicePlatform >NUL 2>NUL
mkdir %PKG_ROOT%\DevicePlatform\OEMDevicePlatformMin2GB >NUL 2>NUL
copy %INFO_ROOT%\OEMDevicePlatform\OEMDevicePlatform.wm.xml %PKG_ROOT%\DevicePlatform\ >NUL
copy %INFO_ROOT%\OEMDevicePlatform\DevicePlatform.xml %PKG_ROOT%\DevicePlatform\OEMDevicePlatform >NUL
copy %INFO_ROOT%\OEMDevicePlatformMin2GB\OEMDevicePlatformMin2GB.wm.xml %PKG_ROOT%\DevicePlatform\ >NUL
copy %INFO_ROOT%\OEMDevicePlatformMin2GB\DevicePlatform.xml %PKG_ROOT%\DevicePlatform\OEMDevicePlatformMin2GB >NUL

mkdir %PKG_ROOT%\DeviceLayout\DeviceLayoutProd >NUL 2>NUL
mkdir %PKG_ROOT%\DeviceLayout\DeviceLayoutProdMin2GB >NUL 2>NUL
echo %INFO_ROOT%\DeviceLayoutProd\DeviceLayoutProd.wm.xml %PKG_ROOT%\DeviceLayout\
copy %INFO_ROOT%\DeviceLayoutProd\DeviceLayoutProd.wm.xml %PKG_ROOT%\DeviceLayout\ >NUL
echo %INFO_ROOT%\DeviceLayoutProd\devicelayoutprod.xml %PKG_ROOT%\DeviceLayout\DeviceLayoutProd
copy %INFO_ROOT%\DeviceLayoutProd\devicelayoutprod.xml %PKG_ROOT%\DeviceLayout\DeviceLayoutProd >NUL
copy %INFO_ROOT%\DeviceLayoutProdMin2GB\DeviceLayoutProdMin2GB.wm.xml %PKG_ROOT%\DeviceLayout\ >NUL
copy %INFO_ROOT%\DeviceLayoutProdMin2GB\devicelayoutprodmin2gb.xml %PKG_ROOT%\DeviceLayout\DeviceLayoutProdMin2GB >NUL

exit /b 0

:ARM64

:: Copy Driver Packages
echo Copying Driver Packages to %PKG_ROOT%
mkdir %PKG_ROOT%\Audio >NUL 2>NUL
copy %BUILD_ROOT%\imxaud\* %PKG_ROOT%\Audio\ >NUL
if errorlevel 1 (set FAILURE=imxaud & goto ERROR)
copy %REPO_ROOT%\driver\audio\controller\imxaud\imxaud.wm.xml %PKG_ROOT%\Audio\ >NUL

mkdir %PKG_ROOT%\GPIO >NUL 2>NUL
copy %BUILD_ROOT%\imxgpio\* %PKG_ROOT%\GPIO\ >NUL
if errorlevel 1 (set FAILURE=imxgpio & goto ERROR)
copy %REPO_ROOT%\driver\gpio\imxgpio\imxgpio.wm.xml %PKG_ROOT%\GPIO\ >NUL

mkdir %PKG_ROOT%\I2C >NUL 2>NUL
copy %BUILD_ROOT%\imxi2c\* %PKG_ROOT%\I2C\ >NUL
if errorlevel 1 (set FAILURE=imxi2c & goto ERROR)
copy %REPO_ROOT%\driver\i2c\imxi2c\imxi2c.wm.xml %PKG_ROOT%\I2C\ >NUL

mkdir %PKG_ROOT%\Net >NUL 2>NUL
copy %BUILD_ROOT%\imxnetmini\* %PKG_ROOT%\Net\ >NUL
if errorlevel 1 (set FAILURE=imxnetmini & goto ERROR)
copy %REPO_ROOT%\driver\net\ndis\imxnetmini\imxnetmini.wm.xml %PKG_ROOT%\Net\ >NUL

mkdir %PKG_ROOT%\UART >NUL 2>NUL
copy %BUILD_ROOT%\imxuart\* %PKG_ROOT%\UART\ >NUL
if errorlevel 1 (set FAILURE=imxuart & goto ERROR)
copy %REPO_ROOT%\driver\serial\imxuart\imxuart.wm.xml %PKG_ROOT%\UART\ >NUL

mkdir %PKG_ROOT%\SPI >NUL 2>NUL
copy %BUILD_ROOT%\imxecspi\* %PKG_ROOT%\SPI\ >NUL
if errorlevel 1 (set FAILURE=imxecspi & goto ERROR)
copy %REPO_ROOT%\driver\spi\imxecspi\imxecspi.wm.xml %PKG_ROOT%\SPI\ >NUL

mkdir %PKG_ROOT%\PWM >NUL 2>NUL
copy %BUILD_ROOT%\imxpwm\* %PKG_ROOT%\PWM\ >NUL
if errorlevel 1 (set FAILURE=imxpwm & goto ERROR)
copy %REPO_ROOT%\driver\pwm\imxpwm\imxpwm.wm.xml %PKG_ROOT%\PWM\ >NUL

mkdir %PKG_ROOT%\USDHC >NUL 2>NUL
copy %BUILD_ROOT%\imxusdhc\* %PKG_ROOT%\USDHC\ >NUL
if errorlevel 1 (set FAILURE=imxusdhc & goto ERROR)
copy %REPO_ROOT%\driver\sd\imxusdhc\imxusdhc.wm.xml %PKG_ROOT%\USDHC\ >NUL

mkdir %PKG_ROOT%\OPTEE >NUL 2>NUL
copy %BUILD_ROOT%\OpteeTrEE\* %PKG_ROOT%\OPTEE\ >NUL
if errorlevel 1 (set FAILURE=OpteeTrEE & goto ERROR)
copy %REPO_ROOT%\driver\TrEE\TrEE\OpteeTrEE.wm.xml %PKG_ROOT%\OPTEE\ >NUL

:: Copy Components
echo Copying components to %PKG_ROOT%
mkdir %PKG_ROOT%\Arm64CrtRuntime >NUL 2>NUL
copy %BUILD_ROOT%\Arm64CrtRuntime\* %PKG_ROOT%\Arm64CrtRuntime\ >NUL
if errorlevel 1 (set FAILURE=Arm64CrtRuntime & goto ERROR)
copy %REPO_ROOT%\components\Arm64CrtRuntime\Arm64CrtRuntime.wm.xml %PKG_ROOT%\Arm64CrtRuntime\ >NUL

:: Copy HAL Extension Packages
echo Copying HAL Extension Packages to %PKG_ROOT%
mkdir %PKG_ROOT%\HalExtDma >NUL 2>NUL
copy %BUILD_ROOT%\HalExtiMXDma\* %PKG_ROOT%\HalExtDma\ >NUL
if errorlevel 1 (set FAILURE=HalExtiMXDma & goto ERROR)
copy %REPO_ROOT%\hals\halext\HalExtiMXDma\HalExtiMXDma.wm.xml %PKG_ROOT%\HalExtDma\ >NUL

:: Copy Firmware Packages
echo Copying Firmware Packages to %PKG_ROOT%
mkdir %PKG_ROOT%\BootFirmware\BootFirmware >NUL 2>NUL
copy %INFO_ROOT%\BootFirmware\BootFirmware.wm.xml %PKG_ROOT%\BootFirmware\ >NUL
copy %INFO_ROOT%\BootFirmware\uefi.fit %PKG_ROOT%\BootFirmware\BootFirmware\ >NUL

mkdir %PKG_ROOT%\BootLoader\BootLoader >NUL 2>NUL
copy %INFO_ROOT%\BootLoader\BootLoader.wm.xml %PKG_ROOT%\BootLoader\ >NUL
copy %INFO_ROOT%\BootLoader\firmware_fit.merged %PKG_ROOT%\BootLoader\BootLoader\ >NUL

:: Copy the UpdateOS Package XML
mkdir %PKG_ROOT%\SVPlatExtensions >NUL 2>NUL
copy %INFO_ROOT%\SVPlatExtensions\BSP.svupdateOS.wm.xml %PKG_ROOT%\SVPlatExtensions\svupdateOS.wm.xml >NUL

:: Copy Platform Information Packages
echo Copying Platform Information Packages to %PKG_ROOT%
mkdir %PKG_ROOT%\SystemInformation >NUL 2>NUL
copy %INFO_ROOT%\SystemInformation\* %PKG_ROOT%\SystemInformation\ >NUL

mkdir %PKG_ROOT%\DevicePlatform\OEMDevicePlatform >NUL 2>NUL
copy %INFO_ROOT%\OEMDevicePlatform\OEMDevicePlatform.wm.xml %PKG_ROOT%\DevicePlatform\ >NUL
copy %INFO_ROOT%\OEMDevicePlatform\DevicePlatform.xml %PKG_ROOT%\DevicePlatform\OEMDevicePlatform >NUL

mkdir %PKG_ROOT%\DeviceLayout\DeviceLayoutProd >NUL 2>NUL
echo %INFO_ROOT%\DeviceLayoutProd\DeviceLayoutProd.wm.xml %PKG_ROOT%\DeviceLayout\
copy %INFO_ROOT%\DeviceLayoutProd\DeviceLayoutProd.wm.xml %PKG_ROOT%\DeviceLayout\ >NUL
echo %INFO_ROOT%\DeviceLayoutProd\devicelayoutprod.xml %PKG_ROOT%\DeviceLayout\DeviceLayoutProd
copy %INFO_ROOT%\DeviceLayoutProd\devicelayoutprod.xml %PKG_ROOT%\DeviceLayout\DeviceLayoutProd >NUL

exit /b 0

:ERROR
echo BuildBSP.bat failed to copy %FAILURE%binaries to the BSP directory! Please ensure that the drivers have built in Visual Studio.
exit /b 1
