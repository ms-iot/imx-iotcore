:: Copyright (c) Microsoft Corporation. All rights reserved.
:: Licensed under the MIT License.

set BATCH_HOME=%~dp0
call ..\..\..\tools\SetupBuildEnv.bat


if "%PLATFORM%"=="ARM" (

    REM i.MX6 Platforms

    cd /d %BATCH_HOME%
    echo "Building HummingBoardEdge_iMX6Q_2GB FFU"
    call BuildImage HummingBoardEdge_iMX6Q_2GB HummingBoardEdge_iMX6Q_2GB_TestOEMInput.xml

    REM cd /d %BATCH_HOME%
    REM echo "Building Sabre_iMX6Q_1GB FFU"
    REM call BuildImage Sabre_iMX6Q_1GB Sabre_iMX6Q_1GB_TestOEMInput.xml

    REM cd /d %BATCH_HOME%
    REM echo "Building SabreLite_iMX6Q_1GB FFU"
    REM call BuildImage SabreLite_iMX6Q_1GB SabreLite_iMX6Q_1GB_TestOEMInput.xml

    REM cd /d %BATCH_HOME%
    REM echo "Building Sabre_iMX6QP_1GB FFU"
    REM call BuildImage Sabre_iMX6QP_1GB Sabre_iMX6QP_1GB_TestOEMInput.xml

    REM cd /d %BATCH_HOME%
    REM echo "Building HummingBoardEdge_iMX6S_512MB FFU"
    REM call BuildImage HummingBoardEdge_iMX6S_512MB HummingBoardEdge_iMX6S_512MB_TestOEMInput.xml

    REM cd /d %BATCH_HOME%
    REM echo "Building HummingBoardEdge_iMX6DL_1GB FFU"
    REM call BuildImage HummingBoardEdge_iMX6DL_1GB HummingBoardEdge_iMX6DL_1GB_TestOEMInput.xml

    REM cd /d %BATCH_HOME%
    REM echo "Building UdooNeo_iMX6SX_1GB FFU"
    REM call BuildImage UdooNeo_iMX6SX_1GB UdooNeo_iMX6SX_1GB_TestOEMInput.xml

    REM cd /d %BATCH_HOME%
    REM echo "Building VAB820_iMX6Q_1GB FFU"
    REM call BuildImage VAB820_iMX6Q_1GB VAB820_iMX6Q_1GB_TestOEMInput.xml

    REM i.MX7 Platforms

    REM cd /d %BATCH_HOME%
    REM echo "Building ClSomImx7_iMX7D_1GB FFU"
    REM call BuildImage ClSomImx7_iMX7D_1GB ClSomImx7_iMX7D_1GB_TestOEMInput.xml

    REM cd /d %BATCH_HOME%
    REM echo "Building Sabre_iMX7D_1GB FFU"
    REM call BuildImage Sabre_iMX7D_1GB Sabre_iMX7D_1GB_TestOEMInput.xml
)

if "%PLATFORM%"=="ARM64" (

    REM i.MX8M Platforms

    cd /d %BATCH_HOME%
    echo "Building NXPEVK_iMX8M_4GB FFU"
    call BuildImage NXPEVK_iMX8M_4GB NXPEVK_iMX8M_4GB_TestOEMInput.xml

    REM cd /d %BATCH_HOME%
    REM echo "Building NXPEVK_iMX8M_Mini_2GB FFU"
    REM call BuildImage NXPEVK_iMX8M_Mini_2GB NXPEVK_iMX8M_Mini_2GB_TestOEMInput.xml
)
