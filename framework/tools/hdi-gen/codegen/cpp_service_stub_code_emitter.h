/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_CPP_SERVICE_STUB_CODE_EMITTER_H
#define OHOS_HDI_CPP_SERVICE_STUB_CODE_EMITTER_H

#include "codegen/cpp_code_emitter.h"

namespace OHOS {
namespace HDI {
class CppServiceStubCodeEmitter : public CppCodeEmitter {
public:
    CppServiceStubCodeEmitter() : CppCodeEmitter() {}

    virtual ~CppServiceStubCodeEmitter() = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    // ISample.idl -> sample_service_stub.h
    void EmitStubHeaderFile();

    void EmitStubHeaderInclusions(StringBuilder &sb);

    void GetHeaderOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles);

    void EmitStubUsingNamespace(StringBuilder &sb);

    void EmitStubDecl(StringBuilder &sb);

    void EmitStubBody(StringBuilder &sb, const std::string &prefix);

    void EmitStubConstructorDecl(StringBuilder &sb, const std::string &prefix);

    void EmitStubOnRequestDecl(StringBuilder &sb, const std::string &prefix);

    void EmitStubMethodDecls(StringBuilder &sb, const std::string &prefix);

    void EmitStubMethodDecl(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix);

    void EmitStubPrivateData(StringBuilder &sb, const std::string &prefix);

    // ISample.idl -> sample_service_stub.cpp
    void EmitStubSourceFile();

    void EmitStubSourceInclusions(StringBuilder &sb);

    void GetSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles);

    void EmitInterfaceGetMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitGetMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitGetInstanceMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitStubConstructorImpl(StringBuilder &sb, const std::string &prefix);

    void EmitStubOnRequestMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitStubMethodImpls(StringBuilder &sb, const std::string &prefix);

    void EmitStubMethodImpl(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix);

    void EmitStubCallMethod(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix);

    void EmitStubReadInterfaceToken(const std::string &parcelName, StringBuilder &sb, const std::string &prefix);

    void EmitStubReadMemFlag(
        const AutoPtr<ASTMethod> &method, const std::string &parcelName, StringBuilder &sb, const std::string &prefix);

    void EmitLocalVariable(const AutoPtr<ASTParameter> &param, const std::string &parcelName, StringBuilder &sb,
        const std::string &prefix);

    void GetUtilMethods(UtilMethodMap &methods) override;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_CPP_SERVICE_STUB_CODE_EMITTER_H