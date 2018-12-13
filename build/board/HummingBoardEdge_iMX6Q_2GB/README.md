HummingBoard
==============

## Info

HummingBoard is an iMX6 based IoT platform. More information can be found [here] (https://www.solid-run.com/freescale-imx6-family/hummingboard/).

## Build Files

HummingBoardPackage.vcxproj
  - This is a KMDF driver that generates a stub driver that is unsused. The primary role of this project is to generate board specific package like device layout and boot firmware for FFU generation. Board specific package should be added here.
  
HummingBoardDeviceFM.xml
  - File manifest file for HummingBoard. New BSP content would be added here.
  
HummingBoardTestOEMInput.xml
  - The test OEM input file for HummingBoard
  
HummingBoardProductionOEMInput.xml
  - The production OEM input file for HummingBoard
