#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2022 Huawei Device Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.


import os
import random
import re

import hdf_utils


class OperateGroupPasswd(object):
    def __init__(self, tool_settings, root_path):
        super(OperateGroupPasswd, self).__init__()
        etc_path = tool_settings.get_passwd_group_config()
        self.group_newline = etc_path.get("group").get("info_temp")
        self.passwd_newline = etc_path.get("passwd").get("info_temp")

        self.passwd_file = os.path.join(root_path, etc_path.get("passwd").get("path"))
        self.group_file = os.path.join(root_path, etc_path.get("group").get("path"))

        self.passwd_lines = hdf_utils.read_file_lines(self.passwd_file)
        self.group_lines = hdf_utils.read_file_lines(self.group_file)

        self.passwd_group_name_list = []
        self.temp_id = self.get_id()

    def operate_group(self, name):
        result_group = self.group_newline.format(peripheral_name=name, uid=self.temp_id)
        if result_group.split(":")[0] not in self.passwd_group_name_list:
            self.group_lines.append(result_group)
            hdf_utils.write_file_lines(self.group_file, self.group_lines)
        return self.group_file

    def operate_passwd(self, name):
        result_passwd = self.passwd_newline.format(peripheral_name=name, uid=self.temp_id)
        if result_passwd.split(":")[0] not in self.passwd_group_name_list:
            self.passwd_lines.append(result_passwd)
            hdf_utils.write_file_lines(self.passwd_file, self.passwd_lines)
        return self.passwd_file

    def get_id(self):
        passwd_group_id_list = []
        for line in self.group_lines:
            id_re_result = re.search(r"x:\d+", line)
            if id_re_result:
                gid = id_re_result.group().split(":")[-1]
                passwd_group_id_list.append(int(gid))
            self.passwd_group_name_list.append(line.split(":")[0])
        while True:
            temp_id = self.generate_id(max(passwd_group_id_list))
            if temp_id not in passwd_group_id_list:
                break
        return temp_id

    @staticmethod
    def generate_id(max_border):
        max_border += 20
        return random.randint(99, max_border)

