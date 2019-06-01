iMX6SX Sabre
==============

## Info

iMX6SX Sabre is a reference iMX6SX platform. More information can be found [here](https://www.nxp.com/support/developer-resources/evaluation-and-development-boards/i.mx-evaluation-and-development-boards/sabre-board-for-smart-devices-based-on-the-i.mx-6solox-applications-processors:RD-IMX6SX-SABRE).


## Build Files

Sabre_iMX6SX_1GB_Package.vcxproj
  - This is a KMDF driver that generates a stub driver that is unsused. The primary role of this project is to generate board specific package like device layout and boot firmware for FFU generation. Board specific package should be added here.

Sabre_iMX6SX_1GB_DeviceFM.xml
  - File manifest file for iMX6SX Sabre. New BSP content would be added here.

Sabre_iMX6SX_1GB_TestOEMInput.xml
  - The test OEM input file for iMX6SX Sabre

Sabre_iMX6SX_1GB_ProductionOEMInput.xml
  - The production OEM input file for iMX6SX Sabre
