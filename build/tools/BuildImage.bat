:: Copyright (c) Microsoft Corporation. All rights reserved.
:: Licensed under the MIT License.

@echo off

echo ++BuildImage.bat
echo SET board name and OEM input file
REM
SET BOARD_NAME=%1
SET INPUT_FILE_NAME=%2
SET FFU_FILE_NAME=%3

if "%BOARD_NAME%"=="?" (
    goto :USAGE
)

if "%BOARD_NAME%"=="" (
    echo Please specify a boardname
    goto :USAGE
)
echo BOARD_NAME="%BOARD_NAME%"

if "%INPUT_FILE_NAME%"=="" (
    echo Please specify a OEM input file name
    goto :USAGE
)
echo INPUT_FILE_NAME="%INPUT_FILE_NAME%"

set BOARD_BUILD_PATH=%REPO_BUILD_ROOT%\board\%BOARD_NAME%
echo BOARD_BUILD_PATH=%BOARD_BUILD_PATH%

:: Enable OEMInput reuse for output BSPs
set BLD_DIR=%BOARD_BUILD_PATH%
echo BLD_DIR=%BOARD_BUILD_PATH%

if not exist %BOARD_BUILD_PATH% (
    echo %BOARD_BUILD_PATH%
    echo %BOARD_NAME% folder does not exist in %BOARD_BUILD_PATH%
    goto :EXIT
)
echo BOARD_BUILD_PATH =%BOARD_BUILD_PATH%

if not exist %BOARD_BUILD_PATH%\%INPUT_FILE_NAME% (
    echo %INPUT_FILE_NAME% input file does not exist in %BOARD_BUILD_PATH%
    goto :EXIT
)
echo BOARD_BUILD_PATH\INPUT_FILE_NAME=%BOARD_BUILD_PATH%\%INPUT_FILE_NAME%

if "%Configuration%"=="" (
    SET Configuration=Debug
)

if "%Configuration%"=="iMX6-Release" (
    SET Configuration=Release
)

if "%Configuration%"=="iMX6-Debug" (
    SET Configuration=Debug
)

if "%Configuration%"=="iMX7-Release" (
    SET Configuration=Release
)

if "%Configuration%"=="iMX7-Debug" (
    SET Configuration=Debug
)

if "%FFU_FILE_NAME%"=="" (
    SET FFU_FILE_NAME=%INPUT_FILE_NAME%.%Configuration%.ffu
)

REM if "%OutDir%"=="" (
    REM echo Default to board folder if no output folder is specified
    REM SET FFU_FILE_NAME=%BOARD_BUILD_PATH%\%FFU_FILE_NAME%
REM ) else (
    REM echo Use default output directory
    REM SET FFU_FILE_NAME=%OUTDIR%%FFU_FILE_NAME%
REM )

SET FFU_FILE_NAME=%REPO_BUILD_ROOT%\solution\iMXPlatform\Build\FFU\%BOARD_NAME%\%FFU_FILE_NAME%

if "%SolutionDir%"=="" (
    SET SolutionDir=%BOARD_BUILD_PATH%\
	echo SolutionDir=%SolutionDir%
)
REM
echo Export packages to one directory
REM
set BSPPKG_DIR=%REPO_BUILD_ROOT%\solution\iMXPlatform\Build\FFU\bspcabs\%PLATFORM%\%Configuration%
set PKG_VER=1.0.0.0
if not exist %BSPPKG_DIR% ( mkdir %BSPPKG_DIR% )

if not exist %REPO_BUILD_ROOT%\..\..\b\%Configuration%\%PLATFORM% (dir /S /B %REPO_BUILD_ROOT%\solution\iMXPlatform\Build\%PLATFORM%\%Configuration%\*.cab > filelist.txt)
if exist %REPO_BUILD_ROOT%\..\..\b\%Configuration%\%PLATFORM% (dir /S /B %REPO_BUILD_ROOT%\..\..\b\%Configuration%\%PLATFORM%\*.cab > filelist.txt)

REM Append the %BOARD_NAME% folder to the end of the scrape list so the correct SV.PlatExtensions.UpdateOS.cab is in bspcabs
if not exist %REPO_BUILD_ROOT%\..\..\b\%Configuration%\%PLATFORM% (dir /S /B %REPO_BUILD_ROOT%\solution\iMXPlatform\Build\%PLATFORM%\%Configuration%\%BOARD_NAME%\*.cab >> filelist.txt)
if exist %REPO_BUILD_ROOT%\..\..\b\%Configuration%\%PLATFORM% (dir /S /B %REPO_BUILD_ROOT%\..\..\b\%Configuration%\%PLATFORM%\%BOARD_NAME%\*.cab >> filelist.txt)

for /f "delims=" %%i in (filelist.txt) do (
    echo %%i
    copy "%%i" "%BSPPKG_DIR%" >nul 2>nul
)
del filelist.txt >nul
REM
echo Run Feature Merger
REM
FeatureMerger %BOARD_BUILD_PATH%\%BOARD_NAME%_FMFileList.xml %BSPPKG_DIR% %PKG_VER% %BOARD_BUILD_PATH%\MergedFMs /InputFMDir:%BOARD_BUILD_PATH%\InputFMs /Languages:en-us /Resolutions:1024x768 /ConvertToCBS /variables:_cputype=%PLATFORM%;buildtype=fre;releasetype=production >fmlog.txt
del %BSPPKG_DIR%\*.spkg >nul 2>nul
del %BSPPKG_DIR%\*.merged.txt >nul 2>nul

REM
echo Begin Image generation
REM
echo building image
echo imggen.cmd "%FFU_FILE_NAME%" "%BOARD_BUILD_PATH%\%INPUT_FILE_NAME%" "%KITSROOT%MSPackages" %PLATFORM%
call imggen.cmd "%FFU_FILE_NAME%" "%BOARD_BUILD_PATH%\%INPUT_FILE_NAME%" "%KITSROOT%MSPackages" %PLATFORM%
goto :EXIT

:USAGE
echo.
echo BuildImage ^<Board Name^> ^<OEM Input File^> ^<FFU Filename^>
echo.
echo    Boardname - The name of the board to be build
echo    OEMInputFile - The name of the OEM input file
echo    FFU Filename - FFU filename (Optional)
echo.
echo Example: BuildImage HummingBoard TestOEMInput.xml
echo          BuildImage HummingBoard TestOEMInput.xml Flash.ffu
echo.
echo --BuildImage.bat
:EXIT

if errorlevel 1 (
    echo !!!!! If the error was a Feature Manifest XML error, please check %cd%\fmlog.txt !!!!!
)
