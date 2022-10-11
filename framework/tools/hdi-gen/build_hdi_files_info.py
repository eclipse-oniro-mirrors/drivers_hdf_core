#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2021-2022 Huawei Device Co., Ltd.
#
# HDF is dual licensed: you can use it either under the terms of
# the GPL, or the BSD license, at your option.
# See the LICENSE file in the root of this repository for complete details.

import argparse
import json
import os
import re
import sys


class TokenType(object):
    UNKNOWN = 0
    COMMENT = 1
    PACKAGE = 2
    IMPORT = 3
    INTERFACE = 4
    CALLBACK = 5
    ID = 6
    END_OF_FILE = 7


class Token(object):
    def __init__(self, file_name, token_type, value):
        self.token_type = token_type
        self.value = value
        self.row = 1
        self.col = 1
        self.file_name = file_name

    def clean(self):
        self.token_type = TokenType.UNKNOWN
        self.value = ""
        self.row = 1
        self.col = 1

    def dump(self):
        return "{{{}[{}:{}]: {}, {}}}".format(self.file_name, self.row, self.col, self.token_type, self.value)


class Char(object):
    def __init__(self, is_eof, char):
        self.is_eof = is_eof
        self.char = char

    def dump(self):
        return "{%s, %s}" % (self.is_eof, self.char)


class Lexer(object):
    _key_words = {
        "package": TokenType.PACKAGE,
        "import": TokenType.IMPORT,
        "interface": TokenType.INTERFACE,
        "callback": TokenType.CALLBACK,
    }

    def __init__(self, idl_file_path):
        self.have_peek = False
        with open(idl_file_path, 'r') as idl_file:
            file_info = idl_file.read()
        self.data = file_info
        self.data_len = len(self.data)
        self.read_index = 0
        self.cur_token = Token(os.path.basename(idl_file_path), TokenType.UNKNOWN, "")
        self.cur_row = 1
        self.cur_col = 1

    def peek_char(self, peek_count=0):
        index = self.read_index + peek_count
        if index >= self.data_len:
            return Char(True, '0')
        return Char(False, self.data[index])

    def get_char(self):
        if self.read_index >= self.data_len:
            return Char(True, '0')
        read_index = self.read_index
        self.read_index += 1
        if self.data[read_index] == '\n':
            self.cur_row += 1
            self.cur_col = 1
        else:
            self.cur_col += 1
        return Char(False, self.data[read_index])

    def peek_token(self):
        if not self.have_peek:
            self.read_token()
            self.have_peek = True
        return self.cur_token

    def get_token(self):
        if not self.have_peek:
            self.read_token()
        self.have_peek = False
        return self.cur_token

    def read_token(self):
        self.cur_token.clean()
        while not self.peek_char().is_eof:
            new_char = self.peek_char()
            if new_char.char.isspace():
                self.get_char()
                continue
            self.cur_token.row = self.cur_row
            self.cur_token.col = self.cur_col
            if new_char.char.isalpha() or new_char.char == '_':
                self.read_id()
                return
            if new_char.char == '/':
                self.read_comment()
                return
            self.cur_token.value = new_char.char
            self.cur_token.token_type = TokenType.UNKNOWN
            self.get_char()
            return
        self.cur_token.token_type = TokenType.END_OF_FILE

    def read_id(self):
        token_value = []
        token_value.append(self.get_char().char)
        while not self.peek_char().is_eof:
            new_char = self.peek_char()
            if new_char.char.isalpha() or new_char.char.isdigit() or new_char.char == '_' or new_char.char == '.':
                token_value.append(new_char.char)
                self.get_char()
                continue
            break
        key = "".join(token_value)
        if key in self._key_words.keys():
            self.cur_token.token_type = self._key_words[key]
        else:
            self.cur_token.token_type = TokenType.ID
        self.cur_token.value = key

    def read_comment(self):
        token_value = []
        token_value.append(self.get_char().char)
        new_char = self.peek_char()
        if not new_char.is_eof:
            if new_char.char == '/':
                self.read_line_comment(token_value)
                return
            elif new_char.char == '*':
                self.read_block_comment(token_value)
                return
        self.cur_token.token_type = TokenType.UNKNOWN
        self.cur_token.value = "".join(token_value)

    def read_line_comment(self, token_value):
        token_value.append(self.get_char().char)
        while not self.peek_char().is_eof:
            new_char = self.get_char()
            if new_char.char == '\n':
                break
            token_value.append(new_char.char)
        self.cur_token.token_type = TokenType.COMMENT
        self.cur_token.value = "".join(token_value)

    def read_block_comment(self, token_value):
        # read *
        token_value.append(self.get_char().char)
        while not self.peek_char().is_eof:
            new_char = self.get_char()
            token_value.append(new_char.char)
            if new_char.char == '*' and self.peek_char().char == '/':
                token_value.append(self.get_char().char)
                break
        value = "".join(token_value)
        if value.endswith("*/"):
            self.cur_token.token_type = TokenType.COMMENT
        else:
            self.cur_token.token_type = TokenType.UNKNOWN
        self.cur_token.value = value


class Option(object):
    __instance = None

    def __new__(cls, *option_args, **option_kwargs):
        if cls.__instance is None:
            cls.__instance = object.__new__(cls, *option_args, **option_kwargs)
        return cls.__instance

    def __init__(self):
        self.language = ""
        self.mode = "ipc"
        self.gen_dir = ""
        self.root_package = ""
        self.root_path = ""
        self.idl_sources = []

    def load(self, opt_args):
        ret = True
        if opt_args.language != 'c' and opt_args.language != 'cpp':
            print("The language must be 'c' or 'cpp', please check input")
            ret = False
        else:
            self.language = opt_args.language

        if opt_args.mode != "ipc" and opt_args.mode != "passthrough":
            print("The hdi mode must be 'ipc' or 'passthrough', please check input")
            ret = False
        else:
            self.mode = opt_args.mode

        if opt_args.out == "":
            print("The gen_dir is empty, please check input")
            ret = False
        else:
            self.gen_dir = opt_args.out

        map_result = opt_args.root.split(":")
        if len(map_result) != 2:
            print("The package path is valid, please check input")
            ret = False
        else:
            self.root_package = map_result[0]
            self.root_path = map_result[1]

        if len(opt_args.file) == 0:
            print("The idl sources is empty, please check input")
            ret = False
        else:
            self.idl_sources = opt_args.file
        return ret

    def dump(self):
        result = {
            "language": self.language,
            "gen_dir": self.gen_dir,
            "root_package": self.root_package,
            "root_path": self.root_path,
            "idl_sources": self.idl_sources
        }
        return json.dumps(result)


class IdlType(object):
    INTERFACE = 1
    CALL_INTERFACE = 2
    CALLBACK = 3
    TYPES = 4


# file detail of idl file
class IdlDetail(object):
    def __init__(self, path):
        self.package = ""
        self.idl_type = IdlType.TYPES
        self.imports = []
        self.file_path = path

        file_name = os.path.basename(self.file_path)
        self.name = file_name.split('.')[0]

    # package + file name, like 'ohos.hdi.foo.v1_0.IFoo'
    def full_name(self):
        return "{}.{}".format(self.package, self.name)

    def dump(self):
        result = {
            "package": self.package,
            "type": self.idl_type,
            "imports": self.imports,
            "path": self.file_path
        }
        return json.dumps(result)


# module info of all idl
class ModuleInfo(object):
    def __init__(self):
        self.errors = []
        self.package = ""
        self.version = ""
        self.include_dirs = set()
        self.out_dir = ""
        self.sources = []
        self.proxy_sources = []
        self.stub_sources = []

    def json_info(self):
        include_dirs = sorted(list(self.include_dirs))
        self.sources.sort()
        self.proxy_sources.sort()
        self.stub_sources.sort()

        result = {
            "errors": self.errors,
            "package": self.package,
            "version": self.version,
            "include_dirs": include_dirs,
            "out_dir": self.out_dir,
            "sources": self.sources,
            "proxy_sources": self.proxy_sources,
            "stub_sources": self.stub_sources
        }
        return json.dumps(result)


class IdlParser(object):
    def __init__(self, parse_option):
        self.module_info = ModuleInfo()
        self.option = parse_option
        self.calc = Calculation(self.option)

    def parse(self, all_idl_files):
        all_idl_details = {}
        for idl_file in all_idl_files:
            idl_detail = self.parse_one(idl_file)
            all_idl_details[idl_detail.full_name()] = idl_detail
        self.parse_deps(all_idl_details)
        self.parse_module_info(all_idl_details)

    def parse_one(self, file_path):
        cur_idl_detail = IdlDetail(file_path)
        lex = Lexer(file_path)
        while lex.peek_token().token_type != TokenType.END_OF_FILE:
            cur_token_type = lex.peek_token().token_type
            if cur_token_type == TokenType.PACKAGE:
                self.parse_package(lex, cur_idl_detail)
            elif cur_token_type == TokenType.IMPORT:
                self.parse_import(lex, cur_idl_detail)
            elif cur_token_type == TokenType.CALLBACK:
                cur_idl_detail.idl_type = IdlType.CALLBACK
                lex.get_token()
            elif cur_token_type == TokenType.INTERFACE:
                self.parse_interface(lex, cur_idl_detail)
            else:
                lex.get_token()
        return cur_idl_detail

    def parse_package(self, lex, cur_idl_detail):
        lex.get_token()  # package token
        if lex.peek_token().token_type != TokenType.ID:
            self.module_info.errors.append("expected package name before {}".format(lex.peek_token().dump()))
            return
        if not self.parse_version(lex.get_token().value, cur_idl_detail):
            self.module_info.errors.append("failed to parse package name")

    def parse_version(self, package_name, cur_idl_detail):
        result = re.findall(r'\w+(?:\.\w+)*\.[V|v](\d+)_(\d+)', package_name)
        if len(result) > 0:
            cur_idl_detail.package = package_name
            major_version = result[0][0]
            minor_version = result[0][1]
            cur_idl_detail.version = "{}.{}".format(major_version, minor_version)
            return True
        return False

    def parse_import(self, lex, cur_idl_detail):
        lex.get_token()  # import token
        if lex.peek_token().token_type != TokenType.ID:
            self.module_info.errors.append("expected package name 3")
            return
        cur_idl_detail.imports.append(lex.get_token().value)

    def parse_interface(self, lex, cur_idl_detail):
        lex.get_token()  # interface token
        if lex.peek_token().token_type != TokenType.ID:
            self.module_info.errors.append("expected interface name")
            return
        interface_name = lex.get_token().value
        if interface_name != cur_idl_detail.name:
            self.module_info.errors.append(
                "interface name '{}' is not equal file name '{}'".format(interface_name, cur_idl_detail.name))
        if cur_idl_detail.idl_type != IdlType.CALLBACK:
            cur_idl_detail.idl_type = IdlType.INTERFACE

    def parse_deps(self, all_idl_details):
        for detail_name, idl_detail in all_idl_details.items():
            self.query_and_update_idl_type(idl_detail, all_idl_details)

    # update interface idl file type if the file import by other idl file
    def query_and_update_idl_type(self, idl_detail, all_idl_details):
        for other_name, other_detail in all_idl_details.items():
            if idl_detail.full_name() == other_name:
                continue
            if self.imported_by_other_idl(idl_detail, other_detail) and idl_detail.idl_type == IdlType.INTERFACE:
                idl_detail.idl_type = IdlType.CALL_INTERFACE
                break

    def imported_by_other_idl(self, idl_detail, other_detail):
        for import_name in other_detail.imports:
            if idl_detail.full_name() == import_name:
                return True
        return False

    def parse_module_info(self, all_idl_details):
        self.module_info.out_dir = self.option.gen_dir
        self.parse_sources(all_idl_details)

    def parse_sources(self, all_idl_details):
        self.module_info.include_dirs.add(self.option.gen_dir)
        for idl_detail in all_idl_details.values():
            if idl_detail.idl_type == IdlType.INTERFACE:
                self.module_info.package = idl_detail.package
                self.module_info.version = idl_detail.version
            self.module_info.include_dirs.add(self.calc.parse_include_dirs(idl_detail.package))

            sources, proxy_sources, sub_sources = self.calc.get_sources_info(idl_detail)
            self.module_info.sources.extend(sources)
            self.module_info.proxy_sources.extend(proxy_sources)
            self.module_info.stub_sources.extend(sub_sources)


class Calculation(object):
    __instance = None

    def __init__(self, calc_option):
        self.option = calc_option

    def __new__(cls, *calc_args, **calc_kwargs):
        if not cls.__instance:
            cls.__instance = super(Calculation, cls).__new__(cls)
            return cls.__instance
        else:
            return cls.__instance

    def parse_include_dirs(self, package):
        sub_package = self.get_sub_package(package)
        last_point_index = sub_package.rfind('.')
        package_without_version = sub_package[:last_point_index]
        package_dir_without_version = package_without_version.replace('.', os.sep)
        return os.path.join(self.option.gen_dir, package_dir_without_version)

    def get_source_file_dir(self, package):
        sub_package = self.get_sub_package(package)
        sub_package_dir = "{}{}".format(sub_package.replace('.', os.sep), os.sep)
        return os.path.join(self.option.gen_dir, sub_package_dir)

    # package is 'ohos.hdi.foo.v1_0'
    # -r ohos.hdi:./interface
    # sub_package is foo.v1_0
    def get_sub_package(self, package):
        if package.startswith(self.option.root_package):
            root_package_len = len(self.option.root_package)
            return package[root_package_len + 1:]
        return package

    # the camel name transalte under score case name
    def camel_to_under_score_case(self, name):
        result = []
        name_len = len(name)
        for index in range(name_len):
            cur_char = name[index]
            if cur_char.isupper():
                if index == 0:
                    result.append(cur_char.lower())
                    continue
                next_index = index + 1
                if next_index < name_len and name[next_index].islower():
                    result.append('_')
                result.append(cur_char.lower())
            else:
                result.append(cur_char)
        return "".join(result)

    def translate_file_name(self, file_name):
        under_line = '_'
        result = []
        name_len = len(file_name)
        for index in range(name_len):
            cur_char = file_name[index]
            if cur_char.isupper():
                if index > 1:
                    result.append(under_line)
                result.append(cur_char.lower())
            else:
                result.append(cur_char)
        return "".join(result)

    def translate_proxy_name(self, base_name):
        temp_name = "{}Proxy".format(base_name)
        return self.translate_file_name(temp_name)

    def translate_stub_name(self, base_name):
        temp_name = "{}Stub".format(base_name)
        return self.translate_file_name(temp_name)

    def translate_service_name(self, base_name):
        temp_name = "{}Service".format(base_name)
        return self.translate_file_name(temp_name)

    def translate_driver_name(self, base_name):
        temp_name = "{}Driver".format(base_name)
        return self.translate_file_name(temp_name)

    def get_interface_names(self, name):
        base_name = name[1:] if name.startswith("I") else name

        interface_name = self.translate_file_name(name)
        proxy_name = self.translate_proxy_name(base_name)
        stub_name = self.translate_stub_name(base_name)
        service_name = self.translate_service_name(base_name)
        driver_name = self.translate_driver_name(base_name)

        return interface_name, proxy_name, stub_name, service_name, driver_name

    def get_call_interface_names(self, name):
        base_name = name[1:] if name.startswith("I") else name

        interface_name = self.translate_file_name(name)
        proxy_name = self.translate_proxy_name(base_name)
        stub_name = self.translate_stub_name(base_name)
        service_name = self.translate_service_name(base_name)
        return interface_name, proxy_name, stub_name, service_name

    def get_callback_interface_names(self, name):
        base_name = name[1:] if name.startswith("I") else name

        interface_name = self.translate_file_name(name)
        proxy_name = self.translate_proxy_name(base_name)
        stub_name = self.translate_stub_name(base_name)
        service_name = self.translate_service_name(base_name)
        return interface_name, proxy_name, stub_name, service_name

    def get_type_names(self, name):
        base_name = self.translate_file_name(name)
        return base_name

    def header_file(self, name):
        return "{}.h".format(name)

    def c_source_file(self, name):
        return "{}.c".format(name)

    def cpp_source_file(self, name):
        return "{}.cpp".format(name)

    def get_sources_info(self, idl_detail):
        if idl_detail.idl_type == IdlType.INTERFACE:
            return self.get_interface_sources_info(idl_detail)
        elif idl_detail.idl_type == IdlType.CALL_INTERFACE:
            return self.get_call_interface_sources_info(idl_detail)
        elif idl_detail.idl_type == IdlType.CALLBACK:
            return self.get_callback_interface_sources_info(idl_detail)
        else:
            return self.get_types_sources_info(idl_detail)

    def get_interface_sources_info(self, idl_detail):
        if self.option.language == 'c':
            return self.get_c_interface_sources_info(idl_detail)
        elif self.option.language == 'cpp':
            return self.get_cpp_interface_sources_info(idl_detail)
        else:
            return [], [], []

    def get_call_interface_sources_info(self, idl_detail):
        if self.option.language == 'c':
            return self.get_c_call_interface_sources_info(idl_detail)
        elif self.option.language == 'cpp':
            return self.get_cpp_call_interface_sources_info(idl_detail)
        else:
            return [], [], []

    def get_callback_interface_sources_info(self, idl_detail):
        if self.option.language == 'c':
            return self.get_c_callback_interface_sources_info(idl_detail)
        elif self.option.language == 'cpp':
            return self.get_cpp_callback_interface_sources_info(idl_detail)
        else:
            return [], [], []

    def get_types_sources_info(self, idl_detail):
        if self.option.language == 'c':
            return self.get_c_types_sources_info(idl_detail)
        elif self.option.language == 'cpp':
            return self.get_cpp_types_sources_info(idl_detail)
        else:
            return [], [], []

    # get sources by c language
    def get_c_interface_sources_info(self, idl_detail):
        file_dir = self.get_source_file_dir(idl_detail.package)
        interface_name, proxy_name, stub_name, service_name, driver_name = self.get_interface_names(idl_detail.name)

        c_iface_header_file = os.path.join(file_dir, self.header_file(interface_name))
        c_client_proxy_source_file = os.path.join(file_dir, self.c_source_file(proxy_name))
        c_server_stub_header_file = os.path.join(file_dir, self.header_file(stub_name))
        c_server_stub_source_file = os.path.join(file_dir, self.c_source_file(stub_name))
        c_server_impl_source_file = os.path.join(file_dir, self.c_source_file(service_name))
        c_server_driver_source_file = os.path.join(file_dir, self.c_source_file(driver_name))

        sources = []
        libproxy_sources = []
        libstub_sources = []
        if self.option.mode == "ipc":
            sources.append(c_iface_header_file)
            sources.append(c_client_proxy_source_file)
            sources.append(c_server_stub_header_file)
            sources.append(c_server_stub_source_file)
            sources.append(c_server_impl_source_file)
            sources.append(c_server_driver_source_file)
            libproxy_sources.append(c_client_proxy_source_file)
            libstub_sources.append(c_server_stub_source_file)
        elif self.option.mode == "passthrough":
            sources.append(c_iface_header_file)
            sources.append(c_client_proxy_source_file)
            sources.append(c_server_impl_source_file)
            libproxy_sources.append(c_client_proxy_source_file)

        return sources, libproxy_sources, libstub_sources

    def get_c_call_interface_sources_info(self, idl_detail):
        file_dir = self.get_source_file_dir(idl_detail.package)
        interface_name, proxy_name, stub_name, service_name = self.get_call_interface_names(idl_detail.name)

        c_call_iface_header_file = os.path.join(file_dir, self.header_file(interface_name))
        c_call_client_proxy_source_file = os.path.join(file_dir, self.c_source_file(proxy_name))
        c_call_server_stub_header_file = os.path.join(file_dir, self.header_file(stub_name))
        c_call_server_stub_source_file = os.path.join(file_dir, self.c_source_file(stub_name))
        c_call_server_impl_header_file = os.path.join(file_dir, self.header_file(service_name))
        c_call_server_impl_source_file = os.path.join(file_dir, self.c_source_file(service_name))

        sources = []
        libproxy_sources = []
        libstub_sources = []
        if self.option.mode == "ipc":
            sources.append(c_call_iface_header_file)
            sources.append(c_call_client_proxy_source_file)
            sources.append(c_call_server_stub_header_file)
            sources.append(c_call_server_stub_source_file)
            sources.append(c_call_server_impl_header_file)
            sources.append(c_call_server_impl_source_file)
            libproxy_sources.append(c_call_client_proxy_source_file)
            libstub_sources.append(c_call_server_stub_source_file)
        elif self.option.mode == "passthrough":
            sources.append(c_call_iface_header_file)
            sources.append(c_call_server_impl_header_file)
            sources.append(c_call_server_impl_source_file)

        return sources, libproxy_sources, libstub_sources

    def get_c_callback_interface_sources_info(self, idl_detail):
        file_dir = self.get_source_file_dir(idl_detail.package)
        interface_name, proxy_name, stub_name, service_name = self.get_callback_interface_names(idl_detail.name)

        c_cb_iface_header_file = os.path.join(file_dir, self.header_file(interface_name))
        c_cb_client_proxy_source_file = os.path.join(file_dir, self.c_source_file(proxy_name))
        c_cb_server_stub_header_file = os.path.join(file_dir, self.header_file(stub_name))
        c_cb_server_stub_source_file = os.path.join(file_dir, self.c_source_file(stub_name))
        c_cb_server_impl_header_file = os.path.join(file_dir, self.header_file(service_name))
        c_cb_server_impl_source_file = os.path.join(file_dir, self.c_source_file(service_name))

        sources = []
        libproxy_sources = []
        libstub_sources = []
        if self.option.mode == "ipc":
            sources.append(c_cb_iface_header_file)
            sources.append(c_cb_client_proxy_source_file)
            sources.append(c_cb_server_stub_header_file)
            sources.append(c_cb_server_stub_source_file)
            sources.append(c_cb_server_impl_header_file)
            sources.append(c_cb_server_impl_source_file)
            libproxy_sources.append(c_cb_server_stub_source_file)
            libstub_sources.append(c_cb_client_proxy_source_file)
        elif self.option.mode == "passthrough":
            sources.append(c_cb_iface_header_file)
            sources.append(c_cb_server_impl_header_file)
            sources.append(c_cb_server_impl_source_file)

        return sources, libproxy_sources, libstub_sources

    def get_c_types_sources_info(self, idl_detail):
        file_dir = self.get_source_file_dir(idl_detail.package)
        base_name = self.get_type_names(idl_detail.name)

        c_types_header_file = os.path.join(file_dir, self.header_file(base_name))
        c_types_source_file = os.path.join(file_dir, self.c_source_file(base_name))

        sources = []
        libproxy_sources = []
        libstub_sources = []
        if self.option.mode == "ipc":
            sources.append(c_types_header_file)
            sources.append(c_types_source_file)
            libproxy_sources.append(c_types_source_file)
            libstub_sources.append(c_types_source_file)
        elif self.option.mode == "passthrough":
            sources.append(c_types_header_file)

        return sources, libproxy_sources, libstub_sources

    # get sources by cpp language
    def get_cpp_interface_sources_info(self, idl_detail):
        file_dir = self.get_source_file_dir(idl_detail.package)
        interface_name, proxy_name, stub_name, service_name, driver_name = self.get_interface_names(idl_detail.name)

        cpp_iface_header_file = os.path.join(file_dir, self.header_file(interface_name))
        cpp_client_proxy_header_file = os.path.join(file_dir, self.header_file(proxy_name))
        cpp_client_proxy_source_file = os.path.join(file_dir, self.cpp_source_file(proxy_name))
        cpp_server_stub_header_file = os.path.join(file_dir, self.header_file(stub_name))
        cpp_server_stub_source_file = os.path.join(file_dir, self.cpp_source_file(stub_name))
        cpp_server_impl_header_file = os.path.join(file_dir, self.header_file(service_name))
        cpp_server_impl_source_file = os.path.join(file_dir, self.cpp_source_file(service_name))
        cpp_server_driver_source_file = os.path.join(file_dir, self.cpp_source_file(driver_name))

        sources = []
        libproxy_sources = []
        libstub_sources = []
        if self.option.mode == "ipc":
            sources.append(cpp_iface_header_file)
            sources.append(cpp_client_proxy_header_file)
            sources.append(cpp_client_proxy_source_file)
            sources.append(cpp_server_stub_header_file)
            sources.append(cpp_server_stub_source_file)
            sources.append(cpp_server_impl_header_file)
            sources.append(cpp_server_impl_source_file)
            sources.append(cpp_server_driver_source_file)
            libproxy_sources.append(cpp_client_proxy_source_file)
            libstub_sources.append(cpp_server_stub_source_file)
        elif self.option.mode == "passthrough":
            sources.append(cpp_iface_header_file)
            sources.append(cpp_client_proxy_source_file)
            sources.append(cpp_server_impl_header_file)
            sources.append(cpp_server_impl_source_file)
            libproxy_sources.append(cpp_client_proxy_source_file)

        return sources, libproxy_sources, libstub_sources

    def get_cpp_call_interface_sources_info(self, idl_detail):
        file_dir = self.get_source_file_dir(idl_detail.package)
        interface_name, proxy_name, stub_name, service_name = self.get_call_interface_names(idl_detail.name)

        cpp_call_iface_header_file = os.path.join(file_dir, self.header_file(interface_name))
        cpp_call_client_proxy_header_file = os.path.join(file_dir, self.header_file(proxy_name))
        cpp_call_client_proxy_source_file = os.path.join(file_dir, self.cpp_source_file(proxy_name))
        cpp_call_server_stub_header_file = os.path.join(file_dir, self.header_file(stub_name))
        cpp_call_server_stub_source_file = os.path.join(file_dir, self.cpp_source_file(stub_name))
        cpp_call_server_impl_header_file = os.path.join(file_dir, self.header_file(service_name))
        cpp_call_server_impl_source_file = os.path.join(file_dir, self.cpp_source_file(service_name))

        sources = []
        libproxy_sources = []
        libstub_sources = []
        if self.option.mode == "ipc":
            sources.append(cpp_call_iface_header_file)
            sources.append(cpp_call_client_proxy_header_file)
            sources.append(cpp_call_client_proxy_source_file)
            sources.append(cpp_call_server_stub_header_file)
            sources.append(cpp_call_server_stub_source_file)
            sources.append(cpp_call_server_impl_header_file)
            sources.append(cpp_call_server_impl_source_file)
            libproxy_sources.append(cpp_call_client_proxy_source_file)
            libstub_sources.append(cpp_call_server_stub_source_file)
        elif self.option.mode == "passthrough":
            sources.append(cpp_call_iface_header_file)
            sources.append(cpp_call_server_impl_header_file)
            sources.append(cpp_call_server_impl_source_file)

        return sources, libproxy_sources, libstub_sources

    def get_cpp_callback_interface_sources_info(self, idl_detail):
        file_dir = self.get_source_file_dir(idl_detail.package)
        interface_name, proxy_name, stub_name, service_name = self.get_callback_interface_names(idl_detail.name)

        cpp_cb_iface_header_file = os.path.join(file_dir, self.header_file(interface_name))
        cpp_cb_client_proxy_header_file = os.path.join(file_dir, self.header_file(proxy_name))
        cpp_cb_client_proxy_source_file = os.path.join(file_dir, self.cpp_source_file(proxy_name))

        cpp_cb_server_stub_header_file = os.path.join(file_dir, self.header_file(stub_name))
        cpp_cb_server_stub_source_file = os.path.join(file_dir, self.cpp_source_file(stub_name))
        cpp_cb_server_impl_header_file = os.path.join(file_dir, self.header_file(service_name))
        cpp_cb_server_impl_source_file = os.path.join(file_dir, self.cpp_source_file(service_name))

        sources = []
        libproxy_sources = []
        libstub_sources = []
        if self.option.mode == "ipc":
            sources.append(cpp_cb_iface_header_file)
            sources.append(cpp_cb_client_proxy_header_file)
            sources.append(cpp_cb_client_proxy_source_file)
            sources.append(cpp_cb_server_stub_header_file)
            sources.append(cpp_cb_server_stub_source_file)
            sources.append(cpp_cb_server_impl_header_file)
            sources.append(cpp_cb_server_impl_source_file)
            libproxy_sources.append(cpp_cb_server_stub_source_file)
            libstub_sources.append(cpp_cb_client_proxy_source_file)
        elif self.option.mode == "passthrough":
            sources.append(cpp_cb_iface_header_file)
            sources.append(cpp_cb_server_impl_header_file)
            sources.append(cpp_cb_server_impl_source_file)

        return sources, libproxy_sources, libstub_sources

    def get_cpp_types_sources_info(self, idl_detail):
        file_dir = self.get_source_file_dir(idl_detail.package)
        base_name = self.get_type_names(idl_detail.name)

        cpp_types_header_file = os.path.join(file_dir, self.header_file(base_name))
        cpp_types_source_file = os.path.join(file_dir, self.cpp_source_file(base_name))

        sources = []
        libproxy_sources = []
        libstub_sources = []
        if self.option.mode == "ipc":
            sources.append(cpp_types_header_file)
            sources.append(cpp_types_source_file)
            libproxy_sources.append(cpp_types_source_file)
            libstub_sources.append(cpp_types_source_file)
        elif self.option.mode == "passthrough":
            sources.append(cpp_types_header_file)
        return sources, libproxy_sources, libstub_sources


def check_python_version():
    if sys.version_info < (3, 0):
        print("Please run with python version >= 3.0")
        sys.exit(-1)


if __name__ == "__main__":
    check_python_version()
    option_parser = argparse.ArgumentParser(
        description="Tools for generating compilation infomation of idl files",
    )

    option_parser.add_argument("-l",
                               "--language",
                               action="store",
                               help="language")
    option_parser.add_argument("-m",
                               "--mode",
                               default="ipc",
                               help="generate code of passthrough")
    option_parser.add_argument("-o",
                               "--out",
                               default=".",
                               required=True,
                               action="store",
                               help="direstory of generate file")
    option_parser.add_argument("-r",
                               "--root",
                               required=True,
                               action="store",
                               help="<root package>:<path>")
    option_parser.add_argument("-f",
                               "--file",
                               required=True,
                               action="append",
                               help="the idl file")
    args = option_parser.parse_args()
    option = Option()

    if not option.load(args):
        exit(-1)
    idl_parser = IdlParser(option)
    idl_parser.parse(option.idl_sources)
    sys.stdout.write(idl_parser.module_info.json_info())
