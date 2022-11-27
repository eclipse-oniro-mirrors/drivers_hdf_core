/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_ASTINTERFACETYPE_H
#define OHOS_HDI_ASTINTERFACETYPE_H

#include "ast/ast_attribute.h"
#include "ast/ast_method.h"

#include <vector>

#include "ast/ast_type.h"
#include "util/autoptr.h"

namespace OHOS {
namespace HDI {
class ASTInterfaceType : public ASTType {
public:
    ASTInterfaceType()
        : ASTType(TypeKind::TYPE_INTERFACE, false),
        license_(),
        attr_(new ASTInfAttr()),
        isSerializable_(false),
        methods_(),
        getVerMethod_()
    {
    }

    void SetNamespace(const AutoPtr<ASTNamespace> &nspace) override;

    inline void SetLicense(const std::string &license)
    {
        license_ = license;
    }

    inline std::string GetLicense() const
    {
        return license_;
    }

    void SetAttribute(const AutoPtr<ASTInfAttr> &attr)
    {
        if (attr != nullptr) {
            attr_ = attr;
            if (attr_->isCallback_) {
                isSerializable_ = true;
            }
        }
    }

    inline bool IsOneWay()
    {
        return attr_->isOneWay_;
    }

    inline bool IsCallback()
    {
        return attr_->isCallback_;
    }

    inline void SetSerializable(bool isSerializable)
    {
        isSerializable_ = isSerializable;
    }

    inline bool IsSerializable()
    {
        return isSerializable_;
    }

    inline bool IsFull()
    {
        return attr_->isFull_;
    }

    inline bool IsLite()
    {
        return attr_->isLite_;
    }

    void AddMethod(const AutoPtr<ASTMethod> &method);

    AutoPtr<ASTMethod> GetMethod(size_t index);

    inline size_t GetMethodNumber()
    {
        return methods_.size();
    }

    void AddVersionMethod(const AutoPtr<ASTMethod> &method)
    {
        getVerMethod_ = method;
    }

    AutoPtr<ASTMethod> GetVersionMethod()
    {
        return getVerMethod_;
    }

    bool IsInterfaceType() override;

    std::string ToString() const override;

    std::string Dump(const std::string &prefix) override;

    TypeKind GetTypeKind() override;

    std::string GetFullName() const;

    std::string EmitDescMacroName() const;

    std::string EmitCType(TypeMode mode = TypeMode::NO_MODE) const override;

    std::string EmitCppType(TypeMode mode = TypeMode::NO_MODE) const override;

    std::string EmitJavaType(TypeMode mode, bool isInnerType = false) const override;

    void EmitCWriteVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const override;

    void EmitCProxyReadVar(const std::string &parcelName, const std::string &name, bool isInnerType,
        const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitCStubReadVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const override;

    void EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, unsigned int innerLevel = 0) const override;

    void EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, bool initVariable, unsigned int innerLevel = 0) const override;

    void EmitJavaWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitJavaReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitJavaReadInnerVar(const std::string &parcelName, const std::string &name, bool isInner, StringBuilder &sb,
        const std::string &prefix) const override;

    void RegisterWriteMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const override;

    void RegisterReadMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const override;

    void EmitCWriteMethods(
        StringBuilder &sb, const std::string &prefix, const std::string &methodPrefix, bool isDecl) const;

    void EmitCReadMethods(
        StringBuilder &sb, const std::string &prefix, const std::string &methodPrefix, bool isDecl) const;

    void EmitCppReadMethods(
        StringBuilder &sb, const std::string &prefix, const std::string &methodPrefix, bool isDecl) const;
private:
    std::string license_;

    AutoPtr<ASTInfAttr> attr_;
    bool isSerializable_;
    std::vector<AutoPtr<ASTMethod>> methods_;
    AutoPtr<ASTMethod> getVerMethod_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_ASTINTERFACETYPE_H