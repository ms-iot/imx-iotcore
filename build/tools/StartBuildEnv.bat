:: Copyright (c) Microsoft Corporation. All rights reserved.
:: Licensed under the MIT License.

@echo off

call SetupBuildEnv

REM
REM Start console
REM

cd /d "%REPO_SOURCE_ROOT%"
start cmd.exe
