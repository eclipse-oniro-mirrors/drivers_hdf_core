#
# Copyright (c) 2020-2023 Huawei Device Co., Ltd.
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
#

include drivers/hdf/khdf/platform/platform.mk
ccflags-y += -DUSER_I2C_SUPPORT
obj-y  += $(HDF_PLATFORM_FRAMEWORKS_ROOT)/src/i2c/i2c_core.o \
          $(HDF_PLATFORM_FRAMEWORKS_ROOT)/src/i2c/i2c_if.o \
          $(HDF_PLATFORM_FRAMEWORKS_ROOT)/src/i2c/i2c_msg.o \
          ./i2c_adapter.o
