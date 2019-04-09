IoT Core Test Methods
================

This document walks through various tools available for testing the functionality and stability of new drivers for Windows IoT Core.

# Hardware Lab Kit
The Windows Hardware Lab kit (Windows HLK) is a test framework used to test hardware devices for Windows 10. A subset of the tests is available for use on Windows IoT Core devices.
## Prerequisites
 - A machine dedicated solely for HLK testing purposes running Windows Server 2008 R2, Windows Server 2012, Windows Server 2012 R2, or Windows Server 2016.
 - Network connectivity with the device under test.

## HLK Setup

**1. Set up the HLK server**:
 https://docs.microsoft.com/en-us/windows-hardware/test/hlk/getstarted/step-1-install-controller-and-studio-on-the-test-server 

**2. Machine Pool Setup**

   + Open HLK Studio on the HLK server.
   + Click Configuration in the top menu and navigate to the Machine Management tab.
   + Create and name a new machine pool for your device.

**3. WTT Proxy Setup**

In order to connect your IoT Core device to your HLK server, it will need to connect through a WTT Proxy service. The easiest way to do this is to setup the WTT Proxy on your HLK server directly.

(Full documentation: https://docs.microsoft.com/en-us/windows-hardware/test/hlk/getstarted/proxy-step-2--setup-an-hlk-proxy-system)

   + On your HLK server run \\\\localhost\HLKInstall\ProxyClient\setup.exe
   + After WWT Proxy setup completes, open an administrator command prompt and run
   ```cmd
   "%ProgramFiles(X86)%\WTTMobile\Client\WTTProxy.exe -console"
   ```
   + Ensure that network sharing and discovery is enabled on your HLK server

**4. Onboard your IoT Core device**

   + Open another administrator command prompt and navigate to WTTMobile\Tools
   ```cmd
   cd "%ProgramFiles(X86)%\WTTMobile\Tools"
   ```
   + Find the UniqueId by looking for your system's IP address in the output of:
   ```cmd
   KitsDeviceDetector.exe /rundevicediscovery
   REM example output: Name: 000EC687A555 | UniqueId: 00000000-0000-0000-0000-000ec687a555 | Address: 10.123.123.46 | Connection: SirepBroadcast2 | Location:
   REM Note: If you have multiple matching results and the first doesn't work, try the UniqueID that is mostly zeroes.
   ```
   ```powershell
   # You can also run the following in PowerShell to search for just your IP address
   ./KitsDeviceDetector.exe /rundevicediscovery | Select-String "10.123.123.46"
   ```
   + 	Modify the DeviceName, DeviceID, and machinepool values and run the following command to onboard the device. DeviceID = UniqueId from above, machinepool should match the name from step 2, DeviceName is a unique name of your choice.
   ```cmd
   KitsDeviceDetector.exe /Physical:Fake_PC.dll /DeviceName:HB_000EC687A555  /DeviceId:00000000-0000-0000-0000-000ec687a555 /machinepool:$\PoolName /SkipFFUCheck
   ```
   + Onboarding takes a few minutes, don’t be alarmed when it stalls on the FFU step.
   + If your device fails to onboard, ensure that the device and the proxy can ping each other, restart your Proxy and HLK server, and try again.
   + After your device is onboarded, close WTTProxy.exe and run it again as administrator.
   ```cmd
   "%ProgramFiles(X86)%\WTTMobile\Client\WTTProxy.exe -console"
   ```

## HLK Usage
**1. Running an HLK test**
   + Ensure that your WTTProxy console is running.
   + Open HLK Studio.
   + Create a new project.
   + Under the **Selection** tab, select the machine pool that you onboarded your device to.
     + If your system isn't visible in the systems tab, go to the configuration tab, select your machine pool, and right click your device to Change Machine Status to Ready.
   + Select individual devices in the **device manager** section, or the whole system in the **systems** section.
   + Select tests from the **Tests** tab, then run selected.


## Recommended devices to select in HLK device manager
  - Resource Hub proxy device
  - i.MX UART Device
  - i.MX I2C Device
  - i.MX GPIO Controller
  - i.MX ECSPI Controller
  - i.MX PWM Controller
  - i.MX Ultra Secure Digital Host Controller (uSDHC)
  - Microsoft Optee TrEE Device
  - i.MX Ethernet Adapter (NDIS 6.30)
  - IMX6 Display Only Driver
  - IMX6 Audio Device
  - Sgtl5000AudioCodec Device

## Recommended HLK tests
  - Selecting any of the devices in the above list will expose a set of Device Fundamentals tests prefixed with "DF". These tests are a good baseline for the general robustness of a Windows driver.
    - [Device Fundamentals tests](https://docs.microsoft.com/en-us/windows-hardware/drivers/devtest/device-fundamentals-tests)
  - Selecting "Resource Hub proxy device" will expose GPIO, I2C, and SPI stress tests that will test your low level IO exposure in Windows. Most of these tests require additional hardware, more information is available in the following documentation:
    - [GPIO WinRT tests](https://docs.microsoft.com/en-us/windows-hardware/test/hlk/testref/f1fc0922-1186-48bd-bfcd-c7385a2f6f96)
    - [I2C WinRT tests](https://docs.microsoft.com/en-us/windows-hardware/test/hlk/testref/a60f5a94-12b2-4905-8416-e9774f539f1d)
    - [SPI WinRT tests](https://docs.microsoft.com/en-us/windows-hardware/test/hlk/testref/50cf9ccc-bbd3-4514-979f-b0499cb18ed8)

# Running TAEF Tests Manually
Many of the tests run through the HLK are built on the [Test Authoring and Execution Framework (TAEF)](https://docs.microsoft.com/en-us/windows-hardware/drivers/taef/). Depending on their complexity some of these tests can easily be run directly using TE.exe.

Copy the following two folders from your HLK server to your IoT Core device:
```
C:\Program Files (x86)\Windows Kits\10\Hardware Lab Kit\Tests\arm\iot
C:\Program Files (x86)\Windows Kits\10\Testing\Runtimes\TAEF\arm\MinTe
```

SSH into your IoT Core device and run the following:
```bash
cd <Parent directory of MinTE and iot>
# List all the tests in the DLL
MinTE\TE.exe iot\windows.devices.lowlevel.unittests.dll -list
# List just the PWM tests
MinTE\TE.exe iot\windows.devices.lowlevel.unittests.dll /name:Pwm* -list
# Run just the PwmTests namespace
MinTE\TE.exe iot\windows.devices.lowlevel.unittests.dll /name:PwmTests::*
# Run a specific test
MinTE\TE.exe iot\windows.devices.lowlevel.unittests.dll /name:PwmTests::VerifyControllerAndPinCreationConcurrent
```

# Test Tools

## Storage
The diskspd utility is an open source disk measurement utility that works on IoT Core and ARM.
+ https://gallery.technet.microsoft.com/DiskSpd-a-robust-storage-6cd2f223
+ https://github.com/microsoft/diskspd

## Network

+ https://github.com/Microsoft/ctsTraffic