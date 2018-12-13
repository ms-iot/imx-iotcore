:: Copyright (c) Microsoft Corporation. All rights reserved.
:: Licensed under the MIT License.

@echo off

echo ++SetupBuildEnv.bat
echo Set source root
REM

set REPO_BUILD_TOOL=%~dp0
echo REPO_BUILD_TOOL=%REPO_BUILD_TOOL%
cd /d "%REPO_BUILD_TOOL%.."

set REPO_BUILD_ROOT=%cd%\
cd /d "%REPO_BUILD_ROOT%.."
echo REPO_BUILD_ROOT=%REPO_BUILD_ROOT%

set REPO_SOURCE_ROOT=%cd%\
echo REPO_SOURCE_ROOT=%REPO_SOURCE_ROOT%

REM
REM Reused the same script to reliably find ADK install location
REM
echo Query the 32-bit and 64-bit Registry hive for KitsRoot

set regKeyPathFound=1
set wowRegKeyPathFound=1
set KitsRootRegValueName=KitsRoot10

REG QUERY "HKLM\Software\Wow6432Node\Microsoft\Windows Kits\Installed Roots" /v %KitsRootRegValueName% 1>NUL 2>NUL || set wowRegKeyPathFound=0
REG QUERY "HKLM\Software\Microsoft\Windows Kits\Installed Roots" /v %KitsRootRegValueName% 1>NUL 2>NUL || set regKeyPathFound=0

if %wowRegKeyPathFound% EQU 0 (
  if %regKeyPathFound% EQU 0 (
    @echo KitsRoot not found, can't set common path for Deployment Tools
    goto :EOF
  ) else (
    set regKeyPath=HKLM\Software\Microsoft\Windows Kits\Installed Roots
  )
) else (
    set regKeyPath=HKLM\Software\Wow6432Node\Microsoft\Windows Kits\Installed Roots
)

FOR /F "skip=2 tokens=2*" %%i IN ('REG QUERY "%regKeyPath%" /v %KitsRootRegValueName%') DO (set KitsRoot=%%j)

REM
echo Setup basic environment path
echo KITSROOT=%KITSROOT%
REM
set PATH=%KITSROOT%tools\bin\i386;%REPO_BUILD_TOOL%;%PATH%
set AKROOT=%KITSROOT%

REM
echo Setup Deployment and Imaging Tools Environment
call "%KITSROOT%\Assessment and Deployment Kit\Deployment Tools\DandISetEnv.bat"
set SIGN_OEM=1
set SIGN_WITH_TIMESTAMP=0

cd /D "%REPO_BUILD_TOOL%"
echo --SetupBuildEnv.bat
