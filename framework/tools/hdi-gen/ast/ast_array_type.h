/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_ASTARRAYTYPE_H
#define OHOS_HDI_ASTARRAYTYPE_H

#include "ast/ast_type.h"
#include "util/autoptr.h"

namespace OHOS {
namespace HDI {
class ASTArrayType : public ASTType {
public:
    ASTArrayType() : ASTType(TypeKind::TYPE_ARRAY, false), elementType_() {}

    inline void SetElementType(const AutoPtr<ASTType> &elementType)
    {
        elementType_ = elementType;
    }

    inline AutoPtr<ASTType> GetElementType()
    {
        return elementType_;
    }

    virtual bool IsArrayType();

    bool HasInnerType(TypeKind innerType) const override;

    virtual String ToString() const;

    virtual TypeKind GetTypeKind();

    String EmitCType(TypeMode mode = TypeMode::NO_MODE) const override;

    String EmitCppType(TypeMode mode = TypeMode::NO_MODE) const override;

    virtual String EmitJavaType(TypeMode mode, bool isInnerType = false) const;

    void EmitCWriteVar(const String &parcelName, const String &name, const String &ecName, const String &gotoLabel,
        StringBuilder &sb, const String &prefix) const override;

    void EmitCProxyWriteOutVar(const String &parcelName, const String &name, const String &ecName,
        const String &gotoLabel, StringBuilder &sb, const String &prefix) const override;

    void EmitCProxyReadVar(const String &parcelName, const String &name, bool isInnerType, const String &ecName,
        const String &gotoLabel, StringBuilder &sb, const String &prefix) const override;

    void EmitCStubReadVar(const String &parcelName, const String &name, const String &ecName, const String &gotoLabel,
        StringBuilder &sb, const String &prefix) const override;

    void EmitCStubReadOutVar(const String &buffSizeName, const String &memFlagName, const String &parcelName,
        const String &name, const String &ecName, const String &gotoLabel, StringBuilder &sb,
        const String &prefix) const override;

    void EmitCppWriteVar(const String &parcelName, const String &name, StringBuilder &sb, const String &prefix,
        unsigned int innerLevel = 0) const override;

    void EmitCppReadVar(const String &parcelName, const String &name, StringBuilder &sb, const String &prefix,
        bool initVariable, unsigned int innerLevel = 0) const override;

    void EmitCMarshalling(const String &name, StringBuilder &sb, const String &prefix) const override;

    void EmitCUnMarshalling(const String &name, const String &gotoLabel, StringBuilder &sb, const String &prefix,
        std::vector<String> &freeObjStatements) const override;

    void EmitCppMarshalling(const String &parcelName, const String &name, StringBuilder &sb, const String &prefix,
        unsigned int innerLevel = 0) const override;

    void EmitCppUnMarshalling(const String &parcelName, const String &name, StringBuilder &sb, const String &prefix,
        bool emitType, unsigned int innerLevel = 0) const override;

    void EmitMemoryRecycle(
        const String &name, bool isClient, bool ownership, StringBuilder &sb, const String &prefix) const override;

    virtual void EmitJavaWriteVar(
        const String &parcelName, const String &name, StringBuilder &sb, const String &prefix) const;

    virtual void EmitJavaReadVar(
        const String &parcelName, const String &name, StringBuilder &sb, const String &prefix) const;

    virtual void EmitJavaReadInnerVar(const String &parcelName, const String &name, bool isInner, StringBuilder &sb,
        const String &prefix) const;

    void RegisterWriteMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const override;

    void RegisterReadMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const override;

    void RegisterWritePodArrayMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const;

    void RegisterWriteStringArrayMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const;

    void RegisterReadPodArrayMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const;

    void RegisterReadStringArrayMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const;

    // c methods about reading and writing array with pod element

    void EmitCWriteMethods(StringBuilder &sb, const String& prefix, const String& methodPrefix, bool isDecl) const;

    void EmitCReadMethods(StringBuilder &sb, const String& prefix, const String& methodPrefix, bool isDecl) const;

    void EmitCStubReadMethods(StringBuilder &sb, const String& prefix, const String& methodPrefix, bool isDecl) const;

    void EmitCStubReadMethodBody(StringBuilder &sb, const String &prefix) const;

    // c methods about reading and writing string array

    void EmitCWriteStrArrayMethods(StringBuilder &sb, const String& prefix, const String& methodPrefix,
        bool isDecl) const;

    void EmitCReadStrArrayMethods(StringBuilder &sb, const String& prefix, const String& methodPrefix,
        bool isDecl) const;

    void EmitCReadStrArrayMethodBody(StringBuilder &sb, const String& prefix) const;

    void EmitCCheckParamOfReadStringArray(StringBuilder &sb, const String& prefix) const;

    void EmitCStubReadStrArrayMethods(StringBuilder &sb, const String& prefix, const String& methodPrefix,
        bool isDecl) const;

    void EmitCStubReadStrArrayMethodBody(StringBuilder &sb, const String& prefix) const;

    void EmitCStubReadStrArrayFree(StringBuilder &sb, const String& prefix) const;

    // cpp methods about reading and writing array with pod element

    void EmitCppWriteMethods(StringBuilder &sb, const String& prefix, const String& methodPrefix, bool isDecl) const;

    void EmitCppReadMethods(StringBuilder &sb, const String& prefix, const String& methodPrefix, bool isDecl) const;

protected:
    void EmitJavaWriteArrayVar(
        const String &parcelName, const String &name, StringBuilder &sb, const String &prefix) const;

    void EmitCMallocVar(const String &name, const String &lenName, bool isClient, const String &ecName,
        const String &gotoLabel, StringBuilder &sb, const String &prefix) const;

    void EmitCStringElementUnMarshalling(const String &name, const String &gotoLabel, StringBuilder &sb,
        const String &newPrefix, std::vector<String> &freeObjStatements) const;

    AutoPtr<ASTType> elementType_;
};

class ASTListType : public ASTArrayType {
public:
    ASTListType() : ASTArrayType()
    {
        typeKind_ = TypeKind::TYPE_LIST;
        isPod_ = false;
    }

    bool IsArrayType() override;

    bool IsListType() override;

    String ToString() const override;

    TypeKind GetTypeKind() override;

    String EmitJavaType(TypeMode mode, bool isInnerType = false) const override;

    void EmitJavaWriteVar(
        const String &parcelName, const String &name, StringBuilder &sb, const String &prefix) const override;

    void EmitJavaReadVar(
        const String &parcelName, const String &name, StringBuilder &sb, const String &prefix) const override;

    void EmitJavaReadInnerVar(const String &parcelName, const String &name, bool isInner, StringBuilder &sb,
        const String &prefix) const override;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_ASTARRAYTYPE_H