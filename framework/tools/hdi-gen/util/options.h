/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_OPTION_H
#define OHOS_HDI_OPTION_H

#include <string>
#include <unordered_map>
#include <vector>

namespace OHOS {
namespace HDI {
class Options {
public:
    using PkgPathMap = std::unordered_map<std::string, std::string>;

    enum class Language {
        C,
        CPP,
        JAVA,
    };

    static Options &GetInstance();

    Options(const Options &other) = delete;
    Options operator=(const Options &other) = delete;

    Options &Parse(int argc, char *argv[]);

    ~Options() = default;

    inline bool DoShowUsage() const
    {
        return doShowUsage_;
    }

    inline bool DoShowVersion() const
    {
        return doShowVersion_;
    }

    inline bool DoCompile() const
    {
        return doCompile_;
    }

    inline bool DoDumpAST() const
    {
        return doDumpAST_;
    }

    inline bool DoGetHashKey() const
    {
        return doGetHashKey_;
    }

    inline bool DoGenerateCode() const
    {
        return doGenerateCode_;
    }

    inline bool DoGenerateKernelCode() const
    {
        return doModeKernel_;
    }

    inline bool DoPassthrough() const
    {
        return doPassthrough_;
    }

    inline bool HasErrors() const
    {
        return !errors_.empty();
    }

    inline std::vector<std::string> GetSourceFiles() const
    {
        return sourceFiles_;
    }

    inline PkgPathMap GetPackagePathMap() const
    {
        return packagePath_;
    }

    inline Language GetTargetLanguage() const
    {
        return targetLanguage_;
    }

    inline std::string GetCodePart() const
    {
        return codePart_;
    }

    inline std::string GetModuleName() const
    {
        return doSetModuleName_ ? moduleName_ : "sample";
    }

    inline std::string GetGenerationDirectory() const
    {
        return generationDirectory_;
    }

    void ShowErrors() const;

    void ShowVersion() const;

    void ShowUsage() const;

    std::string GetRootPackage(const std::string &package);

    std::string GetRootPath(const std::string &package);

    std::string GetSubPackage(const std::string &package);

    std::string GetPackagePath(const std::string &package);

    std::string GetImportFilePath(const std::string &import);

private:
    Options()
        : program_(),
        sourceFiles_(0),
        targetLanguage_(Language::C),
        codePart_("all"),
        generationDirectory_(),
        illegalOptions_(),
        errors_(),
        doShowUsage_(false),
        doShowVersion_(false),
        doCompile_(false),
        doDumpAST_(false),
        doGetHashKey_(false),
        doGenerateCode_(false),
        doModeKernel_(false),
        doGeneratePart_(false),
        doSetModuleName_(false),
        doOutDir_(false),
        doPassthrough_(false)
    {
    }

    void SetOptionData(char op);

    void AddPackagePath(const std::string &packagePath);

    void AddSources(const std::string &sourceFile);

    void SetOutDir(const std::string &dir);

    void SetModuleName(const std::string &moduleName);

    void SetLanguage(Language language);

    void SetCodePart(const std::string &part);

    void CheckOptions();

    static const char *optSupportArgs;
    static constexpr int OPT_END = -1;

    static constexpr int VERSION_MAJOR = 0;
    static constexpr int VERSION_MINOR = 1;

    std::string program_;
    std::vector<std::string> sourceFiles_;
    Language targetLanguage_;
    std::string codePart_;
    std::string moduleName_;
    std::string generationDirectory_;
    std::string illegalOptions_;
    std::vector<std::string> errors_;
    PkgPathMap packagePath_;

    bool doShowUsage_;
    bool doShowVersion_;
    bool doCompile_;
    bool doDumpAST_;
    bool doGetHashKey_;
    bool doGenerateCode_;
    bool doModeKernel_;
    bool doGeneratePart_;
    bool doSetModuleName_;
    bool doOutDir_;
    bool doPassthrough_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDIL_OPTION_H