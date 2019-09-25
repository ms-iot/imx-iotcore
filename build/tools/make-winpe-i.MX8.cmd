::
:: Prepares a WinPE image for boot on i.MX 8M and i.MX 8M Mini.
::
@echo off
setlocal enableextensions disabledelayedexpansion

set WINPE_DIR=%ProgramFiles(x86)%\Windows Kits\10\Assessment and Deployment Kit\Windows Preinstallation Environment\arm64
set DEST=winpe_imx
set FFU_DISK_NUM=0
set WINPE_DRIVE_LETTER=D
set SCRIPT_DIR=%~dp0

:: Parse options
:GETOPTS
 if /I "%~1" == "/?" goto USAGE
 if /I "%~1" == "/Help" goto USAGE
 if /I "%~1" == "/builddir" set BUILD_DIR=%2& shift
 if /I "%~1" == "/firmware" set FIRMWARE_PATH=%2& shift
 if /I "%~1" == "/uefi" set UEFI_PATH=%2& shift
 if /I "%~1" == "/ffu" set FFU_PATH=%2& shift
 if /I "%~1" == "/ffudisk" set FFU_DISK_NUM=%2& shift
 if /I "%~1" == "/winpedrive" set WINPE_DRIVE_LETTER=%2& shift
 if /I "%~1" == "/apply" set DISK_NUM=%2& shift
 if /I "%~1" == "/clean" set CLEAN=1
 if /I "%~1" == "/winpedebug" set WINPE_DEBUG=1
 shift
if not (%1)==() goto GETOPTS

:: make WIM mount directory
rmdir /s /q mount > NUL 2>&1
mkdir mount
pushd mount
set MOUNT_DIR=%CD%
popd

:: make directory to hold packages
rmdir /s /q packages > NUL 2>&1
mkdir packages
pushd packages
set PACKAGES_DIR=%CD%
popd

if not "%CLEAN%" == "" goto CLEAN
if not "%DISK_NUM%" == "" goto APPLY

if "%BUILD_DIR%" == "" (
    echo Missing required option '/builddir'. Type /? for usage.
    exit /b 1
)

if "%FIRMWARE_PATH%" == "" (
    echo Missing required option '/firmware'. Type /? for usage.
    exit /b 1
)

if "%UEFI_PATH%" == "" (
    echo Missing required option '/uefi'. Type /? for usage.
    exit /b 1
)

:: make temporary WinPE directory
rmdir /s /q "%DEST%" > NUL 2>&1
mkdir "%DEST%"
mkdir "%DEST%\sources"

:: copy firmware files
echo Copying firmware from %FIRMWARE_PATH%
copy "%FIRMWARE_PATH%" "%DEST%" /y || goto err

echo Copying UEFI from %UEFI_PATH%
copy "%UEFI_PATH%" "%DEST%\uefi.fit" /y || goto err

:: copy winpe files
echo Creating WinPE image layout at %DEST%
echo Copying WinPE files from %WINPE_DIR%
if not exist "%WINPE_DIR%\Media" (
    echo You must install the Windows PE Add-on for the ADK
    echo https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/download-winpe--windows-pe
    goto err
)
xcopy /herky "%WINPE_DIR%\Media" "%DEST%\" || goto err
copy "%WINPE_DIR%\en-us\winpe.wim" "%DEST%\sources\boot.wim" || goto err
move "%DEST%\bootmgr.efi" "%DEST%\EFI\Microsoft\boot\" || goto err

:: BCD
set TARGET_BCD_STORE=%DEST%\EFI\Microsoft\boot\bcd

if not "%WINPE_DEBUG%" == "" (
    echo Enabling kernel debugging
    bcdedit /store "%TARGET_BCD_STORE%" /dbgsettings SERIAL DEBUGPORT:1 BAUDRATE:115200 || goto err
    bcdedit /store "%TARGET_BCD_STORE%" /debug {default} on || goto err
)

echo Enable boot test/flight signing
bcdedit /store "%TARGET_BCD_STORE%" /set {bootmgr} flightsigning on || goto err
bcdedit /store "%TARGET_BCD_STORE%" /set {bootmgr} testsigning on || goto err

echo Enable kernel test/flight signing...
bcdedit /store "%TARGET_BCD_STORE%" /set {default} testsigning on || goto err
bcdedit /store "%TARGET_BCD_STORE%" /set {default} flightsigning on || goto err

mkdir "%MOUNT_DIR%" > NUL 2>&1
echo Mounting WIM at %MOUNT_DIR%
dism /mount-wim /wimfile:"%DEST%\sources\boot.wim" /mountdir:"%MOUNT_DIR%" /index:1 || goto err

set STARTNET_CMD=%MOUNT_DIR%\Windows\System32\startnet.cmd
if NOT "%FFU_PATH%" == "" (
    echo Setting up FFU deployment to MMC
    copy "%FFU_PATH%" "%DEST%\Flash.ffu"

    echo Appending FFU flashing commands to %STARTNET_CMD%
    echo echo Deploying Flash.ffu to eMMC >> "%STARTNET_CMD%"
    echo dism /Apply-Image /ImageFile:%WINPE_DRIVE_LETTER%:\Flash.ffu /ApplyDrive:\\.\PhysicalDrive%FFU_DISK_NUM% /skipplatformcheck >> "%STARTNET_CMD%"
    echo rmdir /s /q %WINPE_DRIVE_LETTER%:\_efi >> "%STARTNET_CMD%"
    echo ren %WINPE_DRIVE_LETTER%:\efi _efi >> "%STARTNET_CMD%"
    echo echo Successfully flashed new image to eMMC. Restarting the device... >> "%STARTNET_CMD%"
    echo wpeutil reboot >> "%STARTNET_CMD%"
)

echo Copying driver packages to %PACKAGES_DIR%
call :GETPACKAGE NXP.iMX.HalExtiMXDma.cab || goto err
call :GETPACKAGE NXP.iMX.imxecspi.cab || goto err
call :GETPACKAGE NXP.iMX.imxgpio.cab || goto err
call :GETPACKAGE NXP.iMX.imxi2c.cab || goto err
call :GETPACKAGE NXP.iMX.imxnetmini.cab || goto err
call :GETPACKAGE NXP.iMX.imxuart.cab || goto err
call :GETPACKAGE NXP.iMX.imxusdhc.cab || goto err

echo Injecting drivers from %PACKAGES_DIR% into WIM
dism /image:"%MOUNT_DIR%" /Add-Package /packagepath:"%PACKAGES_DIR%" || goto err

echo Unmounting WIM
dism /unmount-wim /mountdir:"%MOUNT_DIR%" /commit || goto err
echo Success
exit /b 0

:GETPACKAGE
    setlocal enableextensions disabledelayedexpansion
    dir "%BUILD_DIR%\*%1" /s /b > %TMP%/temp.txt
    set /p PACKAGE_PATH=<%TMP%/temp.txt
    if "%PACKAGE_PATH%" == "" (
        echo Error: could not find %1 in %BUILD_DIR%
        exit /b 1
    )
    echo Copying %PACKAGE_PATH% to %PACKAGES_DIR%
    copy "%PACKAGE_PATH%" "%PACKAGES_DIR%"
    exit /b %ERRORLEVEL%

:APPLY
    echo Applying image at %DEST% to physical disk %DISK_NUM%
    if not exist "%DEST%" (
    echo No WinPE media directory found at %DEST%. Run the first form of this script to generate WinPE image layout.
        exit /b 1
    )

    echo select disk %DISK_NUM% > diskpart.txt
    echo clean >> diskpart.txt
    echo convert gpt NOERR >> diskpart.txt

    :: Firmware partition
    echo create partition primary id=07de03aa-f07c-4800-a28a-9b9ff76e9f47 size=2 offset=33 align=1 >> diskpart.txt
    :: UEFI partition
    echo create partition primary id=1d30adf8-0aef-4d83-b78c-ac719086c709 size=2 offset=2176 align=1 >> diskpart.txt
    :: EFI partition
    echo create partition primary id=c12a7328-f81f-11d2-ba4b-00a0c93ec93b size=32 offset=4224 align=1 >> diskpart.txt
    echo create partition primary >> diskpart.txt
    echo format fs=fat32 label="WinPE" quick >> diskpart.txt
    echo assign >> diskpart.txt
    echo assign mount="%MOUNT_DIR%" >> diskpart.txt

    echo Formatting disk %DISK_NUM% and mounting to %MOUNT_DIR%...
    diskpart /s diskpart.txt || goto err

    echo Copying files from %DEST% to %MOUNT_DIR%
    xcopy /herky "%DEST%\*.*" "%MOUNT_DIR%\" || goto err

    mountvol "%MOUNT_DIR%" /d
    timeout /t 10 /nobreak

    echo Writing firmware_fit.merged to \\.\PhysicalDrive%DISK_NUM%
    dd "if=%DEST%\flash.bin" of=\\.\PhysicalDrive%DISK_NUM% bs=512 seek=66 || goto err
    dd "if=%DEST%\uefi.fit" of=\\.\PhysicalDrive%DISK_NUM% bs=1024 seek=2176 || goto err
    echo Success
    exit /b 0

:CLEAN
    echo Cleaning up from previous run
    dism /unmount-wim /mountdir:"%MOUNT_DIR%" /discard > NUL 2>&1
    rmdir /s /q "%MOUNT_DIR%" 2> NUL
    rmdir /s /q "%DEST%" 2> NUL
    del diskpart.txt
    exit /b 0

:USAGE
    echo make-winpe.cmd /builddir build_dir /firmware firmware_fit_path
    echo   /uefi uefi_fit_path [/ffu ffu_path] [/ffudisk ffu_disk_number] 
    echo   [/winpedrive winpe_drive_letter] [/winpedebug]
    echo make-winpe.cmd /apply disk_number
    echo make-winpe.cmd /clean
    echo.
    echo Creates a WinPE image for i.MX
    echo Options:
    echo.
    echo    /builddir build_dir              Path to build output directory.
    echo    /firmware firmware_fit_path      Path to flash.bin.
    echo    /uefi uefi_fit_path              Path to uefi.fit.
    echo    /ffu ffu_path                    Optionally specify an FFU to flash.
    echo    /ffudisk ffu_disk_number         Optionally specify the physical disk.
    echo                                     number to which the FFU is applied.
    echo                                     Defaults to 0.
    echo    /winpedrive winpe_drive_letter   Optionally specify the drive letter where
    echo                                     WinPE is installed. Defaults to D.
    echo    /winpedebug                      Optionally enable debugging for WinPE.
    echo    /apply disk_number               Apply WinPE image to physical disk.
    echo    /clean                           Clean up artifacts from a previous run.
    echo.
    echo Examples:
    echo.
    echo Create a WinPE image.
    echo.
    echo    make-winpe.cmd /builddir d:\build\Binaries\release\ARM /firmware d:\build\FFU\HummingBoardEdge_iMX6Q_2GB\Package\BootLoader\firmware_fit.merged /uefi d:\build\FFU\HummingBoardEdge_iMX6Q_2GB\Package\BootFirmware\uefi.fit
    echo.
    echo Create a WinPE image that deploys an FFU to MMC.
    echo.
    echo    make-winpe.cmd /builddir d:\build\Binaries\release\ARM /firmware d:\build\FFU\HummingBoardEdge_iMX6Q_2GB\Package\BootLoader\firmware_fit.merged /uefi d:\build\FFU\HummingBoardEdge_iMX6Q_2GB\Package\BootFirmware\uefi.fit /ffu d:\build\FFU\HummingBoardEdge_iMX6Q_2GB\HummingBoardEdge_iMX6Q_2GB_TestOEMInput.xml.Release.ffu
    echo.
    echo Apply the WinPE image to an SD card (Physical Disk 7, use diskpart
    echo to find the disk number)
    echo.
    echo    make-winpe.cmd /apply 7
    echo.
    echo Clean up artifacts from a previous run of this script
    echo.
    echo    make-winpe.cmd /clean
    echo.
    exit /b 0

:err
    echo Script failed! Cleaning up
    dism /unmount-wim /mountdir:"%MOUNT_DIR%" /discard
    mountvol "%MOUNT_DIR%" /d > NUL 2>&1
    exit /b 1

