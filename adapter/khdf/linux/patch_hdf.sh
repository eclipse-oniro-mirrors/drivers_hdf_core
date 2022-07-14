#!/bin/bash
#
# Copyright (c) 2020-2021 Huawei Device Co., Ltd.
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

set -e

OHOS_SOURCE_ROOT=$1
KERNEL_BUILD_ROOT=$2
KERNEL_PATCH_PATH=$3
DEVICE_NAME=$4
HDF_COMMON_PATCH="common"
HDF_PATCH_FILE=

ln_list=(
    $OHOS_SOURCE_ROOT/drivers/hdf_core/adapter/khdf/linux    drivers/hdf/khdf
    $OHOS_SOURCE_ROOT/drivers/hdf_core/framework             drivers/hdf/framework
    $OHOS_SOURCE_ROOT/drivers/hdf_core/framework/include     include/hdf
)

cp_list=(
    $OHOS_SOURCE_ROOT/third_party/bounds_checking_function  ./
    $OHOS_SOURCE_ROOT/device/soc/hisilicon/common/platform/wifi         drivers/hdf/
    $OHOS_SOURCE_ROOT/third_party/FreeBSD/sys/dev/evdev     drivers/hdf/
)

specify_device_list=(
    rk3568
)


function copy_external_compents()
{
    for ((i=0; i<${#cp_list[*]}; i+=2))
    do
        dst_dir=${cp_list[$(expr $i + 1)]}/${cp_list[$i]##*/}
        mkdir -p $dst_dir
        [ -d ${cp_list[$i]}/ ] && cp -arfL ${cp_list[$i]}/* $dst_dir/
    done
}

function ln_hdf_repos()
{
    for ((i=0; i<${#ln_list[*]}; i+=2))
    do
        ln -sf ${ln_list[$i]} ${ln_list[$(expr $i + 1)]}
    done
}

function get_hdf_patch_patch()
{
    if [[ "$specify_device_list" =~ "$DEVICE_NAME" ]]; then
	    HDF_PATCH_FILE=${KERNEL_PATCH_PATH}/${DEVICE_NAME}_patch/hdf.patch
	else
	    HDF_PATCH_FILE=${KERNEL_PATCH_PATH}/${HDF_COMMON_PATCH}_patch/hdf.patch
	fi
}

function main()
{
    cd $KERNEL_BUILD_ROOT
    get_hdf_patch_patch
    patch -p1 < $HDF_PATCH_FILE
    ln_hdf_repos
    copy_external_compents
    cd -
}

main
