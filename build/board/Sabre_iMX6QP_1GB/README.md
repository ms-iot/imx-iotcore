iMX6QP Sabre
==============

## Info

iMX6QP Sabre is a reference iMX6QP platform. More information can be found [here](https://www.nxp.com/support/developer-resources/hardware-development-tools/sabre-development-system/sabre-board-for-smart-devices-based-on-the-i.mx-6quadplus-applications-processors:RD-IMX6QP-SABRE).


## Build Files

iMX6QPSabrePackage.vcxproj
  - This is a KMDF driver that generates a stub driver that is unsused. The primary role of this project is to generate board specific package like device layout and boot firmware for FFU generation. Board specific package should be added here.

iMX6QPSabreDeviceFM.xml
  - File manifest file for iMX6QP Sabre. New BSP content would be added here.

iMX6QPSabreTestOEMInput.xml
  - The test OEM input file for iMX6QP Sabre

iMX6QPSabreProductionOEMInput.xml
  - The production OEM input file for iMX6QP Sabre
