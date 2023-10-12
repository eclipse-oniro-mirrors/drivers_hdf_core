/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "util/string_helper.h"

#include <cstdarg>
#include <regex>

#include "securec.h"

#define RE_DEC_DIGIT "[0-9]+"

namespace OHOS {
namespace HDI {

const std::string StringHelper::TYPE_NAME_SEPARATOR = ".";
const std::string StringHelper::STRING_HELPER_NULL_STRING = "";

const std::regex RE_VERSION("[V|v]" "(" RE_DEC_DIGIT ")_(" RE_DEC_DIGIT ")");

std::vector<std::string> StringHelper::Split(std::string sources, const std::string &limit)
{
    std::vector<std::string> result;
    if (sources.empty()) {
        return result;
    }

    if (limit.empty()) {
        result.push_back(sources);
        return result;
    }

    size_t begin = 0;
    size_t pos = sources.find(limit, begin);
    while (pos != std::string::npos) {
        std::string element = sources.substr(begin, pos - begin);
        if (!element.empty()) {
            result.push_back(element);
        }
        begin = pos + limit.size();
        pos = sources.find(limit, begin);
    }

    if (begin < sources.size()) {
        std::string element = sources.substr(begin);
        result.push_back(element);
    }
    return result;
}

bool StringHelper::StartWith(const std::string &value, char prefix)
{
    return value.find(prefix) == 0;
}

bool StringHelper::StartWith(const std::string &value, const std::string &prefix)
{
    return value.find(prefix) == 0;
}

bool StringHelper::EndWith(const std::string &value, char suffix)
{
    if (value.empty()) {
        return false;
    }
    return value.back() == suffix;
}

bool StringHelper::EndWith(const std::string &value, const std::string &suffix)
{
    size_t index = value.rfind(suffix);
    if (index == std::string::npos) {
        return false;
    }

    return index + suffix.size() == value.size();
}

std::string StringHelper::Replace(const std::string &value, char oldChar, char newChar)
{
    if (value.empty() || oldChar == newChar) {
        return value;
    }

    std::string result = value;
    for (size_t i = 0; i < result.size(); i++) {
        if (result[i] != oldChar) {
            continue;
        }
        result[i] = newChar;
    }
    return result;
}

std::string StringHelper::Replace(const std::string &value, const std::string &oldstr, const std::string &newstr)
{
    std::string result = value;
    size_t pos = 0;
    while ((pos = result.find(oldstr, pos)) != std::string::npos) {
        result.replace(pos, oldstr.size(), newstr);
        pos += newstr.size();
    }
    return result;
}

std::string StringHelper::Replace(
    const std::string &value, size_t position, const std::string &substr, const std::string &newstr)
{
    if (position >= value.size()) {
        return value;
    }

    std::string prefix = value.substr(0, position);
    std::string suffix = value.substr(position);
    return prefix + Replace(suffix, substr, newstr);
}

std::string StringHelper::Replace(const std::string &value, size_t position, size_t len, const std::string &newStr)
{
    if (position >= value.size() || len == 0) {
        return value;
    }

    std::string prefix = value.substr(0, position);
    std::string suffix = value.substr(position);
    return prefix + newStr + suffix;
}

std::string StringHelper::SubStr(const std::string &value, size_t start, size_t end)
{
    if (value.empty() || start == std::string::npos || start >= end) {
        return "";
    }
    return (end == std::string::npos) ? value.substr(start) : value.substr(start, end - start);
}

std::string StringHelper::StrToLower(const std::string &value)
{
    std::string result = value;
    for (size_t i = 0; i < result.size(); i++) {
        if (std::isupper(result[i])) {
            result[i] = std::tolower(result[i]);
        }
    }
    return result;
}

std::string StringHelper::StrToUpper(const std::string &value)
{
    std::string result = value;
    for (size_t i = 0; i < result.size(); i++) {
        if (std::islower(result[i])) {
            result[i] = std::toupper(result[i]);
        }
    }
    return result;
}

std::string StringHelper::Format(const char *format, ...)
{
    va_list args;
    va_list argsCopy;

    va_start(args, format);
    va_copy(argsCopy, args);

    char buf[lineMaxSize] = {0};
    int len = vsnprintf_s(buf, lineMaxSize, lineMaxSize - 1, format, args);
    if (len <= 0) {
        va_end(args);
        va_end(argsCopy);
        return "";
    }

    va_end(args);
    va_end(argsCopy);
    return std::string(buf, len);
}

std::string StringHelper::GetVersionName(std::string typeName)
{
    std::vector<std::string> result = Split(typeName, TYPE_NAME_SEPARATOR);
    if (result.size() < TYPE_NAME_MIN_PART) {
        return STRING_HELPER_NULL_STRING;
    }

    return result[VERSION_NAME_POSITION];
}

std::string StringHelper::GetMajorVersionName(std::string typeName)
{
    std::string result = GetVersionName(typeName);
    std::cmatch ret;
    if (!std::regex_match(result.c_str(), ret, RE_VERSION)) {
        return STRING_HELPER_NULL_STRING;
    }

    return ret.str(RE_MAJOR_VERSION_INDEX);
}

std::string StringHelper::GetMinorVersionName(std::string typeName)
{
    std::string result = GetVersionName(typeName);
    std::cmatch ret;
    if (!std::regex_match(result.c_str(), ret, RE_VERSION)) {
        return STRING_HELPER_NULL_STRING;
    }

    return ret.str(RE_MINOR_VERSION_INDEX);
}
std::string StringHelper::GetRealTypeName(std::string typeName)
{
    std::vector<std::string> result = Split(typeName, TYPE_NAME_SEPARATOR);
    if (result.size() < TYPE_NAME_MIN_PART) {
        return STRING_HELPER_NULL_STRING;
    }

    return result[TYPE_NAME_POSITION];
}

bool StringHelper::isCorrectVersion(std::string typeName, std::string importInfo)
{
    return GetVersionName(typeName) == GetVersionName(importInfo);
}

} // namespace HDI
} // namespace OHOS