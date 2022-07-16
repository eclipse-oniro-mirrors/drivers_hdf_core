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

#include "util/file.h"

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
            doCompile_ = true;
            sourceFiles_.push_back(optarg);
            break;
        case 'd':
            doOutDir_ = true;
            generationDirectory_ = optarg;
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
            doSetModuleName_ = true;
            moduleName_ = optarg;
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

void Options::AddPackagePath(const String &packagePath)
{
    int index = packagePath.IndexOf(":");
    if (index == -1 || index == packagePath.GetLength() - 1) {
        errors_.push_back(
            String::Format("%s: invalid option parameters '%s'.", program_.string(), packagePath.string()));
        return;
    }

    String package = packagePath.Substring(0, index);
    String path = File::RealPath(packagePath.Substring(index + 1));
    if (path.IsEmpty() || path.IsNull()) {
        errors_.push_back(
            String::Format("%s: invalid path '%s'.", program_.string(), packagePath.Substring(index + 1).string()));
        return;
    }

    auto it = packagePath_.find(package);
    if (it != packagePath_.end()) {
        errors_.push_back(
            String::Format("%s: The '%s:%s' has been set.", program_.string(), package.string(), path.string()));
    }

    packagePath_[package] = path;
}

void Options::SetLanguage(Language kind)
{
    doGenerateCode_ = true;
    targetLanguage_ = kind;
}

void Options::SetCodePart(const String &part)
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
            errors_.push_back(String::Format("%s: nothing to do.", program_.string()));
            return;
        }

        if (!doGenerateCode_ && doOutDir_) {
            errors_.push_back(String::Format("%s: no target language.", program_.string()));
            return;
        }

        if (doGenerateCode_ && !doOutDir_) {
            errors_.push_back(String::Format("%s: no out directory.", program_.string()));
            return;
        }

        if (doGeneratePart_ && !codePart_.Equals("all") && !codePart_.Equals("client") && !codePart_.Equals("server")) {
            String errorLog = "The '--build-target' option parameter must be 'client' 'server' or 'all'.";
            errors_.push_back(String::Format("%s: %s", program_.string(), errorLog.string()));
        }
    } else {
        if (doGetHashKey_ || doDumpAST_ || doGenerateCode_ || doOutDir_) {
            errors_.push_back(String::Format("%s: no '-c' option.", program_.string()));
            return;
        }
    }
}

void Options::ShowErrors() const
{
    for (auto error : errors_) {
        printf("%s\n", error.string());
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
           "  --module-name <module name>     Set driver module name\n"
           "  --build-target <target name>    Generate client code, server code or all code\n"
           "  -d <directory>                  Place generated codes into <directory>\n");
}

/*
 * For Example
 * -r option: -r ohos.hdi:./drivers/interface
 * package:ohos.hdi.foo.v1_0
 * rootPackage:ohos.hdi
 */
String Options::GetRootPackage(const String &package)
{
    const auto &packagePaths = GetPackagePathMap();
    for (const auto &packageRoot : packagePaths) {
        if (package.StartsWith(packageRoot.first)) {
            return packageRoot.first;
        }
    }

    return "";
}

/*
 * For Example
 * -r option: -r ohos.hdi:./drivers/interface
 * package:ohos.hdi.foo.v1_0
 * rootPath:./drivers/interface
 */
String Options::GetRootPath(const String &package)
{
    const auto &packagePaths = GetPackagePathMap();
    for (const auto &packageRoot : packagePaths) {
        if (package.StartsWith(packageRoot.first)) {
            return packageRoot.second;
        }
    }

    return "";
}

/*
 * For Example
 * -r option: -r ohos.hdi:./drivers/interface
 * package:ohos.hdi.foo.v1_0
 * subPackage:foo.v1_0
 */
String Options::GetSubPackage(const String &package)
{
    String rootPackage = GetRootPackage(package);
    if (rootPackage.IsEmpty()) {
        return package;
    }

    return package.Substring(rootPackage.GetLength() + 1);
}

/*
 * For Example
 * -r option: -r ohos.hdi:./drivers/interface
 * package:ohos.hdi.foo.v1_0
 * packagePath:./drivers/interface/foo/v1_0
 */
String Options::GetPackagePath(const String &package)
{
    String rootPackage = "";
    String rootPath = "";
    const auto &packagePaths = GetPackagePathMap();
    for (const auto &packageRoot : packagePaths) {
        if (package.StartsWith(packageRoot.first)) {
            rootPackage = packageRoot.first;
            rootPath = packageRoot.second;
        }
    }

    if (rootPackage.IsEmpty()) {
        // The current path is the root path
        String curPath = File::AdapterPath(package.Replace('.', File::separator));
        return File::RealPath(curPath);
    }

    if (rootPath.EndsWith(File::separator)) {
        rootPath = rootPath.Substring(0, rootPath.GetLength() - 1);
    }

    String subPath = package.Substring(rootPackage.GetLength() + 1).Replace('.', File::separator);
    return File::AdapterPath(rootPath + "/" + subPath);
}

/*
 * For Example
 * -r option: -r ohos.hdi:./drivers/interface
 * import: ohos.hdi.foo.v1_0.MyTypes
 * packagePath:./drivers/interface/foo/v1_0/MyTypes.idl
 */
String Options::GetImportFilePath(const String &import)
{
    int index = import.LastIndexOf('.');
    if (index <= 0) {
        return import;
    }

    String dir = GetPackagePath(import.Substring(0, index));
    String ClassName = import.Substring(index + 1);

    return String::Format("%s%c%s.idl", dir.string(), File::separator, ClassName.string());
}
} // namespace HDI
} // namespace OHOS