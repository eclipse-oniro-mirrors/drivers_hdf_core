/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "util/options.h"

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <getopt.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util/common.h"
#include "util/file.h"
#include "util/string_helper.h"

#include "util/logger.h"

namespace OHOS {
namespace HDI {
const char *Options::optSupportArgs = "c:d:r:";
static struct option g_longOpts[] = {
    {"help",         no_argument,       nullptr, 'h'},
    {"version",      no_argument,       nullptr, 'v'},
    {"gen-c",        no_argument,       nullptr, 'C'},
    {"gen-cpp",      no_argument,       nullptr, 'P'},
    {"gen-java",     no_argument,       nullptr, 'J'},
    {"gen-hash",     no_argument,       nullptr, 'H'},
    {"build-target", required_argument, nullptr, 'p'},
    {"module-name",  required_argument, nullptr, 'N'},
    {"passthrough",  no_argument,       nullptr, 'T'},
    {"kernel",       no_argument,       nullptr, 'K'},
    {"dump-ast",     no_argument,       nullptr, 'D'},
    {nullptr,        0,                 nullptr, 0  }
};

Options &Options::GetInstance()
{
    static Options option;
    return option;
}

Options &Options::Parse(int argc, char *argv[])
{
    program_ = argv[0];
    opterr = 1;
    int op = 0;
    int optIndex = 0;

    while ((op = getopt_long(argc, argv, optSupportArgs, g_longOpts, &optIndex)) != OPT_END) {
        SetOptionData(op);
    }
    CheckOptions();

    return *this;
}

void Options::SetOptionData(char op)
{
    switch (op) {
        case 'c':
            AddSources(optarg);
            break;
        case 'd':
            SetOutDir(optarg);
            break;
        case 'h':
            doShowUsage_ = true;
            break;
        case 'v':
            doShowVersion_ = true;
            break;
        case 'r':
            AddPackagePath(optarg);
            break;
        case 'K':
            doModeKernel_ = true;
            break;
        case 'N':
            SetModuleName(optarg);
            break;
        case 'C':
            SetLanguage(Language::C);
            break;
        case 'P':
            SetLanguage(Language::CPP);
            break;
        case 'J':
            SetLanguage(Language::JAVA);
            break;
        case 'p':
            SetCodePart(optarg);
            break;
        case 'T':
            doPassthrough_ = true;
            break;
        case 'H':
            doGetHashKey_ = true;
            break;
        case 'D':
            doDumpAST_ = true;
            break;
        case '?':
        default:
            doShowUsage_ = true;
            break;
    }
}

void Options::AddPackagePath(const std::string &packagePath)
{
    size_t index = packagePath.find(":");
    if (index == std::string::npos || index == packagePath.size() - 1) {
        errors_.push_back(
            StringHelper::Format("%s: invalid option parameters '%s'.", program_.c_str(), packagePath.c_str()));
        return;
    }

    std::string package = packagePath.substr(0, index);
    std::string path = File::AdapterRealPath(packagePath.substr(index + 1));
    if (path.empty()) {
        errors_.push_back(
            StringHelper::Format("%s: invalid path '%s'.", program_.c_str(), packagePath.substr(index + 1).c_str()));
        return;
    }

    auto it = packagePath_.find(package);
    if (it != packagePath_.end()) {
        errors_.push_back(
            StringHelper::Format("%s: The '%s:%s' has been set.", program_.c_str(), package.c_str(), path.c_str()));
    }

    packagePath_[package] = path;
}

void Options::AddSources(const std::string &sourceFile)
{
    doCompile_ = true;
    sourceFiles_.push_back(sourceFile);
}

void Options::SetOutDir(const std::string &dir)
{
    doOutDir_ = true;
    generationDirectory_ = dir;
}

void Options::SetModuleName(const std::string &moduleName)
{
    doSetModuleName_ = true;
    moduleName_ = moduleName;
}

void Options::SetLanguage(Language kind)
{
    doGenerateCode_ = true;
    targetLanguage_ = kind;
}

void Options::SetCodePart(const std::string &part)
{
    // The default parameter is 'all', and the optional parameters is 'client' or 'server'
    doGeneratePart_ = true;
    codePart_ = part;
}

void Options::CheckOptions()
{
    if (doShowUsage_ || doShowVersion_) {
        return;
    }

    if (doCompile_) {
        if (!doGetHashKey_ && !doDumpAST_ && !doGenerateCode_ && !doOutDir_) {
            errors_.push_back(StringHelper::Format("%s: nothing to do.", program_.c_str()));
            return;
        }

        if (!doGenerateCode_ && doOutDir_) {
            errors_.push_back(StringHelper::Format("%s: no target language.", program_.c_str()));
            return;
        }

        if (doGenerateCode_ && !doOutDir_) {
            errors_.push_back(StringHelper::Format("%s: no out directory.", program_.c_str()));
            return;
        }

        if (doGeneratePart_ && codePart_ != "all" && codePart_ != "client" && codePart_ != "server") {
            std::string errorLog = "The '--build-target' option parameter must be 'client' 'server' or 'all'.";
            errors_.push_back(StringHelper::Format("%s: %s", program_.c_str(), errorLog.c_str()));
        }
    } else {
        if (doGetHashKey_ || doDumpAST_ || doGenerateCode_ || doOutDir_) {
            errors_.push_back(StringHelper::Format("%s: no '-c' option.", program_.c_str()));
            return;
        }
    }
}

void Options::ShowErrors() const
{
    for (auto error : errors_) {
        printf("%s\n", error.c_str());
    }
    printf("Use \"--help\" to show usage.\n");
}

void Options::ShowVersion() const
{
    printf("HDI-GEN: %d.%d\n"
           "Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.\n\n",
        VERSION_MAJOR, VERSION_MINOR);
}

void Options::ShowUsage() const
{
    printf("Compile a .idl file and generate C/C++ and Java codes.\n"
           "Usage: idl [options] file\n"
           "Options:\n"
           "  --help                          Display command line options\n"
           "  --version                       Display toolchain version information\n"
           "  --dump-ast                      Display the AST of the compiled file\n"
           "  -r <rootPackage>:<rootPath>     set root path of root package\n"
           "  -c <*.idl>                      Compile the .idl file\n"
           "  --gen-hash                      Generate hash key of the idl file\n"
           "  --gen-c                         Generate C code\n"
           "  --gen-cpp                       Generate C++ code\n"
           "  --gen-java                      Generate Java code\n"
           "  --kernel                        Generate kernel-mode ioservice stub code,"
           "default user-mode ioservice stub code\n"
           "  --passthrough                   Generate code that only supports pass through mode"
           "  --module-name <module name>     Set driver module name\n"
           "  --build-target <target name>    Generate client code, server code or all code\n"
           "  -d <directory>                  Place generated codes into <directory>\n");
}

/*
 * -r option: -r ohos.hdi:./drivers/interface
 * package:ohos.hdi.foo.v1_0
 * rootPackage:ohos.hdi
 */
std::string Options::GetRootPackage(const std::string &package)
{
    const auto &packagePaths = GetPackagePathMap();
    for (const auto &packageRoot : packagePaths) {
        if (StringHelper::StartWith(package, packageRoot.first)) {
            return packageRoot.first;
        }
    }

    return "";
}

/*
 * -r option: -r ohos.hdi:./drivers/interface
 * package:ohos.hdi.foo.v1_0
 * rootPath:./drivers/interface
 */
std::string Options::GetRootPath(const std::string &package)
{
    const auto &packagePaths = GetPackagePathMap();
    for (const auto &packageRoot : packagePaths) {
        if (StringHelper::StartWith(package, packageRoot.first)) {
            return packageRoot.second;
        }
    }

    return "";
}

/*
 * -r option: -r ohos.hdi:./drivers/interface
 * package:ohos.hdi.foo.v1_0
 * subPackage:foo.v1_0
 */
std::string Options::GetSubPackage(const std::string &package)
{
    std::string rootPackage = GetRootPackage(package);
    if (rootPackage.empty()) {
        return package;
    }

    return package.substr(rootPackage.size() + 1);
}

/*
 * -r option: -r ohos.hdi:./drivers/interface
 * package:ohos.hdi.foo.v1_0
 * packagePath:./drivers/interface/foo/v1_0
 */
std::string Options::GetPackagePath(const std::string &package)
{
    std::string rootPackage = "";
    std::string rootPath = "";
    const auto &packagePaths = GetPackagePathMap();
    for (const auto &packageRoot : packagePaths) {
        if (StringHelper::StartWith(package, packageRoot.first)) {
            rootPackage = packageRoot.first;
            rootPath = packageRoot.second;
        }
    }

    if (rootPackage.empty()) {
        // The current path is the root path
        std::string curPath = File::AdapterPath(StringHelper::Replace(package, '.', SEPARATOR));
        return File::AdapterRealPath(curPath);
    }

    if (StringHelper::EndWith(rootPath, SEPARATOR)) {
        rootPath.pop_back();
    }

    std::string subPath = StringHelper::Replace(package.substr(rootPackage.size() + 1), '.', SEPARATOR);
    return File::AdapterPath(rootPath + "/" + subPath);
}

/*
 * -r option: -r ohos.hdi:./drivers/interface
 * import: ohos.hdi.foo.v1_0.MyTypes
 * packagePath:./drivers/interface/foo/v1_0/MyTypes.idl
 */
std::string Options::GetImportFilePath(const std::string &import)
{
    size_t index = import.rfind('.');
    if (index == std::string::npos) {
        return import;
    }

    std::string dir = GetPackagePath(StringHelper::SubStr(import, 0, index));
    std::string ClassName = import.substr(index + 1);
    return StringHelper::Format("%s%c%s.idl", dir.c_str(), SEPARATOR, ClassName.c_str());
}
} // namespace HDI
} // namespace OHOS