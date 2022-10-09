/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "codegen/code_generator.h"
#include "parser/parser.h"
#include "preprocessor/preprocessor.h"
#include "util/file.h"
#include "util/logger.h"
#include "util/options.h"

using namespace OHOS::HDI;

static bool GetHashKey()
{
    for (const auto &sourceFile : Options::GetInstance().GetSourceFiles()) {
        std::unique_ptr<File> idlFile = std::make_unique<File>(sourceFile, int(File::READ));
        if (!idlFile->IsValid()) {
            Logger::E("hdi-gen", "failed to open idl file");
            return false;
        }
        printf("%s:%lu\n", idlFile->GetPath().c_str(), idlFile->GetHashKey());
    }
    return true;
}
int main(int argc, char **argv)
{
    const Options &options = Options::GetInstance().Parse(argc, argv);
    if (options.HasErrors()) {
        options.ShowErrors();
        return 0;
    }
    if (options.DoShowUsage()) {
        options.ShowUsage();
        return 0;
    }
    if (options.DoShowVersion()) {
        options.ShowVersion();
        return 0;
    }
    if (!options.DoCompile()) {
        return 0;
    }
    if (options.DoGetHashKey()) {
        return GetHashKey() ? 0 : -1;
    }

    Preprocessor preprocessor;
    std::vector<std::string> sourceFiles;
    if (!preprocessor.Preprocess(sourceFiles)) {
        Logger::E("MAIN", "failed to preprocess");
        return -1;
    }

    Parser parser;
    if (!parser.Parse(sourceFiles)) {
        Logger::E("MAIN", "failed to parse file");
        return -1;
    }

    if (options.DoDumpAST()) {
        for (const auto &astPair : parser.GetAllAst()) {
            printf("%s\n", astPair.second->Dump("").c_str());
        }
    }

    if (!options.DoGenerateCode()) {
        return 0;
    }
    if (!CodeGenerator(parser.GetAllAst()).Generate()) {
        Logger::E("hdi-gen", "failed to generate code");
        return -1;
    }
    return 0;
}