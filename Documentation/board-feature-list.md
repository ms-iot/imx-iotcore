Feature List Per Board
=======

This is an overview of the currently supported features for each of the boards.  As features are enabled and boards are added, please update this table.

|Legend| Meaning|
|---|---|
| Y | Enabled |
| N | Not Enabled |
| I/P | In Progress |
| N/A | Feature not applicable |

| Feature | i.MX6Q HMB Edge | i.MX6Q/ i.MX6QP Sabre | i.MX6DL HMB Edge | i.MX6S HMB Edge | i.MX6SX Sabre | i.MX6SX UDOO Neo Full | i.MX6ULL EVK | i.MX7D CL IoT Gate | i.MX7D Sabre | i.MX8M EVK | i.MX8M Mini EVK |
|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|
| Audio    | Y   | N   | Y   | Y   | Y   | N   | N   | Y   | N   | Y   | N   |
| GPIO     | Y   | Y   | Y   | Y   | Y   | Y   | Y   | Y   | Y   | Y   | Y |
| I2C      | Y   | Y   | Y   | Y   | Y   | Y   | Y   | Y   | N   | Y   | Y |
| Ethernet | Y   | Y   | Y   | Y   | Y   | Y   | Y   | Y   | N   | Y   | Y   |
| PWM      | Y   | Y   | Y   | Y   | Y   | Y   | Y   | Y   | N   | N   | N   |
| SDHC     | Y   | Y   | Y   | Y   | Y   | Y   | Y   | Y   | Y   | Y   | Y   |
| SPI      | Y   | Y   | Y   | Y   | Y   | Y   | Y   | Y   | N   | N   | Y   |
| Display  | Y   | Y   | Y   | Y   | Y   | Y   | N   | Y   | N   | Y   | Y   |
| UART     | Y (w/DMA)   | Y (w/DMA)   | Y   | Y   | Y   | Y   | Y   | Y   | N   | Y\*  | Y   |
| USB      | Y   | Y   | Y   | Y   | Y   | Y   | Y   | Y   | N   | Y   | Y   |
| PCIe     | Y   | Y   | Y   | N   | Y   | N   | N   | N   | N   | N   | N   |
| TrEE     | Y   | Y   | Y   | N   | Y   | N   | N   | Y   | N   | Y   | N   |
| M4       | N/A | N/A | N/A | N/A | N   | N   | N   | N   | N   | N   | N   |
| Processor power management      | Y   | Y   | Y   | Y   | Y   | N   | N   | Y   | Y   | Y   | Y   |
| Device power management      | Y   | Y   | Y   | Y   | N   | N   | N   | N   | N   | N   | N   |
| Low power standby      | N   | N   | N   | N   | N   | N   | N   | N   | N   | N   | N   |
| Display power management      | Y   | Y   | Y   | Y   | Y   | N   | N   | N   | N   | N   | N   |


Not all features of a given subsystem maybe fully enabled and/or optimized. If you encounter issues with supported features, please open an issue.

\* To enable the UART, the kernel debugger must be disabled by running the following command on the device and rebooting. The UART exposed is the same UART that the kernel debugger uses.
```
bcdedit /debug off
```
