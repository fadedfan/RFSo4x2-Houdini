The following instructions are adapted from the official repo from the RealDigital Github:

1. Run "petalinux-create -t project -n <name for project> --template zynqMP". This will create a new project with a name of that is for the ZynqMP.

2. Use "cd" to enter the project after it has been created by the above step.

3. Run the command "petalinux-config --get-hw-description .." to load in the hardware design exported in the "Build Hardware" section.

4. At this point, customization of the device tree can be performed by editing the file "project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi". The customized device tree used for the hardware design supported here is as a reference.

5. To access the OLED from user space, the "spidev" driver needs to be added to the kernel configuration. To configure the kernel, execute the command "petalinux-config -c kernel". This can take several minutes of execution time before menu pops up in a new window. Once the menu is up, follow these steps:

  #^ * Select "Device Drivers --->".
  * Scroll down and select "SPI support --->".
  c. Scroll down to "User mode SPI device driver support" and tap twice to get an asterisk in the selection column.
  d. Use Exit three times to get back to the main menu and then out of the configuration program.
  e. When prompted, save the configuration.
  f. Run the "petalinux-build" command to build the PetaLinux system. This step will take from several minutes and possibly more than an hour depending on the speed of the internet connection and the speed of the PC.

6. Run "petalinux-config -c rootfs", to change the login settings, and enable custom libraries like libgpiod.

7. If you need to do Kernel hack and change some default drivers or tools, run command: "petalinux-devtool modify linux-xlnx", and change the files in "<your-petalinux-project>/components/yocto/workspace/sources/linux-xlnx" 

8. Upon completion of the above step, run "petalinux-package --boot --u-boot --fpga images/linux/system.bit". This step creates the BOOT.BIN file needed for installation on the SD Card. Use --force to overwrite.

9. Other files created by running "petalinux-build" that should be copied to the first SD card partition are "boot.scr", "image.ub", and "system.dtb".
