#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2020-2021 Huawei Device Co., Ltd.
# 
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.

class GetToolVersion(object):
    def __init__(self):
        self.HDF_VERSION_MAJOR = 0
        self.HDF_VERSION_MINOR = 1

    def get_version(self):
        HDF_TOOL_VERSION = "%s.%s" % (self.HDF_VERSION_MAJOR, self.HDF_VERSION_MINOR)
        return HDF_TOOL_VERSION
