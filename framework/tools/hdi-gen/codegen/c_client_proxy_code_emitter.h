/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_C_CLIENT_PROXY_CODE_EMITTER_H
#define OHOS_HDI_C_CLIENT_PROXY_CODE_EMITTER_H

#include "codegen/c_code_emitter.h"

namespace OHOS {
namespace HDI {
class CClientProxyCodeEmitter : public CCodeEmitter {
public:
    CClientProxyCodeEmitter() : CCodeEmitter() {}

    virtual ~CClientProxyCodeEmitter() = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitPassthroughProxySourceFile();

    void EmitPassthroughProxyInclusions(StringBuilder &sb);

    void EmitPassthroughGetInstanceMethod(StringBuilder &sb);

    void EmitPassthroughReleaseInstanceMethod(StringBuilder &sb);

    void EmitProxySourceFile();

    void EmitProxyDefinition(StringBuilder &sb);

    void EmitProxyInclusions(StringBuilder &sb);

    void GetHeaderOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles);

    void EmitProxyCallMethodImpl(StringBuilder &sb);

    void EmitProxyKernelCallMethodImpl(StringBuilder &sb);

    void EmitProxyMethodImpls(StringBuilder &sb);

    void EmitProxyMethodImpl(const AutoPtr<ASTMethod> &method, StringBuilder &sb);

    void EmitProxyMethodBody(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix);

    void EmitCreateBuf(
        const std::string &dataBufName, const std::string &replyBufName, StringBuilder &sb, const std::string &prefix);

    void EmitCheckThisPointer(StringBuilder &sb, const std::string &prefix);

    void EmitWriteInterfaceToken(const std::string &dataBufName, StringBuilder &sb, const std::string &prefix);

    void EmitWriteFlagOfNeedSetMem(
        const AutoPtr<ASTMethod> &method, const std::string &dataBufName, StringBuilder &sb, const std::string &prefix);

    void EmitReleaseBuf(
        const std::string &dataBufName, const std::string &replyBufName, StringBuilder &sb, const std::string &prefix);

    void EmitReadProxyMethodParameter(const AutoPtr<ASTParameter> &param, const std::string &parcelName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix);

    void EmitStubCallMethod(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix);

    void EmitProxyAsObjectMethodImpl(StringBuilder &sb);

    void EmitProxyConstruction(StringBuilder &);

    void EmitProxyExternalMethodImpl(StringBuilder &sb);

	// the get method for getting kernel driver client object
    void EmitKernelProxyGetMethodImpl(StringBuilder &sb);

    void EmitKernelProxyGetInstanceMethodImpl(const std::string &objName, const std::string &serMajorName,
        const std::string &serMinorName, const std::string &remoteName, const std::string &serviceName,
        StringBuilder &sb);

    // the release metod for releasing kernel driver client object
    void EmitKernelProxyReleaseMethodImpl(const std::string &remoteName, const std::string &recycleFuncName,
        StringBuilder &sb);

    // the get method for getting interface object
    void EmitIfaceProxyGetMethodImpl(const std::string &objName, const std::string &serMajorName,
        const std::string &serMinorName, const std::string &remoteName, StringBuilder &sb);

    // the release metod for releasing interface obj
    void EmitIfaceProxyReleaseMethodImpl(
        const std::string &remoteName, const std::string &recycleFuncName, StringBuilder &sb);

    void EmitCbProxyReleaseMethodImpl(
        const std::string &remoteName, const std::string &recycleFuncName, StringBuilder &sb);

    // the get method for getting driver client object
    void EmitProxyGetMethodImpl(StringBuilder &sb);

    void EmitProxyGetInstanceMethodImpl(const std::string &objName, const std::string &serMajorName,
        const std::string &serMinorName, const std::string &remoteName, const std::string &serviceName,
        StringBuilder &sb);

    void EmitProxyGetRemoteService(
        const std::string &remoteName, const std::string &serviceName, StringBuilder &sb, const std::string &prefix);

    void EmitProxySetInterfaceDesc(const std::string &remoteName, const std::string &recycleFuncName,
        StringBuilder &sb, const std::string &prefix);

    void EmitProxyCreateProxyObject(const std::string &clientObjName, const std::string &remoteName,
        const std::string &recycleFuncName, StringBuilder &sb, const std::string &prefix);

    void EmitProxyCheckVersion(const std::string &clientObjName, const std::string &serMajorName,
        const std::string &serMinorName, StringBuilder &sb, const std::string &prefix);

    // the release metod for releasing driver client obj
    void EmitProxyReleaseMethodImpl(StringBuilder &sb);

    void EmitProxyReleaseInstanceMethodImpl(const std::string &serviceName, const std::string &remoteName,
        const std::string &recycleFuncName, StringBuilder &sb);

    void EmitProxyLoadOrUnLoadHdiImpl(const std::string &serviceName, bool isLoad, StringBuilder &sb,
        const std::string &prefix);

    void GetUtilMethods(UtilMethodMap &methods) override;

    std::vector<std::string> freeObjStatements_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_C_CLIENT_PROXY_CODE_EMITTER_H