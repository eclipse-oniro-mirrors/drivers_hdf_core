/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_CPP_CLIENT_PROXY_CODE_EMITTER_H
#define OHOS_HDI_CPP_CLIENT_PROXY_CODE_EMITTER_H

#include "codegen/cpp_code_emitter.h"

namespace OHOS {
namespace HDI {
class CppClientProxyCodeEmitter : public CppCodeEmitter {
public:
    CppClientProxyCodeEmitter() : CppCodeEmitter() {}

    virtual ~CppClientProxyCodeEmitter() = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitProxyHeaderFile();

    void EmitProxyHeaderInclusions(StringBuilder &sb);

    void GetHeaderOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles);

    void EmitProxyDecl(StringBuilder &sb, const std::string &prefix);

    void EmitProxyConstructor(StringBuilder &sb, const std::string &prefix);

    void EmitProxyMethodDecls(StringBuilder &sb, const std::string &prefix);

    void EmitProxyMethodDecl(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix);

    void EmitProxyConstants(StringBuilder &sb, const std::string &prefix);

    void EmitProxyMethodParameter(const AutoPtr<ASTParameter> &param, StringBuilder &sb, const std::string &prefix);

    void EmitPassthroughProxySourceFile();

    void EmitPassthroughProxySourceInclusions(StringBuilder &sb);

    void EmitPassthroughGetInstanceMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitProxySourceFile();

    void EmitProxySourceInclusions(StringBuilder &sb);

    void GetSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles);

    void EmitGetMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitGetInstanceMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitProxyPassthroughtLoadImpl(StringBuilder &sb, const std::string &prefix);

    void EmitProxyMethodImpls(StringBuilder &sb, const std::string &prefix);

    void EmitProxyMethodImpl(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix);

    void EmitProxyMethodBody(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix);

    void EmitWriteInterfaceToken(const std::string &parcelName, StringBuilder &sb, const std::string &prefix);

    void EmitWriteFlagOfNeedSetMem(
        const AutoPtr<ASTMethod> &method, const std::string &dataBufName, StringBuilder &sb, const std::string &prefix);

    void GetUtilMethods(UtilMethodMap &methods) override;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_CPP_CLIENT_PROXY_CODE_EMITTER_H