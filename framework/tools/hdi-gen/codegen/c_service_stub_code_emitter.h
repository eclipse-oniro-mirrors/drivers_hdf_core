/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_C_SERVICE_STUB_CODEE_MITTER_H
#define OHOS_HDI_C_SERVICE_STUB_CODEE_MITTER_H

#include "codegen/c_code_emitter.h"

namespace OHOS {
namespace HDI {
class CServiceStubCodeEmitter : public CCodeEmitter {
public:
    CServiceStubCodeEmitter() : CCodeEmitter() {}

    virtual ~CServiceStubCodeEmitter() = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitServiceStubHeaderFile();

    void EmitStubHeaderInclusions(StringBuilder &sb);

    void EmitCbServiceStubDef(StringBuilder &sb);

    void EmitCbServiceStubMethodsDcl(StringBuilder &sb);

    void EmitServiceStubSourceFile();

    void EmitStubSourceInclusions(StringBuilder &sb);

    void GetSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles);

    // get or release method for driver interface object
    void EmitExternalMethodImpl(StringBuilder &sb);

    void EmitGetMethodImpl(StringBuilder &sb);

    void EmitGetInstanceMehtodImpl(StringBuilder &sb);

    void EmitReleaseMethodImpl(StringBuilder &sb);

    void EmitReleaseInstanceMethodImpl(StringBuilder &sb);

    void EmitServiceStubMethodImpls(StringBuilder &sb, const std::string &prefix);

    void EmitServiceStubMethodImpl(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix);

    void EmitReadFlagVariable(bool readFlag, StringBuilder &sb, const std::string &prefix);

    void EmitStubLocalVariable(const AutoPtr<ASTParameter> &param, StringBuilder &sb, const std::string &prefix);

    void EmitReadStubMethodParameter(const AutoPtr<ASTParameter> &param, const std::string &parcelName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix);

    void EmitReadCStringStubMethodParameter(const AutoPtr<ASTParameter> &param, const std::string &parcelName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix, AutoPtr<ASTType> &type);

    void EmitOutVarMemInitialize(const AutoPtr<ASTParameter> &param, const std::string &parcelName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix);

    void EmitStubCallMethod(
        const AutoPtr<ASTMethod> &method, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix);

    void EmitCallParameter(
        StringBuilder &sb, const AutoPtr<ASTType> &type, ParamAttr attribute, const std::string &name);

    void EmitStubGetVerMethodImpl(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix);

    void EmitStubAsObjectMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitKernelStubOnRequestMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitKernelStubConstruct(StringBuilder &sb);

    void EmitStubOnRequestMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitStubRemoteDispatcher(StringBuilder &sb);

    void EmitStubNewInstance(StringBuilder &sb);

    void EmitStubReleaseMethod(StringBuilder &sb);

    void EmitStubConstructor(StringBuilder &sb);

    void EmitStubRegAndUnreg(StringBuilder &sb);

    void GetUtilMethods(UtilMethodMap &methods) override;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_C_SERVICE_STUB_CODEE_MITTER_H