
iMX6 SabreLite
==============

## Info

iMX6 SabreLite (BD-SL-I.MX6) is a reference iMX6 platform from Boundary Devices Inc. More information can be found [here](https://boundarydevices.com/product/sabre-lite-imx6-sbc/).


## Build Files

iMX6SabreLitePackage.vcxproj
- This is a KMDF driver that generates a stub driver that is unsused. The primary role of this project is to generate board specific package like device layout and boot firmware for FFU generation. Board specific package should be added here.

iMX6SabreLiteDeviceFM.xml - File manifest file for iMX6 Sabre. New BSP content would be added here.

iMX6SabreLiteTestOEMInput.xml - The test OEM input file for iMX6 Sabre

iMX6SabreLiteProductionOEMInput.xml - The production OEM input file for iMX6 Sabre
### Notes:
1. Board does NOT have onboard FTDI serial-to-USB for console connection so you WILL NOT have good results running baudrate at 921600. Use the standard 115200 baudrate for best results.
2. Board has factory blown boot fuses to only boot from SPI serial flash eprom. Therefore you must flash firmware_fit.merged bootloader into the serial flash.
3. Factory u-boot console UART port is assigned to UART2 (sometimes marked CONSOLE on breakout cable). WIN IoT u-boot uses UART1.
4. Important- Don't forget to look for output on UART1 after flashing using factory u-boot!
#### To flash bootloader do the following:
1. Format a full size SD card with FAT
2. Place the firmware_fit.merged binary on the card.
3. Place card in full size SD slot.
4. Boot up u-boot (if using factory u-boot output is on UART2, if WIN IoT u-boot output on UART1).
5. Erase and reprogram the serial flash using the following u-boot commands:
 * mmc dev
 * fatload mmc 0:1 0x10800000 firmware_fit.merged
 * sf probe
 * sf erase 0 0xc0000
 * sf write 0x10800000 0x400 ${filesize}
6. Reboot and look for output on UART1!
7. Replace SD with card programmed with WIN IoT ffu image.
8. Reboot again.



There may be other ways to do this ... see the Boundary Devices support pages for more info.








