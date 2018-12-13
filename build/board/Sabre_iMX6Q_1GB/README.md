iMX6 Sabre
==============

## Info

iMX6 Sabre is a reference iMX6 platform. More information can be found [here](https://www.nxp.com/support/developer-resources/hardware-development-tools/sabre-development-system/sabre-platform-for-smart-devices-based-on-the-i.mx-6-series:RDIMX6SABREPLAT).


## Build Files

iMX6SabrePackage.vcxproj
  - This is a KMDF driver that generates a stub driver that is unsused. The primary role of this project is to generate board specific package like device layout and boot firmware for FFU generation. Board specific package should be added here.

iMX6SabreDeviceFM.xml
  - File manifest file for iMX6 Sabre. New BSP content would be added here.

iMX6SabreTestOEMInput.xml
  - The test OEM input file for iMX6 Sabre

iMX6SabreProductionOEMInput.xml
  - The production OEM input file for iMX6 Sabre
