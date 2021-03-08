$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

# The gps config appropriate for this device
$(call inherit-product, device/common/gps/gps_us_supl.mk)

$(call inherit-product-if-exists, vendor/onyx/t68/t68-vendor.mk)

DEVICE_PACKAGE_OVERLAYS += device/onyx/t68/overlay

LOCAL_PATH := device/onyx/t68
ifeq ($(TARGET_PREBUILT_KERNEL),)
	LOCAL_KERNEL := $(LOCAL_PATH)/kernel
else
	LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

PRODUCT_COPY_FILES += \
    $(LOCAL_KERNEL):kernel

$(call inherit-product, build/target/product/full.mk)

PRODUCT_BUILD_PROP_OVERRIDES += BUILD_UTC_DATE=0
PRODUCT_NAME := full_t68
PRODUCT_DEVICE := t68


# Init files
PRODUCT_COPY_FILES += \
    device/onyx/t68/fstab.freescale:root/fstab.freescale \
	device/onyx/t68/recovery/init.recovery.freescale.rc:root/init.recovery.freescale.rc \
	device/onyx/t68/ueventd.freescale.rc:root/ueventd.freescale.rc

# Include epdc firmware in recovery ramdisk
# TODO include in recovery only
PRODUCT_COPY_FILES += \
	vendor/onyx/t68/proprietary/firmware/imx/epdc.fw:root/lib/firmware/imx/epdc/epdc_E68_V220.fw