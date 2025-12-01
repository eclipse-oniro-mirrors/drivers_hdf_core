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

struct LibImplInfo {
    std::string libNameBase;
    uint32_t majorVersion;
    uint32_t minorVersion;
    std::string extension;
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
    return implInfo;
}

struct HdiImpl {
    HdiImpl(const std::string &ifname, const std::string &libname) : interfaceName(ifname), mLibraryName(libname),
        handler(nullptr), constructor(nullptr), destructor(nullptr), useCount(0) {}
    ~HdiImpl() = default;

    int32_t Load()
    {
        // mLibraryName must be available
        Dl_namespace ns_ps;
        handler = dlopen(mLibraryName.c_str(), RTLD_LAZY);
        if ((handler == nullptr) && !dlns_get("passthrough", &ns_ps)) {
            handler = dlopen_ns(&ns_ps, mLibraryName.c_str(), RTLD_LAZY);
        }
        if (handler == nullptr) {
            HDF_LOGE("%{public}s, failed to dlopen %{public}s, %{public}s", __func__, mLibraryName.c_str(), dlerror());
            return HDF_FAILURE;
        }

        std::string symName = interfaceName + "ImplGetInstance";
        constructor = reinterpret_cast<HdiImplInstanceFunc>(dlsym(handler, symName.data()));
        if (constructor == nullptr) {
            HDF_LOGE("%{public}s failed to get symbol of '%{public}s', %{public}s",
                __func__, symName.c_str(), dlerror());
            Unload();
            return HDF_FAILURE;
        }
        std::string desSymName = interfaceName + "ImplRelease";
        destructor = reinterpret_cast<HdiImplReleaseFunc>(dlsym(handler, desSymName.data()));
        if (destructor == nullptr) {
            HDF_LOGW("%{public}s failed to get symbol of '%{public}s', %{public}s",
                __func__, desSymName.c_str(), dlerror());
        }
        return HDF_SUCCESS;
    }

    void Unload()
    {
        if (handler != nullptr) {
            if (useCount != 0) {
                HDF_LOGW("%{public}s, Unload %{public}s, useCount: %{public}u",
                    __func__, interfaceName.c_str(), useCount);
                return;
            }
            dlclose(handler);
            handler = nullptr;
            constructor = nullptr;
            destructor = nullptr;
            useCount = 0;
        }
    }

    void *Construct()
    {
        if (handler == nullptr) {
            int32_t ret = Load();
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%{public}s, Construct and load failed, ret %{public}d", __func__, ret);
                return nullptr;
            }
        }

        if (constructor == nullptr) {
            HDF_LOGE("%{public}s constructor is nullptr", __func__);
            return nullptr;
        }

        void *impl = constructor();
        if (impl) {
            ++useCount;
        }
        return impl;
    }
    
    void Destruct(void *impl)
    {
        if (useCount > 0) {
            --useCount;
        }

        if (destructor != nullptr) {
            destructor(impl);
        } else {
            HDF_LOGW("%{public}s destructor is nullptr", __func__);
        }
        if (useCount == 0) {
            Unload();
        }
    }

private:
    const std::string interfaceName;      // interface name
    const std::string mLibraryName;           // shared library name;
    void *handler;
    void *(*constructor)(void);
    void (*destructor)(void *);
    uint32_t useCount;
};

static std::mutex g_loaderMutex;

class HdiImplConstructorDelegate {
    struct HdiImplConstructor {
        struct LibImplInfo libImplInfo;
        struct HdiImpl hdiImpl;
    };

public:
    void *Construct(const LibImplInfo& implInfo, const std::string &interfaceName);
    void Destruct(const LibImplInfo &implInfo, void *impl);

    static HdiImplConstructorDelegate& GetInstance()
    {
        static HdiImplConstructorDelegate delegate;
        return delegate;
    }
    HdiImplConstructorDelegate(const HdiImplConstructorDelegate&) = delete;
    HdiImplConstructorDelegate& operator = (const HdiImplConstructorDelegate&) = delete;

private:
    HdiImplConstructorDelegate() = default;
    void *ConstructFromCache(const LibImplInfo& implInfo, bool &misMatch);
    void *SearchMatchedLibrary(const LibImplInfo& implInfo, const std::string &interfaceName);
    void *SearchMatchedLibraryInDirs(const LibImplInfo& implInfo, const std::string &interfaceName);
    void *SearchMatchedLibraryInDir(const LibImplInfo& implInfo,
        const std::string &interfaceName, const char *directory, bool &misMatch);
    bool RegexMatch(const std::string &fileName, const LibImplInfo &implInfo, LibImplInfo &fileImplInfo);
private:
    std::vector<HdiImplConstructor> hdiImplConstructorVect;
};

void *HdiImplConstructorDelegate::Construct(const LibImplInfo& implInfo, const std::string &interfaceName)
{
    bool misMatch = false;
    void *impl = ConstructFromCache(implInfo, misMatch);
    if (impl != nullptr || misMatch) {
        return impl;
    }
    // interface name for XxxImplGetInstance/XxxImplRelease symbol
    impl = SearchMatchedLibrary(implInfo, interfaceName);
    if (impl == nullptr) {
        HDF_LOGE("%{public}s: can not find matched library, pattern %{public}s_%{public}d.X%{public}s.",
            __func__, implInfo.libNameBase.c_str(), implInfo.majorVersion, implInfo.extension.c_str());
    }

    return impl;
}

void HdiImplConstructorDelegate::Destruct(const LibImplInfo &implInfo, void *impl)
{
    for (auto &elem : hdiImplConstructorVect) {
        if (implInfo.libNameBase == elem.libImplInfo.libNameBase &&
            implInfo.majorVersion == elem.libImplInfo.majorVersion) {
            if (implInfo.minorVersion > elem.libImplInfo.minorVersion) {
                HDF_LOGW("%{public}s: Destruct impl library: libasename: %{public}s, "
                    "majorVersion: %{public}u, minorVersion: %{public}u; loaded impl library: libasename: %{public}s, "
                    "majorVersion: %{public}u, minorVersion: %{public}u.", __func__, implInfo.libNameBase.c_str(),
                    implInfo.majorVersion, implInfo.minorVersion, elem.libImplInfo.libNameBase.c_str(),
                    elem.libImplInfo.majorVersion, elem.libImplInfo.minorVersion);
                return;
            }
            elem.hdiImpl.Destruct(impl);
            return;
        }
    }
    return;
}

/**
 * @brief try construct service implementation object from cache builder
 *
 * @param implInfo resovled library information .
 * @param misMatch true if cache builder could construct same libNameBase & majorVersion
 * but smaller minorVersion else false.
 * @param ashmem Pointer to the shared memory created.
 * @return Returns <b>DDK_SUCCESS</b> if the operation is successful; returns a negative value otherwise.
 */
void* HdiImplConstructorDelegate::ConstructFromCache(const LibImplInfo& implInfo, bool &misMatch)
{
    misMatch = false;
    for (auto &elem : hdiImplConstructorVect) {
        if (implInfo.libNameBase == elem.libImplInfo.libNameBase &&
            implInfo.majorVersion == elem.libImplInfo.majorVersion) {
            // One majar version has unique minor version implementation
            if (implInfo.minorVersion > elem.libImplInfo.minorVersion) {
                HDF_LOGW("%{public}s, already load %{public}s, but try load %{public}s.",
                    __func__, ToLibImplName(elem.libImplInfo).c_str(), ToLibImplName(implInfo).c_str());
                misMatch = true;
                return nullptr;
            }
            return elem.hdiImpl.Construct();
        }
    }
    return nullptr;
}

void *HdiImplConstructorDelegate::SearchMatchedLibrary(const LibImplInfo& implInfo,
    const std::string &interfaceName)
{
    HdiImpl hdiImpl(interfaceName, ToLibImplName(implInfo));

    struct HdiImplConstructor hdiImplConstructor = {
        .libImplInfo = implInfo,
        .hdiImpl = hdiImpl
    };

    if (hdiImplConstructor.hdiImpl.Load() == HDF_SUCCESS) {
        hdiImplConstructorVect.push_back(std::move(hdiImplConstructor));
        void *impl = hdiImplConstructorVect.back().hdiImpl.Construct();
        if (impl == nullptr) {
            hdiImplConstructor.hdiImpl.Unload();
        }
        hdiImplConstructorVect.clear();
        return impl;
    }

    return SearchMatchedLibraryInDirs(implInfo, interfaceName);
}

void *HdiImplConstructorDelegate::SearchMatchedLibraryInDirs(const LibImplInfo& implInfo,
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

    // find matched library in the system
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        const char *path = paths[i];
        bool misMatch = false;
        void *impl = SearchMatchedLibraryInDir(implInfo, interfaceName, path, misMatch);
        if (impl != nullptr || misMatch) {
            return impl;
        }
    }
    return nullptr;
}

void *HdiImplConstructorDelegate::SearchMatchedLibraryInDir(const LibImplInfo& implInfo,
    const std::string &interfaceName, const char *directory, bool &misMatch)
{
    misMatch = false;
    struct dirent *entry;
    if (directory == nullptr) {
        return nullptr;
    }

    DIR *dir = opendir(directory);
    if (dir == nullptr) {
        HDF_LOGW("%{public}s: failed to open %{public}s, error: %{public}s",
            __func__, directory, strerror(errno));
        return nullptr;
    }
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || entry->d_type != DT_REG) {
            continue;
        }
        std::string fileName = entry->d_name;
        LibImplInfo fileImplInfo;
        if (RegexMatch(fileName, implInfo, fileImplInfo)) {
            HdiImpl hdiImplTmp(interfaceName, ToLibImplName(fileImplInfo));
            struct HdiImplConstructor hdiImplConstructorTmp = {
                .libImplInfo = fileImplInfo,
                .hdiImpl = hdiImplTmp
            };
            hdiImplConstructorVect.push_back(std::move(hdiImplConstructorTmp));

            closedir(dir);
            if (implInfo.minorVersion > hdiImplConstructorVect.back().libImplInfo.minorVersion) {
                HDF_LOGD("%{public}s, cache load %{public}s, but try load %{public}s.", __func__,
                    ToLibImplName(hdiImplConstructorVect.back().libImplInfo).c_str(), ToLibImplName(implInfo).c_str());
                misMatch = true;
                hdiImplConstructorVect.clear();
                return nullptr;
            }
            void *impl = hdiImplConstructorVect.back().hdiImpl.Construct();
            hdiImplConstructorVect.clear();
            return impl;
        }
    }
    closedir(dir);
    return nullptr;
}

bool HdiImplConstructorDelegate::RegexMatch(const std::string &fileName,
                                            const LibImplInfo &implInfo, LibImplInfo &fileImplInfo)
{
    constexpr uint32_t minorIndex = 1;
    std::ostringstream ss;
    ss << implInfo.libNameBase << "_" << implInfo.majorVersion << "\\.([1-9][0-9]*)" << implInfo.extension;
    std::regex re = std::regex(ss.str());
    std::smatch result;
    if (!std::regex_match(fileName, result, re)) {
        return false;
    }
    uint32_t minorVersion = stoul(result[minorIndex]);
    fileImplInfo = implInfo;
    fileImplInfo.minorVersion = minorVersion;
    return true;
}

static int32_t ParseInterface(
    const std::string &desc, std::string &interface, LibImplInfo &libName, const char *serviceName)
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

    libName = ParseLibName(TransFileName(interface), serviceName, versionMajor, versionMinor);
    return HDF_SUCCESS;
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
    LibImplInfo libName;
    if (ParseInterface(desc, interfaceName, libName, serviceName) != HDF_SUCCESS) {
        HDF_LOGE("failed to parse hdi interface info from '%{public}s'", desc);
        return nullptr;
    }

    HDF_LOGI("LoadHdiImpl desc: %{public}s, serviceName: %{public}s.", desc, serviceName);
    std::lock_guard<std::mutex> lock(g_loaderMutex);
    return HdiImplConstructorDelegate::GetInstance().Construct(libName, interfaceName);
}

void UnloadHdiImpl(const char *desc, const char *serviceName, void *impl)
{
    if (desc == nullptr || impl == nullptr) {
        return;
    }

    std::string interfaceName;
    LibImplInfo libName;
    if (ParseInterface(desc, interfaceName, libName, serviceName) != HDF_SUCCESS) {
        HDF_LOGE("%{public}s: failed to parse hdi interface info from '%{public}s'", __func__, desc);
        return;
    }
    std::lock_guard<std::mutex> lock(g_loaderMutex);
    return HdiImplConstructorDelegate::GetInstance().Destruct(libName, impl);
}
