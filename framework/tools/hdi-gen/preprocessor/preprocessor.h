/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_PREPROCESSOR_H
#define OHOS_HDI_PREPROCESSOR_H

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "lexer/lexer.h"
#include "util/string_helper.h"

namespace OHOS {
namespace HDI {
class FileDetail {
public:
    inline std::string GetFullName() const
    {
        return StringHelper::Format("%s.%s", packageName_.c_str(), fileName_.c_str());
    }

    std::string Dump() const;

public:
    std::string filePath_;
    std::string fileName_;
    std::string packageName_;
    std::unordered_set<std::string> imports_;
};

class Preprocessor {
public:
    using FileDetailMap = std::unordered_map<std::string, FileDetail>;

    // analyze idl files and return sorted ids files
    bool Preprocess(std::vector<std::string> &compileSourceFiles);

private:
    bool CheckAllFilesPath(const std::vector<std::string> &sourceFiles);

    bool AnalyseImportInfo(const std::vector<std::string> &sourceFiles, FileDetailMap &allFileDetails);

    bool ParseFileDetail(const std::string &sourceFile, FileDetail &info);

    bool ParsePackage(Lexer &lexer, FileDetail &info);

    bool ParseImports(Lexer &lexer, FileDetail &info);

    bool LoadOtherIdlFiles(const FileDetail &ownerFileDetail, FileDetailMap &allFileDetails);

    bool CheckCircularReference(FileDetailMap &allFileDetails, std::vector<std::string> &compileSourceFiles);

    void PrintCyclefInfo(FileDetailMap &allFileDetails);

    void FindCycle(const std::string &curNode, FileDetailMap &allFiles, std::vector<std::string> &trace);
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_PREPROCESSOR_H