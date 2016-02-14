#!/bin/bash

export ARCH=arm
export CROSS_COMPILE=/opt/toolchains/arm-eabi-4.8/bin/arm-eabi-

make exynos5433-gts28wifi_defconfig
make