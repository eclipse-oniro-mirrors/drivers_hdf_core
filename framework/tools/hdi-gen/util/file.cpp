/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "util/file.h"

#include <climits>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>

#include "util/common.h"
#include "util/logger.h"
#include "util/string_helper.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
File::File(const std::string &path, unsigned int mode) : mode_(mode)
{
    if (path.empty()) {
        return;
    }

    if (mode_ & READ) {
        OpenByRead(path);
        return;
    }

    if (mode_ & WRITE) {
        fd_ = fopen(path.c_str(), "w+");
    } else if (mode_ & APPEND) {
        fd_ = fopen(path.c_str(), "a+");
    }

    if (fd_ == nullptr) {
        Logger::E(TAG, "can't open '%s'", path.c_str());
        return;
    }

    path_ = RealPath(path);
}

File::~File()
{
    Close();
}

void File::OpenByRead(const std::string &path)
{
    if (!CheckValid(path)) {
        Logger::E(TAG, "failed to check path '%s'", path.c_str());
        return;
    }

    std::string realPath = RealPath(path);
    if (realPath.empty()) {
        Logger::E(TAG, "invalid path '%s'", path.c_str());
        return;
    }

    fd_ = fopen(realPath.c_str(), "r");
    if (fd_ == nullptr) {
        Logger::E(TAG, "can't open '%s'", realPath.c_str());
        return;
    }

    path_ = realPath;
}

char File::GetChar()
{
    char c = PeekChar();

    if (position_ + 1 <= size_) {
        position_++;

        if (c != '\n') {
            columnNo_++;
        } else {
            columnNo_ = 1;
            lineNo_++;
        }
    }
    return c;
}

char File::PeekChar()
{
    if (position_ + 1 > size_) {
        size_t size = Read();
        if (size == 0) {
            isEof_ = true;
        }
    }

    return buffer_[position_];
}

bool File::IsEof() const
{
    return isEof_ || buffer_[position_] == -1;
}

size_t File::Read()
{
    if (isEof_ || isError_) {
        return -1;
    }

    std::fill(buffer_, buffer_ + BUFFER_SIZE, 0);
    size_t count = fread(buffer_, 1, BUFFER_SIZE - 1, fd_);
    if (count < BUFFER_SIZE - 1) {
        isError_ = ferror(fd_) != 0;
        buffer_[count] = -1;
    }
    size_ = count;
    position_ = 0;
    return count;
}

size_t File::ReadData(void *data, size_t size) const
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    if (fd_ == nullptr) {
        return 0;
    }

    return fread(data, 1, size, fd_);
}

bool File::WriteData(const void *data, size_t size) const
{
    if (data == nullptr || size == 0) {
        return true;
    }

    if (fd_ == nullptr || !(mode_ & (WRITE | APPEND))) {
        return false;
    }

    size_t count = fwrite(data, size, 1, fd_);
    return count == 1;
}

void File::Flush()
{
    if ((mode_ & (WRITE | APPEND)) && fd_ != nullptr) {
        fflush(fd_);
    }
}

bool File::Reset()
{
    if (fd_ == nullptr) {
        return false;
    }

    return fseek(fd_, 0, SEEK_SET) == 0;
}

bool File::Skip(long size)
{
    if (fd_ == nullptr) {
        return false;
    }

    return fseek(fd_, size, SEEK_CUR) == 0;
}

void File::Close()
{
    if (fd_ != nullptr) {
        fclose(fd_);
        fd_ = nullptr;
    }
}

bool File::CreateParentDir(const std::string &path)
{
    if (!access(path.c_str(), F_OK | R_OK | W_OK)) {
        return true;
    }

    size_t pos = 1;
    while ((pos = path.find(SEPARATOR, pos)) != std::string::npos) {
        std::string partPath = StringHelper::SubStr(path, 0, pos);
        struct stat st;
        if (stat(partPath.c_str(), &st) < 0) {
            if (errno != ENOENT) {
                return false;
            }

#ifndef __MINGW32__
            if (mkdir(partPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
#else
            if (mkdir(partPath.c_str()) < 0) {
#endif
                return false;
            }
        } else if (!S_ISDIR(st.st_mode)) {
            return false;
        }
        pos += 1;
    }
    return true;
}

std::string File::AdapterPath(const std::string &path)
{
#ifndef __MINGW32__
    std::string newPath = StringHelper::Replace(path, '\\', '/');
#else
    std::string newPath = StringHelper::Replace(path, '/', '\\');
#endif

    // "foo/v1_0//ifoo.h" -> "foo/v1_0/ifoo.h"
    StringBuilder adapterPath;
    bool hasSep = false;
    for (size_t i = 0; i < newPath.size(); i++) {
        char c = newPath[i];
        if (c == SEPARATOR) {
            if (hasSep) {
                continue;
            }
            adapterPath.Append(c);
            hasSep = true;
        } else {
            adapterPath.Append(c);
            hasSep = false;
        }
    }
    return adapterPath.ToString();
}

std::string File::AdapterRealPath(const std::string &path)
{
    if (path.empty()) {
        return "";
    }
    return RealPath(File::AdapterPath(path));
}

std::string File::RealPath(const std::string &path)
{
    if (path.empty()) {
        return "";
    }

    char realPath[PATH_MAX + 1];
#ifdef __MINGW32__
    char *absPath = _fullpath(realPath, path.c_str(), PATH_MAX);
#else
    char *absPath = realpath(path.c_str(), realPath);
#endif
    return absPath == nullptr ? "" : absPath;
}

bool File::CheckValid(const std::string &path)
{
    if (access(path.c_str(), F_OK | R_OK | W_OK)) {
        return false;
    }

    struct stat st;
    if (stat(path.c_str(), &st) < 0) {
        return false;
    }

    if (S_ISDIR(st.st_mode)) {
        return false;
    }

    return true;
}

size_t File::GetHashKey()
{
    StringBuilder fileStr;
    while (!IsEof()) {
        fileStr.Append(GetChar());
    }

    return std::hash<std::string>()(fileStr.ToString());
}
} // namespace HDI
} // namespace OHOS