Adding New Drivers
================

This document walks through the steps required to add a new driver to FFU image builds.

## Adding a New Driver to the Solution
1) Right click the Drivers folder in Solution Explorer and add a New Project
2) Select Windows Drivers then Kernel Mode Driver or User Mode Driver. Set the name of your driver and set the location to `imx-iotcore\driver`. The rest of the instructions assume the name `MyTestDriver`. After the project has been created, select it in Solution Explorer and save your changes with Ctrl+S
3) Copy the reference TestDriver.wm.xml from `imx-iotcore\build\tools\TestDriver.wm.xml` into your project directory and rename it after your project. The rest of the instructions assume the name `MyTestDriver.wm.xml`.
4) Edit `MyTestDriver.wm.xml` to set the name, namespace, owner, legacyName, and INF source. The legacyName field determines what your driver cab package will be named.
5) Open `MyTestDriver.vcxproj` in a text editor and add the following XML as the first entry under the `<Project>` tag at the top of the file. Change the wm.xml names to match your new one, then save and close the file.
```XML
  <Import Project="$(SolutionDir)..\..\common.props"/>
  <ItemGroup>
    <PkgGen Include="MyTestDriver.wm.xml">
      <AdditionalOptions>/universalbsp</AdditionalOptions>
    </PkgGen>
  </ItemGroup>
```
6) Navigate back to Visual Studio and select Reload Solution if it prompts.
7) Modify your driver's inf to store driver in the Driver Store. Change the DIRID number in DefaultDestDir and ServiceBinary from 12 to 13. This will cause your driver binary to be stored in `C:\Windows\System32\DriverStore` instead of `C:\Windows\System32\Drivers`.
```
[DestinationDirs]
DefaultDestDir = 13
...
ServiceBinary  = %13%\MyTestDriver.sys
```
8) Right click the GenerateTestFFU project, select Project Dependencies, then check the box next to your new project.


## Adding a Driver to the FFU
1) After adding your driver to the project and building it, confirm that your driver has built and placed its binaries and .cab file inside of `imx-iotcore\build\solution\iMXPlatform\Build\Release\ARM`
2) Open the Device Feature Manifest for your board (e.g. HummingBoard uses `imx-iotcore\build\board\HummingBoardEdge_iMX6Q_2GB\InputFMs\HummingBoardDeviceFM.xml`)
3) Add a new PackageFile section to the XML and modify it with the package name set by legacyName in your wm.xml. Change FeatureID to match the other drivers in your board file, or create a new FeatureID for your feature.
```XML
      <PackageFile Path="%BSPPKG_DIR%" Name="MyOEM.MyNamespace.MyTestDriver.cab">
        <FeatureIDs>
          <FeatureID>MYNEWFEATURE_DRIVERS</FeatureID>
        </FeatureIDs>
      </PackageFile>
```
4) **If you did not create a new FeatureID, skip this step.** If you created a new FeatureID in your DeviceFM.xml then you must select it in your ProductionOEMInput.xml and TestOEMInput.xml files to include the driver in the respective image (e.g. HummingBoard Edge iMX6Q uses `imx-iotcore\build\board\HummingBoardEdge_iMX6Q_2GB\HummingBoardEdge_iMX6Q_2GB_TestOEMInput.xml` and `imx-iotcore\build\board\HummingBoardEdge_iMX6Q_2GB\HummingBoardEdge_iMX6Q_2GB_ProductionOEMInput.xml`).
```XML
<OEM>
  <Feature>MYNEWFEATURE_DRIVERS</Feature>
  <Feature>IMX_DRIVERS</Feature>
</OEM>
```
5) Clean the solution then rebuild the GenerateTestFFU project and your driver will be included in the FFU.
