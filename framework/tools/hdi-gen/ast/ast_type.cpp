/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_type.h"

namespace OHOS {
namespace HDI {
void ASTType::SetName(const std::string &name)
{
    name_ = name;
}

std::string ASTType::GetName()
{
    return name_;
}

void ASTType::SetNamespace(const AutoPtr<ASTNamespace> &nspace)
{
    namespace_ = nspace;
}

AutoPtr<ASTNamespace> ASTType::GetNamespace()
{
    return namespace_;
}

bool ASTType::IsBooleanType()
{
    return false;
}

bool ASTType::IsByteType()
{
    return false;
}

bool ASTType::IsShortType()
{
    return false;
}

bool ASTType::IsIntegerType()
{
    return false;
}

bool ASTType::IsLongType()
{
    return false;
}

bool ASTType::IsUcharType()
{
    return false;
}

bool ASTType::IsUshortType()
{
    return false;
}

bool ASTType::IsUintType()
{
    return false;
}

bool ASTType::IsUlongType()
{
    return false;
}

bool ASTType::IsFloatType()
{
    return false;
}

bool ASTType::IsDoubleType()
{
    return false;
}

bool ASTType::IsStringType()
{
    return false;
}

bool ASTType::IsListType()
{
    return false;
}

bool ASTType::IsMapType()
{
    return false;
}

bool ASTType::IsEnumType()
{
    return false;
}

bool ASTType::IsStructType()
{
    return false;
}

bool ASTType::IsUnionType()
{
    return false;
}

bool ASTType::IsInterfaceType()
{
    return false;
}

bool ASTType::IsSequenceableType()
{
    return false;
}

bool ASTType::IsVoidType()
{
    return false;
}

bool ASTType::IsArrayType()
{
    return false;
}

bool ASTType::IsFdType()
{
    return false;
}

bool ASTType::IsSmqType()
{
    return false;
}

bool ASTType::IsAshmemType()
{
    return false;
}

bool ASTType::IsBufferHandleType()
{
    return false;
}

bool ASTType::IsPod() const
{
    return isPod_;
}

bool ASTType::HasInnerType(TypeKind innerType) const
{
    return false;
}

std::string ASTType::ToShortString()
{
    return name_;
}

std::string ASTType::ToString() const
{
    return (namespace_ == nullptr) ? name_ : (namespace_->ToString() + name_);
}

TypeKind ASTType::GetTypeKind()
{
    return TypeKind::TYPE_UNKNOWN;
}

std::string ASTType::EmitCType(TypeMode mode) const
{
    return "unknow";
}

std::string ASTType::EmitCppType(TypeMode mode) const
{
    return "unknow";
}

std::string ASTType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    return "unknow";
}

void ASTType::EmitCWriteVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("//Writing \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::EmitCProxyWriteOutVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("//Writing \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::EmitCProxyReadVar(const std::string &parcelName, const std::string &name, bool isInnerType,
    const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("//Reading \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::EmitCStubReadVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("//Reading \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::EmitCStubReadOutVar(const std::string &buffSizeName, const std::string &memFlagName,
    const std::string &parcelName, const std::string &name, const std::string &ecName, const std::string &gotoLabel,
    StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("//Reading \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("//Writing \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool initVariable, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("//Reading \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::EmitCMarshalling(const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("//Writing \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::EmitCUnMarshalling(const std::string &name, const std::string &gotoLabel, StringBuilder &sb,
    const std::string &prefix, std::vector<std::string> &freeObjStatements) const
{
    sb.Append(prefix).AppendFormat("//Reading \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::EmitFreeStatements(
    const std::vector<std::string> &freeObjStatements, StringBuilder &sb, const std::string &prefix) const
{
    for (auto it = freeObjStatements.rbegin(); it != freeObjStatements.rend(); it++) {
        sb.Append(prefix).Append(*it);
    }
}

void ASTType::EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("//Writing \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::EmitCppUnMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool emitType, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("//Reading \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::EmitMemoryRecycle(
    const std::string &name, bool isClient, bool ownership, StringBuilder &sb, const std::string &prefix) const
{
    // only std::string, Array, List, struct and union type need recycle memory
    (void)name;
    (void)isClient;
    (void)ownership;
    (void)sb;
    (void)prefix;
}

void ASTType::EmitJavaWriteVar(
    const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("//Writing \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::EmitJavaReadVar(
    const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("//Reading \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::EmitJavaReadInnerVar(const std::string &parcelName, const std::string &name, bool isInner,
    StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("//Reading \"%s\" type of data is not supported\n", name_.c_str());
}

void ASTType::RegisterWriteMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const
{
    // register methods that generate write util methods
    (void)language;
    (void)methods;
}

void ASTType::RegisterReadMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const
{
    // register methods that generate read util methods
    (void)language;
    (void)methods;
}
} // namespace HDI
} // namespace OHOS