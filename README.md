Wisblock for Zephyr
===================

This project is prepared to use RAKwireless Wisblock LoRa modules with Zephyr project. All sample projects are tested on RAK19007 Wisblock base board.

There is also support for the RAK5010 BLE to GPRS/LTE-M/NB-IoT industrial gateway card.

The main idea of the repo is inspired by https://github.com/zephyrproject-rtos/example-application.

### Initialization
Before starting initialization, please install VsCODE IDE, GIT, and NRFUTIL programs for your operating system.

After that, you must install toolchain via:

```shell
# Install tools for managing and using toolchains:
nrfutil install device
nrfutil install toolchain-manager
# Install a specific version toolchain:
nrfutil toolchain-manager install --ncs-version v3.3.0
# List your currently installed toolchain and learn its path:
nrfutil toolchain-manager list
# Launch toolchain commands in the environment directly:
nrfutil toolchain-manager launch --terminal
# Go to setup path:
cd <go to destination you wanna setup wisblock-zephyr-workspace>
# Initialize wisblock-zephyr-workspace:
west init -m https://github.com/srcnert/wisblock-for-zephyr wisblock-zephyr-workspace
# Update zephyr modules:
cd wisblock-zephyr-workspace
west update
```

After that, please install the Zephyr Software Development Kit (SDK) that contains toolchains
for each of Zephyr’s supported architectures, which including compiler, assembler,
linker and other programs required to build Zephyr applications.

Please firstly install 'wget' tool according to your operating system. After that, the user
must apply following commands:

```shell
# Python environment is missing the tqdm package, which is required by Zephyr's
# west tool to display progress bars during installations.
pip install tqdm
# Remove current zephyr-sdk from your toolchain. (Please use your own toolchain address!)
rm -rf /your/path/to/toolchains/0123456789/opt/zephyr-sdk
# Go to wisblock-zephyr-workspace folder.
cd <go to destination wisblock-zephyr-workspace>
# Install zephyr-sdk for each of Zephyr’s supported architectures.
west sdk install -d /your/path/to/toolchains/0123456789/opt/zephyr-sdk -t arm-zephyr-eabi riscv64-zephyr-elf xtensa-espressif_esp32s3_zephyr-elf
```

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run
the command below to retrieve those files.

```shell
cd <go to destination wisblock-zephyr-workspace>
west blobs fetch hal_espressif
```

To upgrade esptool, please apply following changes.
```shell
nrfutil toolchain-manager launch --shell
pip install --upgrade "esptool>=5.0.2"
```

This is the easier way to set up a toolchain for Zephyr RTOS or nRF Connect SDK for RAK4631(nRF52840 + SX1262) or any other boards.

Second way is to follow up Zephyr RTOS guide:
https://docs.zephyrproject.org/latest/develop/getting_started/index.html

### Patch
To patch addressed issues, run the following command:

```shell
cd wisblock-for-zephyr
west patch_rak
```

To revert applied patches:

```shell
west patch_rak --revert
```

To list patch files:

```shell
west patch list
```

### Building and running
To build an application, open '../wisblock-zephyr-workspace/wisblock-for-zephyr' directory on your Visual Studio Code app. After that, please open '../.vscode/settings.json' file and set following parameters:

```shell
"ZEPHYR_WORKSPACE": "/your/path/to/wisblock-zephyr-workspace",

"TOOLCHAIN_BASE":   "/your/path/to/toolchains/0123456789",
"JLINK_PATH":       "/your/path/to/JLink_V123",
"MCUMGR_PATH":      "/your/path/to/go/bin"
"OPENOCD_PATH":     "/your/path/to/xpack-openocd-<version>"
```

*** McuMgr is necessary to use mcumgr commands. Please check dfu application for details.

You can build your project via 'VsCode --> Terminal --> Run Build Task' option.
For example, if you wanna build 'app/adc' example for rak3172, please select following options:
- Select the board: rak3172
- Select the build type: --no-sysbuild
- Select the ble overlay: no_overlay.conf
- Select the mcuboot overlay: no_overlay.conf
- Select the sleep overlay: no_overlay.conf
- Select the project: app/adc

For example, if you wanna build 'app/lorawan_otaa' example for rak11720, please select following options:
- Select the board: rak11720
- Select the build type: --sysbuild
- Select the ble overlay: overlay_ble.conf
- Select the mcuboot overlay: overlay_mcuboot.conf
- Select the sleep overlay: overlay_sleep_rak11720.conf
- Select the project: app/lorawan_otaa

The overlay files can be selected according to your request. Whole sample examples are tested on Rak19007 base board. J-Link is used to program stamp modules.

To program your board, please use 'VsCode --> Terminal --> Run Task --> flash' option.

### Debugging

Vs Code IDE's 'Cortex-Debug' extension is used to debug any RAK module on Vs Code IDE.

* RAK3172, RAK4631, RAK5010 and RAK11720 modules are Cortex-M base MCUs. To debug and program ARM based modules, please install latest J-LINK. 'Cortex-Debug' extension is used with "JLinkGDBServerCL" exe to debug these modules. Please select
"Cortex Debug ARM" configuration at VS Code IDE "Run and Dubug" section.

* RAK3112 is ESP32-S3 based module and comes with internal(built-in) JTAG probe. To debug this module,
OPENOCD and GDB tools are used. Please check Digikey video (https://www.youtube.com/watch?v=XGTtMYa7IiM) to learn how to set up your operating system. If you are Windows OS user, you must set-up internal JTAG probe with Zadig program.

Please also take a look for detailed information:
https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/jtag-debugging/index.html

Shortly, OPENOCD (https://github.com/xpack-dev-tools/openocd-xpack/releases) must be installed for your operating system. There is no installer for it and you just unzip it to appropriate location and please open '../.vscode/settings.json' file and set 'OPENOCD_PATH' parameter.

If you are Mac OS user, you must go to Settings of your Mac and then Privacy&Security section. After that, run "openocd --version" command continuously and then click OPEN ANYWAY button on Privacy&Security section. The reason is that some openocd files was blocked by OS to protect your Mac. The aim is to give permision to all related openocd exes.

If all steps are completed successfully, run following command and see that RAK3112 module is connected and openocd server is running.

```shell
openocd -f board/esp32s3-builtin.cfg
```

You can fing GDB tool inside your "/your/path/to/toolchains/0123456789/opt/zephyr-sdk/gnu/xtensa-espressif_esp32s3_zephyr-elf/bin" path and the exe name is "xtensa-espressif_esp32s3_zephyr-elf-gdb". There is no need to extra set up for GDB.
'Cortex-Debug' extension is used with "xtensa-espressif_esp32s3_zephyr-elf-gdb" exe to debug rak3112 module.
Please select "Cortex Debug XTENSA" configuration at VS Code IDE "Run and Dubug" section.

Do NOT ASSIGN GPIO19 and GPIO20 to any peripheral!!! They are USB pins!!!

If you brick RAK3112 module, please apply following steps.
- Set BOOT pin to GND and reset module.
- Program board with "west flash"
- Disconnect BOOT pin from GND and reset module.
