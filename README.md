# RFSo4x2-Houdini
A custom linux distribution designed for RFSoc4x2 with user space GPIO controls and SPI communication to the on-board LMK04828 and LMX2594 chips.

The Hardware folder contains the board files for RFSoc4x2 from RealDigital, a TCL file for creating the Vivado Hardware Platform, and a example XSA file for setting up the petalinux project.

The Petalinux Project Files folder contains:
1. A user defined DTSI file that need to be placed in (replace): <project-root-dir>/project-spec/meta-user/recipes-bsp/device-tree/files
2. A custom library addition command file that need to be placed in (replace): <project-root-dir>/project-spec/meta-user/conf

Petalinux Project Directories: https://drive.google.com/file/d/14JaCjJ4gdto-c6BeOtUyNlwLFsppkf6j/view?usp=sharing

Example Boot Images: https://drive.google.com/file/d/1-RzhDtFdqg2pkDm769la_5lxMcgW9sOF/view?usp=sharing

The Software folder contains the C files used for the Vitis projects and example ELF files that are executable on RFSoc4x2 board. 


Shifan Liu
Rice Wireless Lab
