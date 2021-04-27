# Copyright (C) 2008 The Android Open Source Project
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


LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked and stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
ifeq ($(TARGET_HWCOMPOSER_VERSION),v1.0)
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SHARED_LIBRARIES :=	\
	liblog					\
	libEGL					\
	libcutils				\
	libutils				\
	libui					\
	libhardware				\
	libhardware_legacy		\
	libbinder

LOCAL_SRC_FILES :=			\
	hwcomposer.cpp			\
	hwc_vsync.cpp			\
    hwc_display.cpp

LOCAL_C_INCLUDES += device/onyx/t68/kernel_headers

LOCAL_MODULE := hwcomposer.$(TARGET_BOARD_PLATFORM)
LOCAL_CFLAGS:= -DLOG_TAG=\"hwcomposer\"
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
endif
