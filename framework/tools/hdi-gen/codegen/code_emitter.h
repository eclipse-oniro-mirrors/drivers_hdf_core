/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_CODE_EMITTER_H
#define OHOS_HDI_CODE_EMITTER_H

#include <set>

#include "ast/ast.h"
#include "util/autoptr.h"
#include "util/light_refcount_base.h"

namespace OHOS {
namespace HDI {
enum class HeaderFileType {
    OWN_HEADER_FILE,
    SYSTEM_HEADER_FILE,
    C_STD_HEADER_FILE,
    CPP_STD_HEADER_FILE,
    OTHER_MODULES_HEADER_FILE,
    OWN_MODULE_HEADER_FILE,
};

struct HeaderFile {
    HeaderFile(HeaderFileType type, std::string fileName) : type_(type), fileName_(fileName) {}

    struct compare {
        bool operator()(const HeaderFile &lhs, const HeaderFile &rhs)
        {
            int compareRet = lhs.fileName_.compare(rhs.fileName_);
            if (compareRet == 0) {
                return false;
            }

            if (lhs.type_ != rhs.type_) {
                return lhs.type_ < rhs.type_;
            }

            return compareRet < 0;
        }
    };

    std::string ToString() const
    {
        switch (type_) {
            case HeaderFileType::OWN_HEADER_FILE:
            case HeaderFileType::OWN_MODULE_HEADER_FILE:
                return StringHelper::Format("#include \"%s.h\"", fileName_.c_str());
            case HeaderFileType::SYSTEM_HEADER_FILE:
            case HeaderFileType::C_STD_HEADER_FILE:
            case HeaderFileType::OTHER_MODULES_HEADER_FILE:
                return StringHelper::Format("#include <%s.h>", fileName_.c_str());
            case HeaderFileType::CPP_STD_HEADER_FILE:
                return StringHelper::Format("#include <%s>", fileName_.c_str());
            default:
                return StringHelper::Format("//");
        }
    }

    using HeaderFileSet = std::set<HeaderFile, HeaderFile::compare>;

    HeaderFileType type_;
    std::string fileName_;
};

class CodeEmitter : public LightRefCountBase {
public:
    virtual ~CodeEmitter() = default;

    bool OutPut(const AutoPtr<AST> &ast, const std::string &targetDirectory, bool isKernelCode = false);

protected:
    bool Reset(const AutoPtr<AST> &ast, const std::string &targetDirectory, bool isKernelCode);

    void CleanData();

    virtual bool ResolveDirectory(const std::string &targetDirectory) = 0;

    virtual void EmitCode() = 0;

    bool NeedFlag(const AutoPtr<ASTMethod> &method);

    std::string GetFileParentPath(const std::string &outDir);

    std::string PackageToFilePath(const std::string &packageName);

    std::string EmitMethodCmdID(const AutoPtr<ASTMethod> &method);

    void EmitInterfaceMethodCommands(StringBuilder &sb, const std::string &prefix);

    /* add version prefix
     * MajorVersion: 1
     * MinorVersion: 0
     * name: IFoo
     * result: v1_0/ifoo.h
     */
    std::string EmitVersionHeaderName(const std::string &name);

    // log tag macro of hdf
    void EmitLogTagMacro(StringBuilder &sb, const std::string &name);

    // file_name -> FILE_NAME
    std::string ConstantName(const std::string &name);

    // file_name -> FileName
    std::string PascalName(const std::string &name);

    // FileName -> file_name
    std::string FileName(const std::string &name);

    virtual void GetUtilMethods(UtilMethodMap &methods);

    virtual void EmitUtilMethods(
        StringBuilder &sb, const std::string &prefix, const UtilMethodMap &methods, bool isDecl);

    void EmitInterfaceBuffSizeMacro(StringBuilder &sb);

protected:
    bool isKernelCode_ = false;
    AutoPtr<AST> ast_ = nullptr;
    AutoPtr<ASTInterfaceType> interface_ = nullptr;
    std::string directory_;

    std::string interfaceName_;
    std::string interfaceFullName_;
    std::string baseName_;
    std::string proxyName_;
    std::string proxyFullName_;
    std::string stubName_;
    std::string stubFullName_;
    std::string implName_;
    std::string implFullName_;
    std::string majorVerName_;
    std::string minorVerName_;

    std::string dataParcelName_;
    std::string replyParcelName_;
    std::string optionName_;
    std::string errorCodeName_;
    std::string flagOfSetMemName_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_CODE_EMITTER_H
