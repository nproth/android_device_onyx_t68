## Specify phone tech before including full_phone
$(call inherit-product, vendor/cm/config/gsm.mk)

# Release name
PRODUCT_RELEASE_NAME := t68

# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# Inherit device configuration
$(call inherit-product, device/onyx/t68/device_t68.mk)

## Device identifier. This must come after all inclusions
PRODUCT_DEVICE := t68
PRODUCT_NAME := cm_t68
PRODUCT_BRAND := onyx
PRODUCT_MODEL := t68
PRODUCT_MANUFACTURER := onyx
