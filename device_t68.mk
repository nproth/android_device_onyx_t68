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

# Input
PRODUCT_COPY_FILES += \
	device/onyx/t68/idc/gpio-keys.idc:system/usr/idc/gpio-keys.idc \
	device/onyx/t68/keylayout/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl

# HALs
PRODUCT_PACKAGES += \
    audio.primary.imx6 \
	gralloc.imx6 \
	hwcomposer.imx6 \
	lights.imx6 \
	power.imx6 

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
#PRODUCT_PACKAGES += \
	libwpa_client \
    hostapd \
    dhcpcd.conf \
    wpa_supplicant \
    wpa_supplicant.conf