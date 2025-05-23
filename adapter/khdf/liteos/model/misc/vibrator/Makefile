# Copyright (c) 2022 Huawei Device Co., Ltd. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of
#    conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list
#    of conditions and the following disclaimer in the documentation and/or other materials
#    provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be used
#    to endorse or promote products derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

include $(LITEOSTOPDIR)/../../drivers/hdf_core/adapter/khdf/liteos/lite.mk

MODULE_NAME := hdf_vibrator_driver

FRAMEWORKS_VIBRATOR_ROOT = $(LITEOSTOPDIR)/../../drivers/hdf_core/framework/model/misc/vibrator/driver
PERIPHERAL_VIBRATOR_ROOT = $(LITEOSTOPDIR)/../../drivers/peripheral/vibrator

LOCAL_INCLUDE := $(FRAMEWORKS_VIBRATOR_ROOT)/include \
                 $(PERIPHERAL_VIBRATOR_ROOT)/chipset/linear

LOCAL_SRCS += $(FRAMEWORKS_VIBRATOR_ROOT)/src/vibrator_driver.c \
              $(FRAMEWORKS_VIBRATOR_ROOT)/src/vibrator_haptic.c \
              $(FRAMEWORKS_VIBRATOR_ROOT)/src/vibrator_parser.c

ifeq ($(LOSCFG_DRIVERS_HDF_VIBRATOR_LINEAR), y)
LOCAL_SRCS += $(PERIPHERAL_VIBRATOR_ROOT)/chipset/linear/vibrator_linear_driver.c
endif

include $(HDF_DRIVER)
