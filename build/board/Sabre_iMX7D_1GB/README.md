iMX7 Sabre
==============

## Info

iMX7 Sabre is a reference iMX7 platform. More information can be found [here](https://www.nxp.com/support/developer-resources/hardware-development-tools/sabre-development-system/sabre-board-for-smart-devices-based-on-the-i.mx-7dual-applications-processors:MCIMX7SABRE).


## Build Files

iMX7SabrePackage.vcxproj
  - This is a KMDF driver that generates a stub driver that is unsused. The primary role of this project is to generate board specific package like device layout and boot firmware for FFU generation. Board specific package should be added here.

iMX7SabreDeviceFM.xml
  - File manifest file for iMX7 Sabre. New BSP content would be added here.

iMX7SabreTestOEMInput.xml
  - The test OEM input file for iMX7 Sabre

iMX7SabreProductionOEMInput.xml
  - The production OEM input file for iMX7 Sabre
