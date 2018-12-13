:: Copyright (c) Microsoft Corporation. All rights reserved.
:: Licensed under the MIT License.

set CURR_DIR=%~dp0
set PATH=%CURR_DIR%;%PATH%
call SetupBuildEnv.bat

REM
REM Install OEM Test Cert
REM

set W10_KITROOT=%KITSROOT%
set WPDKCONTENTROOT=%W10_KITROOT%
set W10_TOOL=%W10_KITROOT%\bin\x86;%W10_KITROOT%\Tools\bin\i386
set PATH=%PATH%;%W10_TOOL%;
call Installoemcerts.cmd
