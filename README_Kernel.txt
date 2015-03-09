################################################################################

1. How to Build
	- get Toolchain
		From android git server, codesourcery and etc ..
		 - arm-eabi-4.7

	- edit Makefile
		edit "CROSS_COMPILE" to right toolchain path(You downloaded).
		  EX)   export CROSS_COMPILE= $(android platform directory you download)/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-
		  Ex)   export CROSS_COMPILE=/usr/local/toolchain/arm-eabi-4.7/bin/arm-eabi-          // check the location of toolchain
		  or
		  Ex)	export CROSS_COMPILE=arm-eabi-
		  Ex)	export PATH=$PATH:<toolchain_parent_dir>/arm-eabi-4.7/bin

		$ cd drivers/vendor/hisi/build
		$ python obuild.py product=hi3630_udp acore-oam_ps -j 8
		$ cd -
		$ make ARCH=arm O=../out/target/product/hi3630/obj/KERNEL_OBJ merge_hi3630_defconfig
		$ make ARCH=arm O=../out/target/product/hi3630/obj/KERNEL_OBJ -j8

2. Output files
	- Kernel : out/target/product/hi3630/obj/KERNEL_OBJ/arch/arm/boot/zImage
	- module : out/target/product/hi3630/obj/KERNEL_OBJ/drivers/*/*.ko

3. How to Clean
		$ make ARCH=arm distclean
		$ rm -rf out
################################################################################
