#!/bin/bash
# based on the instructions from edk2-platform
set -e
. build_common.sh
# not actually GCC5; it's GCC7 on Ubuntu 18.04.
./build_bootshim.sh
GCC5_ARM_PREFIX=arm-linux-gnueabihf- build -j$(nproc) -s -n 0 -a ARM -t GCC5 -p EXYNOS7885Pkg/Devices/a10.dsc
cat BootShim/BootShim.bin workspace/Build/EXYNOS7885Pkg/DEBUG_GCC5/FV/EXYNOS7885PKG_UEFI.fd > workspace/UEFI
#mkbootimg --kernel workspace/UEFI -o workspace/boot.img

python3 mkbootimg.py \
  --kernel workspace/UEFI \
  --ramdisk ramdisk \
  --base 0x10000000 \
  --kernel_offset 0x00008000 \
  --ramdisk_offset 0x01000000 \
  --second_offset 0x00f00000 \
  --tags_offset 0x00000100 \
  --os_version 6.0.0 \
  --pagesize 2048 \
  --os_patch_level "$(date '+%Y-%m')" \
  --header_version 0 \
  -o "boot.img" \
  
tar -c boot.img -f boot.tar
