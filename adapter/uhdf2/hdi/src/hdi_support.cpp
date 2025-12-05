/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include "hdi_support.h"
#include <dlfcn.h>
#include <map>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <unistd.h>

#include <dirent.h>
#include "hdf_base.h"
#include "hdf_core_log.h"

#define HDF_LOG_TAG load_hdi

namespace {
#ifdef __LITEOS__
static constexpr const char *HDI_SO_EXTENSION = ".so";
#else
static constexpr const char *HDI_SO_EXTENSION = ".z.so";
#endif

constexpr size_t INTERFACE_MATCH_RESIZE = 4;
constexpr size_t INTERFACE_VERSION_MAJOR_INDEX = 1;
constexpr size_t INTERFACE_VERSION_MINOR_INDEX = 2;
constexpr size_t INTERFACE_NAME_INDEX = 3;
using HdiImplInstanceFunc = void *(*)(void);
using HdiImplReleaseFunc = void (*)(void *);
} // namespace

constexpr uint32_t INVALID_MINOR_VERSION = 0xFFFFFFFF;

struct LibImplInfo {
    std::string libNameBase;
    uint32_t majorVersion;
    uint32_t minorVersion;
    std::string extension;
    std::string libraryName;    // whole libname
};

static std::string ToLibImplName(const LibImplInfo& libImplInfo)
{
    std::ostringstream wholeLibName;
    wholeLibName << libImplInfo.libNameBase << "_" <<
        libImplInfo.majorVersion << "." << libImplInfo.minorVersion << libImplInfo.extension;
    return wholeLibName.str();
}

static std::string TransFileName(const std::string &interfaceName)
{
    if (interfaceName.empty()) {
        return interfaceName;
    }

    std::string result;
    for (size_t i = 0; i < interfaceName.size(); i++) {
        char c = interfaceName[i];
        if (std::isupper(c) != 0) {
            if (i > 1) {
                result += '_';
            }
            result += std::tolower(c);
        } else {
            result += c;
        }
    }
    return result;
}

static LibImplInfo ParseLibName(const std::string &interfaceName, const std::string &serviceName,
    uint32_t versionMajor, uint32_t versionMinor)
{
    std::ostringstream libNameBase;
    libNameBase << "lib" << interfaceName << "_" << serviceName;
    LibImplInfo implInfo;
    implInfo.libNameBase = libNameBase.str();
    implInfo.majorVersion = versionMajor;
    implInfo.minorVersion = versionMinor;
    implInfo.extension = HDI_SO_EXTENSION;
    libNameBase << "_" << versionMajor << "." << versionMinor << HDI_SO_EXTENSION;
    implInfo.libraryName = libNameBase.str();
    return implInfo;
}

struct HdiImpl {
    HdiImpl() : handler(nullptr), constructor(nullptr), destructor(nullptr), useCount(0) {}
    ~HdiImpl() = default;
    void Unload()
    {
        if (handler != nullptr) {
            dlclose(handler);
        }
    }
    void *handler;
    void *(*constructor)(void);
    void (*destructor)(void *);
    uint32_t useCount;
    LibImplInfo libImplInfo;
};

static std::map<std::string, HdiImpl> g_hdiConstructorMap;
static std::mutex g_loaderMutex;

static std::map<std::string, HdiImpl> g_hdiMaxConstructorMap;

static int32_t ParseInterface(
    const std::string &desc, std::string &interface, LibImplInfo &libInfo, const char *serviceName)
{
    static const std::regex reInfDesc("[a-zA-Z_][a-zA-Z0-9_]*(?:\\.[a-zA-Z_][a-zA-Z0-9_]*)*\\."
                                    "[V|v]([0-9]+)_([0-9]+)\\."
                                    "([a-zA-Z_][a-zA-Z0-9_]*)");
    std::smatch result;
    if (!std::regex_match(desc, result, reInfDesc)) {
        return HDF_FAILURE;
    }

    if (result.size() < INTERFACE_MATCH_RESIZE) {
        return HDF_FAILURE;
    }

    const auto &majorVersion = result[INTERFACE_VERSION_MAJOR_INDEX].str();
    const auto &minorVersion = result[INTERFACE_VERSION_MINOR_INDEX].str();

    bool isNumeric = std::all_of(majorVersion.begin(), majorVersion.end(), ::isdigit);
    isNumeric = isNumeric && std::all_of(minorVersion.begin(), minorVersion.end(), ::isdigit);
    if (!isNumeric) {
        return HDF_FAILURE;
    }

    uint32_t versionMajor = std::stoul(majorVersion);
    uint32_t versionMinor = std::stoul(minorVersion);
    std::string interfaceName = result[INTERFACE_NAME_INDEX];

    interface = interfaceName[0] == 'I' ? interfaceName.substr(1) : interfaceName;
    if (interface.empty()) {
        return HDF_FAILURE;
    }

    libInfo = ParseLibName(TransFileName(interface), serviceName, versionMajor, versionMinor);
    return HDF_SUCCESS;
}

static int32_t OpenHdiServiceImpl(const LibImplInfo& libInfo, const std::string& interfaceName, HdiImpl &hdiImpl)
{
    const std::string &libName = libInfo.libraryName;
    Dl_namespace ns_ps;
    hdiImpl.handler = dlopen(libName.c_str(), RTLD_LAZY);
    if ((hdiImpl.handler == nullptr) && !dlns_get("passthrough", &ns_ps)) {
        hdiImpl.handler = dlopen_ns(&ns_ps, libName.c_str(), RTLD_LAZY);
    }
    if (hdiImpl.handler == nullptr) {
        HDF_LOGE("%{public}s dlopen '%{public}s' failed.", __func__, libName.c_str());
        return HDF_FAILURE;
    }
    
    std::string symName = interfaceName + "ImplGetInstance";
    hdiImpl.constructor = reinterpret_cast<HdiImplInstanceFunc>(dlsym(hdiImpl.handler, symName.data()));
    if (hdiImpl.constructor == nullptr) {
        HDF_LOGE("%{public}s failed to get symbol of '%{public}s', %{public}s", __func__, symName.c_str(), dlerror());
        hdiImpl.Unload();
        return HDF_FAILURE;
    }
    std::string desSymName = interfaceName + "ImplRelease";
    hdiImpl.destructor = reinterpret_cast<HdiImplReleaseFunc>(dlsym(hdiImpl.handler, desSymName.data()));
    if (hdiImpl.destructor == nullptr) {
        HDF_LOGW("%{public}s failed to get symbol of '%{public}s', %{public}s",
            __func__, desSymName.c_str(), dlerror());
    }
    hdiImpl.libImplInfo = libInfo;
    return HDF_SUCCESS;
}

static void *LoadHdiImplDirectly(const LibImplInfo& libInfo, const std::string& interfaceName, HdiImpl &hdiImpl)
{
    const std::string libName = libInfo.libraryName;
    void *implInstance = hdiImpl.constructor();
    g_hdiConstructorMap.emplace(std::make_pair(libName, std::move(hdiImpl)));

    return implInstance;
}

bool RegexMatch(const std::string &fileName,
                const LibImplInfo &implInfo, LibImplInfo &pattenLibImplInfo)
{
    constexpr uint32_t libNameIndex = 0;
    constexpr uint32_t minorIndex = 1;
    std::ostringstream ss;
    ss << implInfo.libNameBase << "_" << implInfo.majorVersion << "\\.(0|[1-9][0-9]*)" << implInfo.extension;
    std::regex re = std::regex(ss.str());
    std::smatch result;
    if (!std::regex_match(fileName, result, re)) {
        return false;
    }
    pattenLibImplInfo = implInfo;
    pattenLibImplInfo.libraryName = result[libNameIndex].str();
    pattenLibImplInfo.minorVersion = stoul(result[minorIndex]);
    return true;
}

void SearchMatchedLibraryInDir(const LibImplInfo& implInfo,
    const char *directory, uint32_t &maxMinorVersion)
{
    struct dirent *entry;
    if (directory == nullptr) {
        return;
    }
    DIR *dir = opendir(directory);
    if (dir == nullptr) {
        HDF_LOGW("%{public}s: failed to open %{public}s, error: %{public}s",
            __func__, directory, strerror(errno));
        return;
    }
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || entry->d_type != DT_REG) {
            continue;
        }
        std::string libraryName = entry->d_name;
        LibImplInfo pattenLibImplInfo;
        if (RegexMatch(libraryName, implInfo, pattenLibImplInfo)) {
            if (maxMinorVersion == INVALID_MINOR_VERSION ||
                maxMinorVersion < pattenLibImplInfo.minorVersion) {
                maxMinorVersion = pattenLibImplInfo.minorVersion;
            }
        }
    }
    closedir(dir);
}

static void *ConstructMatchedLibraryInDirs(const LibImplInfo& implInfo,
                                            const std::string &interfaceName)
{
    const char * const paths[] = {
        "/vendor/lib64",
        "/vendor/lib64/passthrough",
        "/chip_prod/lib64",
        "/chip_prod/lib64/passthrough",
        "/vendor/lib",
        "/vendor/lib/passthrough",
        "/chip_prod/lib",
        "/chip_prod/lib/passthrough"
    };

    uint32_t maxMinorVersion = INVALID_MINOR_VERSION;
    // find matched library(max minor version) in the system
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        const char *path = paths[i];
        SearchMatchedLibraryInDir(implInfo, path, maxMinorVersion);
    }
    if (maxMinorVersion == INVALID_MINOR_VERSION) {
        HDF_LOGE("%{public}s, no matched library, libbase: %{public}s, major version: %{public}d",
            __func__, implInfo.libNameBase.c_str(), implInfo.majorVersion);
        return nullptr;
    }
    LibImplInfo tmpLibImplInfo = implInfo;
    tmpLibImplInfo.minorVersion = maxMinorVersion;
    tmpLibImplInfo.libraryName = ToLibImplName(tmpLibImplInfo);

    void *impl = nullptr;
    auto it = g_hdiConstructorMap.find(tmpLibImplInfo.libraryName);
    if (it != g_hdiConstructorMap.end()) {
        impl = it->second.constructor();
        g_hdiMaxConstructorMap.emplace(
            std::make_pair(tmpLibImplInfo.libNameBase, std::move(it->second)));
        g_hdiConstructorMap.erase(it);
    } else {
        HdiImpl hdiImpl;
        if (OpenHdiServiceImpl(tmpLibImplInfo, interfaceName, hdiImpl) != HDF_SUCCESS) {
            HDF_LOGE("%{public}s, OpenHdiServiceImpl failed, library: %{public}s.",
                __func__, tmpLibImplInfo.libraryName.c_str());
            return nullptr;
        }

        impl = hdiImpl.constructor();
        g_hdiMaxConstructorMap.emplace(std::make_pair(tmpLibImplInfo.libNameBase, std::move(hdiImpl)));
        if (tmpLibImplInfo.minorVersion < implInfo.minorVersion) {
            return nullptr;
        }
    }
    return impl;
}

static void *LoadHdiImplPatternly(const LibImplInfo& libImplInfo, const std::string& interfaceName)
{
    auto it = g_hdiMaxConstructorMap.find(libImplInfo.libNameBase);
    if (it != g_hdiMaxConstructorMap.end()) {
        if (libImplInfo.majorVersion != it->second.libImplInfo.majorVersion ||
            libImplInfo.minorVersion > it->second.libImplInfo.minorVersion) {
            return nullptr;
        }
        return it->second.constructor();
    }
    return ConstructMatchedLibraryInDirs(libImplInfo, interfaceName);
}

/*
 * service name: xxx_service
 * interface descriptor name: ohos.hdi.sample.v1_0.IFoo, the last two are version and interface base name
 * interface: Foo
 * versionMajor: 1
 * versionMinor: 0
 * library name: libfoo_xxx_service_1.0.z.so
 * method name: FooImplGetInstance
 */
void *LoadHdiImpl(const char *desc, const char *serviceName)
{
    if (desc == nullptr || serviceName == nullptr || strlen(desc) == 0 || strlen(serviceName) == 0) {
        HDF_LOGE("%{public}s invalid interface descriptor or service name", __func__);
        return nullptr;
    }

    std::string interfaceName;
    LibImplInfo libInfo;
    if (ParseInterface(desc, interfaceName, libInfo, serviceName) != HDF_SUCCESS) {
        HDF_LOGE("failed to parse hdi interface info from '%{public}s'", desc);
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(g_loaderMutex);
    auto constructor = g_hdiConstructorMap.find(libInfo.libraryName);
    if (constructor != g_hdiConstructorMap.end()) {
        return constructor->second.constructor();
    }

    constructor = g_hdiMaxConstructorMap.find(libInfo.libNameBase);
    if (constructor != g_hdiMaxConstructorMap.end()) {
        if (libInfo.libraryName == constructor->second.libImplInfo.libraryName) {
            return constructor->second.constructor();
        }
    }

    HdiImpl hdiImpl;
    auto ret = OpenHdiServiceImpl(libInfo, interfaceName, hdiImpl);
    (void)ret;

    if (hdiImpl.handler == nullptr) {
        HDF_LOGW("%{public}s failed to dlopen, %{public}s", __func__, dlerror());
        return LoadHdiImplPatternly(libInfo, interfaceName);
    }

    return LoadHdiImplDirectly(libInfo, interfaceName, hdiImpl);
}

void UnloadHdiImpl(const char *desc, const char *serviceName, void *impl)
{
    if (desc == nullptr || impl == nullptr) {
        return;
    }

    std::string interfaceName;
    LibImplInfo libInfo;
    if (ParseInterface(desc, interfaceName, libInfo, serviceName) != HDF_SUCCESS) {
        HDF_LOGE("%{public}s: failed to parse hdi interface info from '%{public}s'", __func__, desc);
        return;
    }
    std::lock_guard<std::mutex> lock(g_loaderMutex);
    auto constructor = g_hdiConstructorMap.find(libInfo.libraryName);
    if (constructor != g_hdiConstructorMap.end() && constructor->second.destructor != nullptr) {
        constructor->second.destructor(impl);
        return;
    }

    constructor = g_hdiMaxConstructorMap.find(libInfo.libNameBase);
    if (constructor != g_hdiMaxConstructorMap.end() && constructor->second.destructor != nullptr) {
        constructor->second.destructor(impl);
    }
}