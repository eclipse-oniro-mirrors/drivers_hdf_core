/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_interface_type.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
void ASTInterfaceType::SetNamespace(const AutoPtr<ASTNamespace> &nspace)
{
    ASTType::SetNamespace(nspace);
    if (namespace_ != nullptr) {
        namespace_->AddInterface(this);
    }
}

void ASTInterfaceType::AddMethod(const AutoPtr<ASTMethod> &method)
{
    if (method == nullptr) {
        return;
    }
    methods_.push_back(method);
}

AutoPtr<ASTMethod> ASTInterfaceType::GetMethod(size_t index)
{
    if (index >= methods_.size()) {
        return nullptr;
    }

    return methods_[index];
}

bool ASTInterfaceType::IsInterfaceType()
{
    return true;
}

std::string ASTInterfaceType::ToString() const
{
    return name_;
}

std::string ASTInterfaceType::Dump(const std::string &prefix)
{
    StringBuilder sb;

    sb.Append(prefix);
    sb.Append(prefix).Append(attr_->Dump(prefix)).Append(" ");
    sb.AppendFormat("interface %s {\n", name_.c_str());
    for (auto method : methods_) {
        std::string info = method->Dump(prefix + "  ");
        sb.Append(info);
        if (method != methods_[methods_.size() - 1]) {
            sb.Append('\n');
        }
    }
    sb.Append(prefix).Append("}\n");

    return sb.ToString();
}

TypeKind ASTInterfaceType::GetTypeKind()
{
    return TypeKind::TYPE_INTERFACE;
}

std::string ASTInterfaceType::GetFullName() const
{
    return namespace_->ToString() + name_;
}

std::string ASTInterfaceType::EmitCType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return StringHelper::Format("struct %s", name_.c_str());
        case TypeMode::PARAM_IN:
            return StringHelper::Format("struct %s*", name_.c_str());
        case TypeMode::PARAM_OUT:
            return StringHelper::Format("struct %s**", name_.c_str());
        case TypeMode::LOCAL_VAR:
            return StringHelper::Format("struct %s*", name_.c_str());
        default:
            return "unknow type";
    }
}

std::string ASTInterfaceType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return StringHelper::Format("sptr<%s>", name_.c_str());
        case TypeMode::PARAM_IN:
            return StringHelper::Format("const sptr<%s>&", name_.c_str());
        case TypeMode::PARAM_OUT:
            return StringHelper::Format("sptr<%s>&", name_.c_str());
        case TypeMode::LOCAL_VAR:
            return StringHelper::Format("sptr<%s>", name_.c_str());
        default:
            return "unknow type";
    }
}

std::string ASTInterfaceType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    return name_;
}

void ASTInterfaceType::EmitCWriteVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (HdfSbufWriteRemoteService(%s, %s->AsObject(%s)) != 0) {\n", parcelName.c_str(),
        name.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTInterfaceType::EmitCProxyReadVar(const std::string &parcelName, const std::string &name, bool isInnerType,
    const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    std::string remoteName = StringHelper::Format("%sRemote", name.c_str());
    std::string baseName = StringHelper::StartWith(name_, "I") ? name_.substr(1) : name_;

    sb.Append(prefix).AppendFormat(
        "struct HdfRemoteService *%s = HdfSbufReadRemoteService(%s);\n", remoteName.c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", remoteName.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", remoteName.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");

    sb.Append(prefix).AppendFormat("*%s = %sGet(%s);\n", name.c_str(), baseName.c_str(), remoteName.c_str());
}

void ASTInterfaceType::EmitCStubReadVar(const std::string &parcelName, const std::string &name,
    const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    std::string remoteName = StringHelper::Format("%sRemote", name.c_str());
    std::string baseName = StringHelper::StartWith(name_, "I") ? name_.substr(1) : name_;

    sb.Append(prefix).AppendFormat(
        "struct HdfRemoteService *%s = HdfSbufReadRemoteService(%s);\n", remoteName.c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", remoteName.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", remoteName.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
    sb.Append(prefix).AppendFormat("%s = %sGet(%s);\n", name.c_str(), baseName.c_str(), remoteName.c_str());
}

void ASTInterfaceType::EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteRemoteObject(", parcelName.c_str());
    sb.AppendFormat("OHOS::HDI::ObjectCollector::GetInstance().GetOrNewObject(%s, %s::GetDescriptor()))) {\n",
        name.c_str(), name_.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTInterfaceType::EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool initVariable, unsigned int innerLevel) const
{
    if (initVariable) {
        sb.Append(prefix).AppendFormat("sptr<%s> %s = hdi_facecast<%s>(%s.ReadRemoteObject());\n", name_.c_str(),
            name.c_str(), name_.c_str(), parcelName.c_str());
    } else {
        sb.Append(prefix).AppendFormat(
            "%s = hdi_facecast<%s>(%s.ReadRemoteObject());\n", name.c_str(), name_.c_str(), parcelName.c_str());
    }
}

void ASTInterfaceType::EmitJavaWriteVar(
    const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("%s.writeRemoteObject(%s.asObject());\n", parcelName.c_str(), name.c_str());
}

void ASTInterfaceType::EmitJavaReadVar(
    const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    std::string stubName = StringHelper::StartWith(name_, "I") ? (name_.substr(1) + "Stub") : (name_ + "Stub");
    sb.Append(prefix).AppendFormat(
        "%s = %s.asInterface(%s.readRemoteObject());\n", name.c_str(), stubName.c_str(), parcelName.c_str());
}

void ASTInterfaceType::EmitJavaReadInnerVar(const std::string &parcelName, const std::string &name, bool isInner,
    StringBuilder &sb, const std::string &prefix) const
{
    std::string stubName = StringHelper::StartWith(name_, "I") ? (name_.substr(1) + "Stub") : (name_ + "Stub");
    sb.Append(prefix).AppendFormat("%s %s = %s.asInterface(%s.readRemoteObject());\n",
        EmitJavaType(TypeMode::NO_MODE).c_str(), name.c_str(), stubName.c_str(), parcelName.c_str());
}
} // namespace HDI
} // namespace OHOS