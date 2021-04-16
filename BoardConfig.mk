#
# Copyright (C) 2007 The Android Open Source Project
# Copyright (C) 2012 The Cyanogenmod Project
# Copyright (C) 2021 nproth
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

USE_CAMERA_STUB := true

# inherit from the proprietary version
-include vendor/onyx/t68/BoardConfigVendor.mk

TARGET_ARCH := arm
TARGET_NO_BOOTLOADER := true
TARGET_BOARD_PLATFORM := imx6
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := cortex-a9
TARGET_CPU_SMP := true
ARCH_ARM_HAVE_TLS_REGISTER := true

TARGET_BOOTLOADER_BOARD_NAME := t68

# From unpackbootimg
# TODO enforce SELinux
# Real name of display is probably ED068OG1 instead of E68_V220
BOARD_KERNEL_CMDLINE := console=ttymxc0,115200 init=/init androidboot.console=ttymxc0 video=mxcepdcfb:E68_V220,bpp=16 epdc androidboot.hardware=freescale androidboot.selinux=permissive log_buf_len=4M
BOARD_KERNEL_BASE := 0x80800000
BOARD_KERNEL_PAGESIZE := 2048
BOARD_MKBOOTIMG_ARGS += --ramdisk_offset 0x01000000
BOARD_MKBOOTIMG_ARGS += --second_offset 0x00f00000
BOARD_MKBOOTIMG_ARGS += --tags_offset 0x00000100

# From /proc/partitions, but conflicts with df output
BOARD_BOOTIMAGE_PARTITION_SIZE := 8388608
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 8388608
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 536869888
BOARD_USERDATAIMAGE_PARTITION_SIZE := 2709520384
BOARD_FLASH_BLOCK_SIZE := 4096

BOARD_USES_EMMC := true

TARGET_USERIMAGES_USE_EXT4 := true

TARGET_PREBUILT_KERNEL := device/onyx/t68/kernel

BOARD_HAS_NO_SELECT_BUTTON := false

# Recovery
TARGET_RECOVERY_FSTAB := device/onyx/t68/rootdir/etc/fstab.freescale
BOARD_CUSTOM_GRAPHICS := ../../../device/onyx/t68/recovery/graphics.c
BOARD_CUSTOM_RECOVERY_KEYMAPPING := ../../device/onyx/t68/recovery/recovery_keys.c
BOARD_CUSTOM_RECOVERY_UI := ../../device/onyx/t68/recovery/recovery_ui.c

# Conserve memory in the Dalvik heap
# Details: https://github.com/CyanogenMod/android_dalvik/commit/15726c81059b74bf2352db29a3decfc4ea9c1428
TARGET_ARCH_LOWMEM := true

# Framebuffer does not support 24 bit colors
TARGET_BOOTANIMATION_USE_RGB565 := true
TARGET_BOOTANIMATION_PRELOAD := true
TARGET_BOOTANIMATION_TEXTURE_CACHE := true

# Full name of the firmware file in use (epdc.fw) is V220_C215_68_WC3132_ED068OG1_CTC.fw
HAVE_FSL_EPDC_FB := true

# Later versions do not support epdc
TARGET_HWCOMPOSER_VERSION := v1.0
TARGET_GRALLOC_VERSION := v1

BOARD_USES_ALSA_AUDIO := true
#TARGET_USE_PAN_DISPLAY := true

BOARD_HAVE_VPU := false
HAVE_FSL_IMX_GPU2D := true
HAVE_FSL_IMX_GPU3D := false
HAVE_FSL_IMX_IPU := false
TARGET_HAVE_IMX_GRALLOC := true
TARGET_HAVE_IMX_HWCOMPOSER = true

USE_OPENGL_RENDERER := false

# egl.cfg is obsolete and not considered by android
#BOARD_EGL_CFG := device/onyx/t68/configs/egl.cfg

# no hardware camera
USE_CAMERA_STUB := true

# Wifi
BOARD_WIFI_VENDOR := realtek
WPA_SUPPLICANT_VERSION := VER_0_8_X_RT
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
#CONFIG_DRIVER_WEXT := y
CONFIG_LIBNL20 := y
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_rtl
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_rtl

BOARD_WLAN_DEVICE := rtl8723as

WIFI_DRIVER_MODULE_NAME := "8723as"
WIFI_DRIVER_MODULE_PATH := "/system/lib/modules/8723as.ko"

WIFI_DRIVER_MODULE_ARG := "ifname=wlan0 if2name=p2p0"
WIFI_FIRMWARE_LOADER := "rtw_fwloader"
WIFI_DRIVER_FW_PATH_STA := "STA"
WIFI_DRIVER_FW_PATH_AP := "AP"
WIFI_DRIVER_FW_PATH_P2P := "P2P"
WIFI_DRIVER_FW_PATH_PARAM := "/dev/null"

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0 \
	ro.carrier=wifi-only

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_RTK := true
BLUETOOTH_HCI_USE_RTK_H5 := true
SW_BOARD_HAVE_BLUETOOTH_NAME := rtl8723as
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/onyx/t68/bluetooth/
BOARD_BLUEDROID_VENDOR_CONF := device/onyx/t68/bluetooth/vnd_t68.txt

PRODUCT_PROPERTY_OVERRIDES += \
	ro.property.bluetooth.rtk8723a = true \
	ro.rfkilldisabled = 1