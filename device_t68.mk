$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

$(call inherit-product, frameworks/native/build/tablet-dalvik-heap.mk)

$(call inherit-product-if-exists, vendor/onyx/t68/t68-vendor.mk)

DEVICE_PACKAGE_OVERLAYS += device/onyx/t68/overlay

LOCAL_PATH := device/onyx/t68
ifeq ($(TARGET_PREBUILT_KERNEL),)
	LOCAL_KERNEL := $(LOCAL_PATH)/kernel
	COMMON_GLOBAL_CFLAGS += -DPREBUILT_KERNEL
else
	LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

PRODUCT_COPY_FILES += \
    $(LOCAL_KERNEL):kernel

$(call inherit-product, build/target/product/full.mk)

PRODUCT_BUILD_PROP_OVERRIDES += BUILD_UTC_DATE=0
PRODUCT_NAME := full_t68
PRODUCT_DEVICE := t68

# Include epdc firmware in recovery ramdisk
# Both for the prebuilt kernel and the freescale default path
PRODUCT_COPY_FILES += \
	vendor/onyx/t68/proprietary/firmware/imx/epdc.fw:root/epdc_E68_V220.fw \
	vendor/onyx/t68/proprietary/firmware/imx/epdc.fw:system/vendor/firmware/imx/epdc.fw

# Supported features
PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
	frameworks/native/data/etc/android.hardware.bluetooth_le.xml:system/etc/permissions/android.hardware.bluetooth_le.xml \
    frameworks/native/data/etc/android.hardware.location.xml:system/etc/permissions/android.hardware.location.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
	frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
	frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
	frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml
	

# Input
PRODUCT_COPY_FILES += \
	device/onyx/t68/idc/gpio-keys.idc:system/usr/idc/gpio-keys.idc \
	device/onyx/t68/keylayout/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl

# Audio
PRODUCT_PACKAGES += \
	device/onyx/t68/alsa/audio_policy.conf:system/etc/audio_policy.conf

# HALs
PRODUCT_PACKAGES += \
	gralloc.imx6 \
	hwcomposer.imx6 \
	lights.imx6 \
	power.imx6 \
	audio.primary.imx6 \
	audio.a2dp.default \
    audio.usb.default \
    audio.r_submix.default \
	sensors.default

# RAMdisk
PRODUCT_PACKAGES += \
	fstab.freescale \
	init.freescale.rc \
	init.target.rc \
	ueventd.freescale.rc \
	init.recovery.freescale.rc

# Build emulated OpenGL ES 1.x and symlink to it
PRODUCT_PACKAGES += \
	libGLES_android \
	libGLES

# Wifi
PRODUCT_PACKAGES += \
	rtw_fwloader

# Bluetooth
PRODUCT_PACKAGES += \
	Bluetooth \
    bt_vendor.conf \
    bt_stack.conf \
    bt_did.conf \
    auto_pair_devlist.conf \
    libbt-hci \
    bluetooth.default \
    audio.a2dp.default \
    libbt-client-api \
	lbbt-vendor \
	rtk_hciattach