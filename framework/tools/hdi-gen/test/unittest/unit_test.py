#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2023 Huawei Device Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.

import os
import subprocess
import time


def get_time_stamp():
    return int(round(time.time() * 1000))


def print_success(info):
    print("\033[32m{}\033[0m".format(info))


def print_failure(info):
    print("\033[31m{}\033[0m".format(info))


def compare_file(first_file, second_file):
    with open(first_file, 'r') as first_hash_file:
        with open(second_file, 'r') as second_hash_file:
            first_hash_info = first_hash_file.read()
            second_hash_info = second_hash_file.read()
            return first_hash_info == second_hash_info


def compare_files(first_file_path, second_file_path):
    first_files = set(os.listdir(first_file_path))
    second_files = set(os.listdir(second_file_path))

    if first_files != second_files:
        return False
    
    for files in first_files:
        if not compare_file(os.path.join(first_file_path, files), os.path.join(second_file_path, files)):
            return False
    return True


def exec_command(command):
    return subprocess.getstatusoutput(command)


def file_exists(file_path):
    return os.path.isfile(file_path)


def make_binary_file(file_path):
    print("making hdi-gen...")
    return exec_command("make --directory={} --jobs=4".format(file_path))


class Test:
    def __init__(self, name, working_dir):
        self.name = name
        self.working_dir = working_dir
        self.idl_dir = os.path.join(self.working_dir, "foo")
        self.output_dir = os.path.join(working_dir, "out")
        self.target_dir = os.path.join(working_dir, "target")
        self.command = "../../hdi-gen -s full -m ipc -l cpp -r ohos.hdi:{} -d {}".format(working_dir, self.output_dir)

    def run(self):
        # please add test code here
        return False

    def remove_output(self):
        exec_command("rm -rf {}".format(self.output_dir))

    def test(self):
        print_success("[ RUN       ] {}".format(self.name))
        start_time = get_time_stamp()
        result = self.run()
        end_time = get_time_stamp()

        if result:
            print_success("[        OK ] {} ({}ms)".format(self.name, end_time - start_time))
        else:
            print_failure("[    FAILED ] {} ({}ms)".format(self.name, end_time - start_time))
        return result


# compile empty idl file
class UnitTest01(Test):
    def add_idl_files(self):
        self.command += "-c {}".format(os.path.join(self.idl_dir, "v1_0", "IFoo.idl"))
    
    def run(self):
        self.add_idl_files()
        status, _ = exec_command(self.command)
        if status != 0 and _ == "[HDI-GEN]: no idl files.":
            return True
        return False


# compile empty idl file
class UnitTest02(Test):
    def add_idl_files(self):
        self.command += "-c {}".format(os.path.join(self.idl_dir, "v1_0", "IFoo.idl"))
    
    def run(self):
        self.add_idl_files()
        status, _ = exec_command(self.command)
        if status == 0 and compare_files(self.output_dir, self.target_dir):
            return True
        return False

'''
# get hash key and print standard ouput
class TestHashGood1(Test):
    def run(self):
        result_hash_file_path = "./good/hash.txt"
        command = "../../hdi-gen -D ./good/ -r ohos.hdi:./good/ --hash"
        status, exec_result = exec_command(command)
        if status != 0:
            print(exec_result)
            return False
        temp_hash_info = exec_result

        with open(result_hash_file_path, 'r') as result_hash_file:
            result_hash_info = result_hash_file.read().rstrip()
            return temp_hash_info == result_hash_info


# get hash key and print file
class TestHashGood2(Test):
    def run(self):
        result_hash_file_path = "./good/hash.txt"
        temp_hash_file_path = "./good/temp.txt"
        command = "../../hdi-gen -D ./good/ -r ohos.hdi:./good/ --hash -o {}".format(temp_hash_file_path)
        status, result = exec_command(command)
        if status != 0:
            print(result)
            return False

        result = False
        if compare_file(temp_hash_file_path, result_hash_file_path):
            result = True
        exec_command("rm -f ./good/temp.txt")
        return result


# nothing idl files
class TestBadHash01(Test):
    def run(self):
        command = "../../hdi-gen -D ./bad_01/ -r ohos.hdi:./bad_01/ --hash"
        status, _ = exec_command(command)
        return status != 0


# empty idl file
class TestBadHash02(Test):
    def run(self):
        command = "../../hdi-gen -D ./bad_02/ -r ohos.hdi:./bad_02/ --hash"
        status, _ = exec_command(command)
        return status != 0


# the idl file has no package name
class TestBadHash03(Test):
    def run(self):
        command = "../../hdi-gen -D ./bad_03/ -r ohos.hdi:./bad_03/ --hash"
        status, _ = exec_command(command)
        return status != 0


# the idl file has error package name
class TestBadHash04(Test):
    def run(self):
        command = "../../hdi-gen -D ./bad_04/ -r ohos.hdi:./bad_04/ --hash"
        status, _ = exec_command(command)
        return status != 0
'''

class Tests:
    test_cases = [
        UnitTest01("UnitTestEmptyIdl", "01_empty_idl"),
        UnitTest02("UnitTestStandardInterface", "02_standard_interface_idl"),
        # UnitTest03("UnitTestStandardCallback", "03_standard_callback_idl"),
        # UnitTest04("UnitTestExtendedInterface", "04_extended_interface_idl"),
        # UnitTest05("UnitTestTypesIdl", "05_types_idl"),
        # UnitTest06("UnitTestEnumExtension", "06_extended_enum_idl"),
        # UnitTest06("UnitTestStructExtension", "07_extended_struct_idl"),
        # UnitTest06("UnitTestOverloadMethod", "08_overload_method_idl"),
    ]

    @staticmethod
    def set_up_test_case():
        hdi_gen_file = "../../hdi-gen"
        ret = file_exists(hdi_gen_file)
        if not ret:
            hdi_gen_path = "../../"
            ret = make_binary_file(hdi_gen_path)
        if not ret:
            print_failure("[===========] failed to make hdi-gen")
        return ret

    @staticmethod
    def tear_down_test_case():
        for case in Tests.test_cases:
            case.remove_output()

    @staticmethod
    def test():
        test_case_num = len(Tests.test_cases)
        success_case_num = 0
        print_success("[===========] start {} test".format(test_case_num))
        for test_case in Tests.test_cases:
            if test_case.test():
                success_case_num += 1
        print_success("[    PASSED ] {} test".format(success_case_num))
        failure_case_num = test_case_num - success_case_num
        if failure_case_num > 0:
            print_failure("[    FAILED ] {} test".format(failure_case_num))


if __name__ == "__main__":
    if not Tests.set_up_test_case():
        print_failure("test case set up failed!")
        exit(-1)
    Tests.test()
    Tests.tear_down_test_case()
