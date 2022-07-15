/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_AST_SMQ_H
#define OHOS_HDI_AST_SMQ_H

#include "ast/ast_type.h"

namespace OHOS {
namespace HDI {
class ASTSmqType : public ASTType {
public:
    ASTSmqType() : ASTType(TypeKind::TYPE_SMQ, false), innerType_() {}

    inline void SetInnerType(const AutoPtr<ASTType> &innerType)
    {
        innerType_ = innerType;
    }

    bool IsSmqType() override;

    bool HasInnerType(TypeKind innerType) const override;

    String ToString() const override;

    TypeKind GetTypeKind() override;

    String EmitCppType(TypeMode mode = TypeMode::NO_MODE) const override;

    void EmitCppWriteVar(const String &parcelName, const String &name, StringBuilder &sb, const String &prefix,
        unsigned int innerLevel = 0) const override;

    void EmitCppReadVar(const String &parcelName, const String &name, StringBuilder &sb, const String &prefix,
        bool initVariable, unsigned int innerLevel = 0) const override;
private:
    AutoPtr<ASTType> innerType_;
};

class ASTAshmemType : public ASTType {
public:
    ASTAshmemType() : ASTType(TypeKind::TYPE_ASHMEM, false) {}

    bool IsAshmemType() override;

    String ToString() const override;

    TypeKind GetTypeKind() override;

    String EmitCppType(TypeMode mode = TypeMode::NO_MODE) const override;

    void EmitCppWriteVar(const String& parcelName, const String& name, StringBuilder& sb,
        const String& prefix, unsigned int innerLevel = 0) const override;

    void EmitCppReadVar(const String& parcelName, const String& name, StringBuilder& sb,
        const String& prefix, bool initVariable, unsigned int innerLevel = 0) const override;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_AST_SMQ_H