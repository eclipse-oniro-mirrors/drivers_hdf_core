/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "codegen/code_emitter.h"

#include <cctype>

#include "util/file.h"
#include "util/options.h"

namespace OHOS {
namespace HDI {
bool CodeEmitter::OutPut(const AutoPtr<AST> &ast, const std::string &targetDirectory, bool isKernelCode)
{
    if (!Reset(ast, targetDirectory, isKernelCode)) {
        return false;
    }

    EmitCode();
    return true;
}

bool CodeEmitter::Reset(const AutoPtr<AST> &ast, const std::string &targetDirectory, bool isKernelCode)
{
    if (ast == nullptr || targetDirectory.empty()) {
        return false;
    }

    CleanData();

    isKernelCode_ = isKernelCode;
    ast_ = ast;
    if (ast_->GetASTFileType() == ASTFileType::AST_IFACE || ast_->GetASTFileType() == ASTFileType::AST_ICALLBACK) {
        interface_ = ast_->GetInterfaceDef();
        interfaceName_ = interface_->GetName();
        interfaceFullName_ = interface_->GetNamespace()->ToString() + interfaceName_;
        baseName_ = StringHelper::StartWith(interfaceName_, "I") ? interfaceName_.substr(1) : interfaceName_;
        proxyName_ = baseName_ + "Proxy";
        proxyFullName_ = interface_->GetNamespace()->ToString() + proxyName_;

        stubName_ = baseName_ + "Stub";
        stubFullName_ = interface_->GetNamespace()->ToString() + stubName_;

        implName_ = baseName_ + "Service";
        implFullName_ = interface_->GetNamespace()->ToString() + implName_;
    } else if (ast_->GetASTFileType() == ASTFileType::AST_TYPES) {
        baseName_ = ast_->GetName();
    } else if (ast_->GetASTFileType() == ASTFileType::AST_SEQUENCEABLE) {
        baseName_ = ast_->GetName();
    }

    majorVerName_ = StringHelper::Format("%s_MAJOR_VERSION", ConstantName(interfaceName_).c_str());
    minorVerName_ = StringHelper::Format("%s_MINOR_VERSION", ConstantName(interfaceName_).c_str());
    bufferSizeMacroName_ = StringHelper::Format("%s_BUFF_MAX_SIZE", ConstantName(baseName_).c_str());

    std::string prefix = StringHelper::Format("%c%s", tolower(baseName_[0]), baseName_.substr(1).c_str());
    dataParcelName_ = prefix + "Data";
    replyParcelName_ = prefix + "Reply";
    optionName_ = prefix + "Option";
    errorCodeName_ = prefix + "Ret";
    flagOfSetMemName_ = prefix + "MemSet";

    if (!ResolveDirectory(targetDirectory)) {
        return false;
    }

    return true;
}

void CodeEmitter::CleanData()
{
    isKernelCode_ = false;
    ast_ = nullptr;
    interface_ = nullptr;
    directory_ = "";
    interfaceName_ = "";
    interfaceFullName_ = "";
    baseName_ = "";
    proxyName_ = "";
    proxyFullName_ = "";
    stubName_ = "";
    stubFullName_ = "";
    implName_ = "";
    implFullName_ = "";
    dataParcelName_ = "";
    replyParcelName_ = "";
    optionName_ = "";
    errorCodeName_ = "";
}

bool CodeEmitter::NeedFlag(const AutoPtr<ASTMethod> &method)
{
    for (size_t i = 0; i < method->GetParameterNumber(); i++) {
        AutoPtr<ASTParameter> param = method->GetParameter(i);
        AutoPtr<ASTType> type = param->GetType();
        if (param->GetAttribute() == ParamAttr::PARAM_OUT &&
            (type->IsStringType() || type->IsArrayType() || type->IsListType())) {
            return true;
        }
    }
    return false;
}

/*
 * -r option: -r ohos.hdi:./drivers/interface
 * outDir: ./out
 * package: ohos.hdi.foo.v1_0
 * subPackage: foo.v1_0
 * subPath: foo/v1_0
 * GenPath: ./out/foo/v1_0/
 */
std::string CodeEmitter::GetFileParentPath(const std::string &outDir)
{
    std::string outPath = StringHelper::EndWith(outDir, File::separator) ? outDir.substr(0, outDir.size() - 1) : outDir;
    std::string subPackage = Options::GetInstance().GetSubPackage(ast_->GetPackageName());
    std::string subPath = StringHelper::Replace(subPackage, '.', File::separator);
    if (subPath.empty()) {
        return File::AdapterPath(StringHelper::Format("%s/", outPath.c_str(), subPath.c_str()));
    } else {
        return File::AdapterPath(StringHelper::Format("%s/%s/", outPath.c_str(), subPath.c_str()));
    }
}

std::string CodeEmitter::PackageToFilePath(const std::string &packageName)
{
    std::vector<std::string> packageVec = StringHelper::Split(Options::GetInstance().GetSubPackage(packageName), ".");
    StringBuilder filePath;
    for (auto iter = packageVec.begin(); iter != packageVec.end(); iter++) {
        filePath.Append(FileName(*iter));
        if (iter != packageVec.end() - 1) {
            filePath.Append(File::separator);
        }
    }

    return filePath.ToString();
}

std::string CodeEmitter::EmitMethodCmdID(const AutoPtr<ASTMethod> &method)
{
    return StringHelper::Format("CMD_%s_%s", ConstantName(baseName_).c_str(), ConstantName(method->GetName()).c_str());
}

void CodeEmitter::EmitInterfaceMethodCommands(StringBuilder &sb, const std::string &prefix)
{
    sb.Append(prefix).AppendFormat("enum {\n");
    for (size_t i = 0; i < interface_->GetMethodNumber(); i++) {
        AutoPtr<ASTMethod> method = interface_->GetMethod(i);
        sb.Append(prefix + TAB).Append(EmitMethodCmdID(method)).Append(",\n");
    }

    sb.Append(prefix + TAB).Append(EmitMethodCmdID(interface_->GetVersionMethod())).Append(",\n");
    sb.Append(prefix).Append("};\n");
}

std::string CodeEmitter::EmitVersionHeaderName(const std::string &name)
{
    return StringHelper::Format("v%u_%u/%s", ast_->GetMajorVer(), ast_->GetMinorVer(), FileName(name).c_str());
}

std::string CodeEmitter::ConstantName(const std::string &name)
{
    if (name.empty()) {
        return name;
    }

    StringBuilder sb;

    for (size_t i = 0; i < name.size(); i++) {
        char c = name[i];
        if (isupper(c) != 0) {
            if (i > 1) {
                sb.Append('_');
            }
            sb.Append(c);
        } else {
            sb.Append(toupper(c));
        }
    }

    return sb.ToString();
}

std::string CodeEmitter::PascalName(const std::string &name)
{
    if (name.empty()) {
        return name;
    }

    StringBuilder sb;
    for (size_t i = 0; i < name.size(); i++) {
        char c = name[i];
        if (i == 0) {
            if (islower(c)) {
                c = toupper(c);
            }
            sb.Append(c);
        } else {
            if (c == '_') {
                continue;
            }

            if (islower(c) && name[i - 1] == '_') {
                c = toupper(c);
            }
            sb.Append(c);
        }
    }

    return sb.ToString();
}

std::string CodeEmitter::FileName(const std::string &name)
{
    if (name.empty()) {
        return name;
    }

    StringBuilder sb;
    for (size_t i = 0; i < name.size(); i++) {
        char c = name[i];
        if (isupper(c) != 0) {
            // 2->Index of the last char array.
            if (i > 1) {
                sb.Append('_');
            }
            sb.Append(tolower(c));
        } else {
            sb.Append(c);
        }
    }

    return sb.ToString();
}

void CodeEmitter::GetUtilMethods(UtilMethodMap &methods)
{
    // get util methods
    (void)methods;
}

void CodeEmitter::EmitUtilMethods(
    StringBuilder &sb, const std::string &prefix, const UtilMethodMap &methods, bool isDecl)
{
    // generator util methods
    for (const auto &methodPair : methods) {
        if (!isDecl) {
            sb.Append("\n");
        }
        methodPair.second(sb, "", prefix, isDecl);
    }
}
} // namespace HDI
} // namespace OHOS
