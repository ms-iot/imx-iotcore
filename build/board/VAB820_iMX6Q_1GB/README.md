iMX6 VAB-820
==============

## Info

iMX6 VAB-820 is a reference iMX6 platform. More information can be found [here] (https://www.viatech.com/en/boards/pico-itx/vab-820/).

## Build Files

VAB820_iMX6Q_1GB_Package.vcxproj
  - This is a KMDF driver that generates a stub driver that is unsused. The primary role of this project is to generate board specific package like device layout and boot firmware for FFU generation. Board specific package should be added here.

VAB820_iMX6Q_1GB_FMFileList.xml
  - File manifest file for iMX6 VAB-820. New BSP content would be added here.

VAB820_iMX6Q_1GB_TestOEMInput.xml
  - The test OEM input file for iMX6 VAB-820

VAB820_iMX6Q_1GB_ProductionOEMInput.xml
  - The production OEM input file for iMX6 VAB-820
