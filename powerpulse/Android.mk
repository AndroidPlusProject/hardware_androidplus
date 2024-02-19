#
# Copyright (C) 2018     TeamNexus
# Copyright (C) 2024 The Android Plus Project OS
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
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE               := android.hardware.power@1.3-service.powerpulse
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS          := optional
LOCAL_PROPRIETARY_MODULE   := true

LOCAL_INIT_RC := android.hardware.power@1.3-service.powerpulse.rc

LOCAL_SRC_FILES := \
	Service.cpp \
	Power.cpp

LOCAL_SHARED_LIBRARIES := \
	libbase \
	libcutils \
	libdl \
	libhardware \
	libhidlbase \
	libhidltransport \
	liblog \
	libutils \
	libpowerpulse \
	android.hardware.power@1.0 \
	android.hardware.power@1.3

LOCAL_C_INCLUDES := \
	external/powerpulse/lib/include

LOCAL_CFLAGS := -Wall -Werror -Wno-unused-parameter -Wno-unused-function

ifneq (,$(wildcard hardware/lineage/interfaces/power/1.0/ vendor/cmsdk/))
  LOCAL_SHARED_LIBRARIES += vendor.lineage.power@1.0
  LOCAL_CFLAGS += -DPOWER_HAS_LINEAGE_HINTS
endif

# Enforces strict limitations of rules present in power-HAL.
# This may cause errors but ensures the stability of the
# power-HAL
LOCAL_CFLAGS += -DSTRICT_BEHAVIOUR

# Allow the power HAL to run on any CPU core
LOCAL_CFLAGS += -DNR_CPUS=$(TARGET_NR_CPUS)

include $(BUILD_EXECUTABLE)
