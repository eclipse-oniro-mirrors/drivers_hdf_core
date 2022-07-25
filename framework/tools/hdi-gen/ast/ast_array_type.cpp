/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_array_type.h"
#include "util/options.h"

namespace OHOS {
namespace HDI {
bool ASTArrayType::IsArrayType()
{
    return true;
}

bool ASTArrayType::HasInnerType(TypeKind innerTypeKind) const
{
    if (elementType_ == nullptr) {
        return false;
    }

    if (elementType_->GetTypeKind() == innerTypeKind) {
        return true;
    }
    return elementType_->HasInnerType(innerTypeKind);
}

String ASTArrayType::ToString() const
{
    return String::Format("%s[]", elementType_->ToString().string());
}

TypeKind ASTArrayType::GetTypeKind()
{
    return TypeKind::TYPE_ARRAY;
}

String ASTArrayType::EmitCType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return String::Format("%s*", elementType_->EmitCType(TypeMode::NO_MODE).string());
        case TypeMode::PARAM_IN: {
            if (elementType_->GetTypeKind() == TypeKind::TYPE_STRING) {
                return String::Format("%s*", elementType_->EmitCType(TypeMode::NO_MODE).string());
            }
            return String::Format("const %s*", elementType_->EmitCType(TypeMode::NO_MODE).string());
        }
        case TypeMode::PARAM_OUT:
            return String::Format("%s*", elementType_->EmitCType(TypeMode::NO_MODE).string());
        case TypeMode::LOCAL_VAR:
            return String::Format("%s*", elementType_->EmitCType(TypeMode::NO_MODE).string());
        default:
            return "unknow type";
    }
}

String ASTArrayType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return String::Format("std::vector<%s>", elementType_->EmitCppType(TypeMode::NO_MODE).string());
        case TypeMode::PARAM_IN:
            return String::Format("const std::vector<%s>&", elementType_->EmitCppType(TypeMode::NO_MODE).string());
        case TypeMode::PARAM_OUT:
            return String::Format("std::vector<%s>&", elementType_->EmitCppType(TypeMode::NO_MODE).string());
        case TypeMode::LOCAL_VAR:
            return String::Format("std::vector<%s>", elementType_->EmitCppType(TypeMode::NO_MODE).string());
        default:
            return "unknow type";
    }
}

String ASTArrayType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    return String::Format("%s[]", elementType_->EmitJavaType(TypeMode::NO_MODE, false).string());
}

void ASTArrayType::EmitCWriteVar(const String &parcelName, const String &name, const String &ecName,
    const String &gotoLabel, StringBuilder &sb, const String &prefix) const
{
    String lenName = String::Format("%sLen", name.string());
    if (!elementType_->IsBooleanType() && elementType_->IsPod()) {
        sb.Append(prefix).AppendFormat("if (!WritePodArray(%s, %s, sizeof(%s), %s)) {\n", parcelName.string(),
            name.string(), elementType_->EmitCType().string(), lenName.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
        sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(prefix).Append("}\n");
        return;
    } else if (elementType_->IsStringType()) {
        sb.Append(prefix).AppendFormat(
            "if (!WriteStringArray(%s, %s, %s)) {\n", parcelName.string(), name.string(), lenName.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
        sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(prefix).Append("}\n");
        return;
    }

    sb.Append(prefix).AppendFormat("if (%s == NULL || %s == 0) {\n", name.string(), lenName.string());
    sb.Append(prefix + TAB).AppendFormat("if (!HdfSbufWriteUint32(%s, 0)) {\n", parcelName.string());
    sb.Append(prefix + TAB + TAB)
        .AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + TAB + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix).Append("} else {\n");
    sb.Append(prefix + TAB).AppendFormat("if (!HdfSbufWriteUint32(%s, %s)) {\n", parcelName.string(), lenName.string());
    sb.Append(prefix + TAB + TAB)
        .AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + TAB + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix + TAB).Append("}\n");

    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix + TAB).AppendFormat("for (i = 0; i < %s; i++) {\n", lenName.string());
    } else {
        sb.Append(prefix + TAB).AppendFormat("for (uint32_t i = 0; i < %s; i++) {\n", lenName.string());
    }

    String elementName = "";
    if (elementType_->GetTypeKind() == TypeKind::TYPE_STRUCT || elementType_->GetTypeKind() == TypeKind::TYPE_UNION) {
        elementName = String::Format("&%s[i]", name.string());
    } else {
        elementName = String::Format("%s[i]", name.string());
    }
    elementType_->EmitCWriteVar(parcelName, elementName, ecName, gotoLabel, sb, prefix + TAB + TAB);
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCProxyWriteOutVar(const String &parcelName, const String &name, const String &ecName,
    const String &gotoLabel, StringBuilder &sb, const String &prefix) const
{
    String lenName = String::Format("*%sLen", name.string());
    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteUint32(%s, %s)) {\n", parcelName.string(), lenName.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCProxyReadVar(const String &parcelName, const String &name, bool isInnerType,
    const String &ecName, const String &gotoLabel, StringBuilder &sb, const String &prefix) const
{
    String lenName = String::Format("%sLen", name.string());
    if (!elementType_->IsBooleanType() && elementType_->IsPod()) {
        sb.Append(prefix).AppendFormat("if (!ReadPodArray(%s, %s, sizeof(%s), %s)) {\n", parcelName.string(),
            name.string(), elementType_->EmitCType().string(), lenName.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to read %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
        sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(prefix).Append("}\n");
        return;
    } else if (elementType_->IsStringType()) {
        sb.Append(prefix).AppendFormat(
            "if (!ReadStringArray(%s, %s, %s)) {\n", parcelName.string(), name.string(), lenName.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to read %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
        sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(prefix).Append("}\n");
        return;
    }

    sb.Append(prefix).AppendFormat("if (!HdfSbufReadUint32(%s, %s)) {\n", parcelName.string(), lenName.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s size failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n\n");

    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix).AppendFormat("for (i = 0; i < *%s; i++) {\n", lenName.string());
    } else {
        sb.Append(prefix).AppendFormat("for (uint32_t i = 0; i < *%s; i++) {\n", lenName.string());
    }

    if (elementType_->GetTypeKind() == TypeKind::TYPE_STRUCT) {
        String element = String::Format("&%s[i]", name.string());
        elementType_->EmitCProxyReadVar(parcelName, element, true, ecName, gotoLabel, sb, prefix + TAB);
    } else if (elementType_->GetTypeKind() == TypeKind::TYPE_FILEDESCRIPTOR) {
        String element = String::Format("%s[i]", name.string());
        elementType_->EmitCProxyReadVar(parcelName, element, true, ecName, gotoLabel, sb, prefix + TAB);
    } else {
        String element = String::Format("&%s[i]", name.string());
        elementType_->EmitCProxyReadVar(parcelName, element, true, ecName, gotoLabel, sb, prefix + TAB);
    }
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCStubReadVar(const String &parcelName, const String &name, const String &ecName,
    const String &gotoLabel, StringBuilder &sb, const String &prefix) const
{
    String lenName = String::Format("%sLen", name.string());
    if (!elementType_->IsBooleanType() && elementType_->IsPod()) {
        sb.Append(prefix).AppendFormat("if (!ReadPodArray(%s, (void **)&%s, sizeof(%s), &%s)) {\n", parcelName.string(),
            name.string(), elementType_->EmitCType().string(), lenName.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to read %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
        sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(prefix).Append("}\n");
        return;
    } else if (elementType_->IsStringType()) {
        sb.Append(prefix).AppendFormat(
            "if (!ReadStringArray(%s, &%s, &%s)) {\n", parcelName.string(), name.string(), lenName.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to read %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
        sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(prefix).Append("}\n");
        return;
    }

    sb.Append(prefix).AppendFormat("if (!HdfSbufReadUint32(%s, &%s)) {\n", parcelName.string(), lenName.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s size failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n\n");

    sb.Append(prefix).AppendFormat("if (%s > 0) {\n", lenName.string());
    EmitCMallocVar(name, lenName, false, ecName, gotoLabel, sb, prefix + TAB);
    sb.Append("\n");

    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix + TAB).AppendFormat("for (i = 0; i < %s; i++) {\n", lenName.string());
    } else {
        sb.Append(prefix + TAB).AppendFormat("for (uint32_t i = 0; i < %s; i++) {\n", lenName.string());
    }
    if (elementType_->GetTypeKind() == TypeKind::TYPE_STRUCT) {
        String element = String::Format("&%s[i]", name.string());
        elementType_->EmitCStubReadVar(parcelName, element, ecName, gotoLabel, sb, prefix + TAB + TAB);
    } else if (elementType_->GetTypeKind() == TypeKind::TYPE_FILEDESCRIPTOR) {
        String element = String::Format("%s[i]", name.string());
        elementType_->EmitCStubReadVar(parcelName, element, ecName, gotoLabel, sb, prefix + TAB + TAB);
    } else {
        String element = String::Format("&%s[i]", name.string());
        elementType_->EmitCStubReadVar(parcelName, element, ecName, gotoLabel, sb, prefix + TAB + TAB);
    }
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCStubReadOutVar(const String &buffSizeName, const String &memFlagName, const String &parcelName,
    const String &name, const String &ecName, const String &gotoLabel, StringBuilder &sb, const String &prefix) const
{
    String lenName = String::Format("%sLen", name.string());

    sb.Append(prefix).AppendFormat("if (%s) {\n", memFlagName.string());
    sb.Append(prefix + TAB).AppendFormat("if (!HdfSbufReadUint32(%s, &%s)) {\n", parcelName.string(), lenName.string());
    sb.Append(prefix + TAB + TAB)
        .AppendFormat("HDF_LOGE(\"%%{public}s: read %s size failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + TAB + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).AppendFormat("if (%s > 0) {\n", lenName.string());
    EmitCMallocVar(name, lenName, false, ecName, gotoLabel, sb, prefix + TAB + TAB);
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix).Append("} else {\n");
    sb.Append(prefix + TAB).AppendFormat("%s = (%s*)OsalMemCalloc(%s);\n", name.string(),
        elementType_->EmitCType().string(), buffSizeName.string());
    sb.Append(prefix + TAB).AppendFormat("if (%s == NULL) {\n", name.string());
    sb.Append(prefix + TAB + TAB)
        .AppendFormat("HDF_LOGE(\"%%{public}s: malloc %s failed\", __func__);\n", name.string());
    sb.Append(prefix + TAB + TAB).AppendFormat("%s = HDF_ERR_MALLOC_FAIL;\n", ecName.string());
    sb.Append(prefix + TAB + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).AppendFormat("%sLen = (%s / sizeof(%s));\n", name.string(), buffSizeName.string(),
        elementType_->EmitCType().string());
    sb.Append(prefix).Append("}\n\n");
}

void ASTArrayType::EmitCppWriteVar(const String &parcelName, const String &name, StringBuilder &sb,
    const String &prefix, unsigned int innerLevel) const
{
    if (!elementType_->IsBooleanType() && elementType_->IsPod()) {
        sb.Append(prefix).AppendFormat("if (!WritePodArray(%s, %s)) {\n", parcelName.string(), name.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
        sb.Append(prefix).Append("}\n");
        return;
    }

    sb.Append(prefix).AppendFormat("if (!%s.WriteUint32(%s.size())) {\n", parcelName.string(), name.string());
    sb.Append(prefix + TAB)
        .AppendFormat("HDF_LOGE(\"%%{public}s: write %s size failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");

    if (!elementType_->IsBooleanType() && elementType_->IsPod()) {
        sb.Append(prefix).AppendFormat("if (!%s.empty()) {\n", name.string());
        sb.Append(prefix + TAB).AppendFormat("if (!%s.WriteUnpadBuffer(", parcelName.string());
        sb.AppendFormat("(const void*)%s.data(), sizeof(%s) * %s.size())) {\n", name.string(),
            elementType_->EmitCppType().string(), name.string());
        sb.Append(prefix + TAB + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
        sb.Append(prefix + TAB).Append("}\n");
        sb.Append(prefix).Append("}\n");
    } else {
        String elementName = String::Format("it%d", innerLevel++);
        sb.Append(prefix).AppendFormat("for (const auto& %s : %s) {\n", elementName.string(), name.string());
        elementType_->EmitCppWriteVar(parcelName, elementName, sb, prefix + TAB, innerLevel);
        sb.Append(prefix).Append("}\n");
    }
}

void ASTArrayType::EmitCppReadVar(const String &parcelName, const String &name, StringBuilder &sb, const String &prefix,
    bool initVariable, unsigned int innerLevel) const
{
    if (initVariable) {
        sb.Append(prefix).AppendFormat("%s %s;\n", EmitCppType().string(), name.string());
    }

    if (!elementType_->IsBooleanType() && elementType_->IsPod()) {
        sb.Append(prefix).AppendFormat("if (!ReadPodArray(%s, %s)) {\n", parcelName.string(), name.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to read %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
        sb.Append(prefix).Append("}\n");
        return;
    }

    sb.Append(prefix).AppendFormat("uint32_t %sSize = %s.ReadUint32();\n", name.string(), parcelName.string());
    sb.Append(prefix).AppendFormat("%s.clear();\n", name.string());
    sb.Append(prefix).AppendFormat("%s.reserve(%sSize);\n", name.string(), name.string());
    sb.Append(prefix).AppendFormat(
        "for (uint32_t i%d = 0; i%d < %sSize; ++i%d) {\n", innerLevel, innerLevel, name.string(), innerLevel);
    String valueName = String::Format("value%d", innerLevel++);
    elementType_->EmitCppReadVar(parcelName, valueName, sb, prefix + TAB, true, innerLevel);
    sb.Append(prefix + TAB).AppendFormat("%s.push_back(%s);\n", name.string(), valueName.string());
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCMarshalling(const String &name, StringBuilder &sb, const String &prefix) const
{
    String lenName = String::Format("%sLen", name.string());
    if (!elementType_->IsBooleanType() && elementType_->IsPod()) {
        sb.Append(prefix).AppendFormat("if (!WritePodArray(data, %s, sizeof(%s), %s)) {\n", name.string(),
            elementType_->EmitCType().string(), lenName.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).Append("return false;\n");
        sb.Append(prefix).Append("}\n");
        return;
    } else if (elementType_->IsStringType()) {
        sb.Append(prefix).AppendFormat("if (!WriteStringArray(data, %s, %s)) {\n", name.string(), lenName.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).Append("return false;\n");
        sb.Append(prefix).Append("}\n");
        return;
    }

    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteUint32(data, %s)) {\n", lenName.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", lenName.string());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");

    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix).AppendFormat("for (i = 0; i < %s; i++) {\n", lenName.string());
    } else {
        sb.Append(prefix).AppendFormat("for (uint32_t i = 0; i < %s; i++) {\n", lenName.string());
    }

    String elementName = String::Format("(%s)[i]", name.string());
    elementType_->EmitCMarshalling(elementName, sb, prefix + TAB);
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCUnMarshalling(const String &name, const String &gotoLabel, StringBuilder &sb,
    const String &prefix, std::vector<String> &freeObjStatements) const
{
    String lenName = String::Format("%sLen", name.string());
    if (!elementType_->IsBooleanType() && elementType_->IsPod()) {
        sb.Append(prefix).AppendFormat("if (!ReadPodArray(data, (void**)&%s, sizeof(%s), &%s)) {\n", name.string(),
            elementType_->EmitCType().string(), lenName.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to read %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(prefix).Append("}\n");
        return;
    } else if (elementType_->IsStringType()) {
        sb.Append(prefix).AppendFormat("if (!ReadStringArray(data, &%s, &%s)) {\n", name.string(), lenName.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to read %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(prefix).Append("}\n");
        return;
    }

    sb.Append(prefix).AppendFormat("if (!HdfSbufReadUint32(data, &%s)) {\n", lenName.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", lenName.string());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");
    sb.Append(prefix).AppendFormat("if (%s > 0) {\n", lenName.string());
    String newPrefix = prefix + TAB;

    sb.Append(newPrefix).AppendFormat("%s = (%s*)OsalMemCalloc(sizeof(%s) * %s);\n", name.string(),
        elementType_->EmitCType().string(), elementType_->EmitCType().string(), lenName.string());
    sb.Append(newPrefix).AppendFormat("if (%s == NULL) {\n", name.string());
    sb.Append(newPrefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(newPrefix).Append("}\n");
    freeObjStatements.push_back(String::Format("OsalMemFree(%s);\n", name.string()));

    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(newPrefix).AppendFormat("for (i = 0; i < %s; i++) {\n", lenName.string());
    } else {
        sb.Append(newPrefix).AppendFormat("for (uint32_t i = 0; i < %s; i++) {\n", lenName.string());
    }

    if (elementType_->GetTypeKind() == TypeKind::TYPE_STRING) {
        EmitCStringElementUnMarshalling(name, gotoLabel, sb, newPrefix + TAB, freeObjStatements);
    } else if (elementType_->GetTypeKind() == TypeKind::TYPE_STRUCT) {
        String element = String::Format("&%s[i]", name.string());
        elementType_->EmitCUnMarshalling(element, gotoLabel, sb, newPrefix + TAB, freeObjStatements);
    } else {
        String element = String::Format("%s[i]", name.string());
        elementType_->EmitCUnMarshalling(element, gotoLabel, sb, newPrefix + TAB, freeObjStatements);
    }
    sb.Append(newPrefix).Append("}\n");
    sb.Append(prefix).Append("}\n");
    freeObjStatements.pop_back();
}

void ASTArrayType::EmitCStringElementUnMarshalling(const String &name, const String &gotoLabel, StringBuilder &sb,
    const String &newPrefix, std::vector<String> &freeObjStatements) const
{
    String element = String::Format("%sElement", name.string());
    elementType_->EmitCUnMarshalling(element, gotoLabel, sb, newPrefix, freeObjStatements);
    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(newPrefix).AppendFormat(
            "%s[i] = (char*)OsalMemCalloc(strlen(%s) + 1);\n", name.string(), element.string());
        sb.Append(newPrefix).AppendFormat("if (%s[i] == NULL) {\n", name.string());
        sb.Append(newPrefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(newPrefix).Append("}\n\n");
        sb.Append(newPrefix).AppendFormat("if (strcpy_s((%s)[i], (strlen(%s) + 1), %s) != EOK) {\n", name.string(),
            element.string(), element.string());
        sb.Append(newPrefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", element.string());
        sb.Append(newPrefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(newPrefix).Append("}\n");
    } else {
        sb.Append(newPrefix).Append(TAB).AppendFormat("%s[i] = strdup(%s);\n", name.string(), element.string());
    }
}

void ASTArrayType::EmitCppMarshalling(const String &parcelName, const String &name, StringBuilder &sb,
    const String &prefix, unsigned int innerLevel) const
{
    if (!elementType_->IsBooleanType() && elementType_->IsPod()) {
        sb.Append(prefix).AppendFormat("if (!WritePodArray(%s, %s)) {\n", parcelName.string(), name.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).Append("return false;\n");
        sb.Append(prefix).Append("}\n");
        return;
    }

    sb.Append(prefix).AppendFormat("if (!%s.WriteUint32(%s.size())) {\n", parcelName.string(), name.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: failed write %s.size\", __func__);\n", name.string());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
    String elementName = String::Format("it%d", innerLevel++);
    sb.Append(prefix).AppendFormat("for (const auto& %s : %s) {\n", elementName.string(), name.string());
    elementType_->EmitCppMarshalling(parcelName, elementName, sb, prefix + TAB, innerLevel);
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCppUnMarshalling(const String &parcelName, const String &name, StringBuilder &sb,
    const String &prefix, bool emitType, unsigned int innerLevel) const
{
    int index = name.IndexOf('.', 0);
    String memberName = name.Substring(index + 1);
    String sizeName = String::Format("%sSize", memberName.string());
    if (emitType) {
        sb.Append(prefix).AppendFormat("%s %s;\n", EmitCppType().string(), memberName.string());
    }

    if (!elementType_->IsBooleanType() && elementType_->IsPod()) {
        sb.Append(prefix).AppendFormat("if (!ReadPodArray(%s, %s)) {\n", parcelName.string(), name.string());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: failed to read %s\", __func__);\n", name.string());
        sb.Append(prefix + TAB).Append("return false;\n");
        sb.Append(prefix).Append("}\n");
        return;
    }

    sb.Append(prefix).AppendFormat("uint32_t %s = %s.ReadUint32();\n", sizeName.string(), parcelName.string());
    sb.Append(prefix).AppendFormat("%s.clear();\n", name.string());
    sb.Append(prefix).AppendFormat("%s.reserve(%s);\n", name.string(), sizeName.string());
    sb.Append(prefix).AppendFormat(
        "for (uint32_t i%d = 0; i%d < %s; ++i%d) {\n", innerLevel, innerLevel, sizeName.string(), innerLevel);
    String valueName = String::Format("value%d", innerLevel++);
    if (elementType_->GetTypeKind() == TypeKind::TYPE_STRUCT) {
        sb.Append(prefix + TAB).AppendFormat("%s %s;\n", elementType_->EmitCppType().string(), valueName.string());
    }
    elementType_->EmitCppUnMarshalling(parcelName, valueName, sb, prefix + TAB, true, innerLevel);
    sb.Append(prefix + TAB).AppendFormat("%s.push_back(%s);\n", name.string(), valueName.string());

    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitMemoryRecycle(
    const String &name, bool isClient, bool ownership, StringBuilder &sb, const String &prefix) const
{
    String varName = name;
    String lenName = isClient ? String::Format("*%sLen", name.string()) : String::Format("%sLen", name.string());

    sb.Append(prefix).AppendFormat("if (%s != NULL) {\n", varName.string());
    auto elementTypeNeedFree = [this]() -> bool {
        if (elementType_->IsPod()) {
            return false;
        }

        return elementType_->IsStringType() ? true : false;
    };
    if (elementTypeNeedFree()) {
        if (Options::GetInstance().DoGenerateKernelCode()) {
            sb.Append(prefix + TAB).AppendFormat("for (i = 0; i < %s; i++) {\n", lenName.string());
        } else {
            sb.Append(prefix + TAB).AppendFormat("for (uint32_t i = 0; i < %s; i++) {\n", lenName.string());
        }

        String elementName = String::Format("%s[i]", varName.string());
        elementType_->EmitMemoryRecycle(elementName, false, false, sb, prefix + TAB + TAB);
        sb.Append(prefix + TAB).Append("}\n");
    }

    sb.Append(prefix + TAB).AppendFormat("OsalMemFree(%s);\n", varName.string());
    if (isClient) {
        sb.Append(prefix + TAB).AppendFormat("%s = NULL;\n", varName.string());
    }
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitJavaWriteVar(
    const String &parcelName, const String &name, StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).AppendFormat("if (%s == null) {\n", name.string());
    sb.Append(prefix + TAB).AppendFormat("%s.writeInt(-1);\n", parcelName.string());
    sb.Append(prefix).Append("} else { \n");
    EmitJavaWriteArrayVar(parcelName, name, sb, prefix + TAB);
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitJavaReadVar(
    const String &parcelName, const String &name, StringBuilder &sb, const String &prefix) const
{
    switch (elementType_->GetTypeKind()) {
        case TypeKind::TYPE_BOOLEAN:
            sb.Append(prefix).AppendFormat("%s.readBooleanArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_BYTE:
            sb.Append(prefix).AppendFormat("%s.readByteArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_SHORT:
            sb.Append(prefix).AppendFormat("%s.readShortArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_INT:
        case TypeKind::TYPE_FILEDESCRIPTOR:
            sb.Append(prefix).AppendFormat("%s.readIntArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_LONG:
            sb.Append(prefix).AppendFormat("%s.readLongArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_FLOAT:
            sb.Append(prefix).AppendFormat("%s.readFloatArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_DOUBLE:
            sb.Append(prefix).AppendFormat("%s.readDoubleArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_STRING:
            sb.Append(prefix).AppendFormat("%s.readStringArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_SEQUENCEABLE:
            sb.Append(prefix).AppendFormat("%s.readSequenceableArray(%s);\n", parcelName.string(), name.string());
            break;
        default:
            break;
    }
}

void ASTArrayType::EmitJavaReadInnerVar(
    const String &parcelName, const String &name, bool isInner, StringBuilder &sb, const String &prefix) const
{
    switch (elementType_->GetTypeKind()) {
        case TypeKind::TYPE_BOOLEAN:
            sb.Append(prefix).AppendFormat("%s[] %s = %s.readBooleanArray();\n",
                elementType_->EmitJavaType(TypeMode::NO_MODE).string(), name.string(), parcelName.string());
            break;
        case TypeKind::TYPE_BYTE:
            sb.Append(prefix).AppendFormat("%s[] %s = %s.readByteArray();\n",
                elementType_->EmitJavaType(TypeMode::NO_MODE).string(), name.string(), parcelName.string());
            break;
        case TypeKind::TYPE_SHORT:
            sb.Append(prefix).AppendFormat("%s[] %s = %s.readShortArray();\n",
                elementType_->EmitJavaType(TypeMode::NO_MODE).string(), name.string(), parcelName.string());
            break;
        case TypeKind::TYPE_INT:
        case TypeKind::TYPE_FILEDESCRIPTOR:
            sb.Append(prefix).AppendFormat("%s[] %s = %s.readIntArray();\n",
                elementType_->EmitJavaType(TypeMode::NO_MODE).string(), name.string(), parcelName.string());
            break;
        case TypeKind::TYPE_LONG:
            sb.Append(prefix).AppendFormat("%s[] %s = %s.readLongArray();\n",
                elementType_->EmitJavaType(TypeMode::NO_MODE).string(), name.string(), parcelName.string());
            break;
        case TypeKind::TYPE_FLOAT:
            sb.Append(prefix).AppendFormat("%s[] %s = %s.readFloatArray();\n",
                elementType_->EmitJavaType(TypeMode::NO_MODE).string(), name.string(), parcelName.string());
            break;
        case TypeKind::TYPE_DOUBLE:
            sb.Append(prefix).AppendFormat("%s[] %s = %s.readDoubleArray();\n",
                elementType_->EmitJavaType(TypeMode::NO_MODE).string(), name.string(), parcelName.string());
            break;
        case TypeKind::TYPE_STRING:
            sb.Append(prefix).AppendFormat("%s[] %s = %s.readStringArray();\n",
                elementType_->EmitJavaType(TypeMode::NO_MODE).string(), name.string(), parcelName.string());
            break;
        case TypeKind::TYPE_SEQUENCEABLE:
            sb.Append(prefix).AppendFormat("int size = %s.readInt();\n", parcelName.string());
            sb.Append(prefix).AppendFormat("%s %s = new %s[size];\n",
                elementType_->EmitJavaType(TypeMode::NO_MODE).string(), name.string(),
                elementType_->EmitJavaType(TypeMode::NO_MODE).string());
            sb.Append(prefix).AppendFormat("for (int i = 0; i < size; ++i) {\n");
            elementType_->EmitJavaReadInnerVar(parcelName, "value", true, sb, prefix + TAB);
            sb.Append(prefix + TAB).AppendFormat("%s[i] = value;\n", name.string());
            sb.Append(prefix).Append("}\n");
            break;
        default:
            break;
    }
}

void ASTArrayType::EmitJavaWriteArrayVar(
    const String &parcelName, const String &name, StringBuilder &sb, const String &prefix) const
{
    switch (elementType_->GetTypeKind()) {
        case TypeKind::TYPE_BOOLEAN:
            sb.Append(prefix).AppendFormat("%s.writeBooleanArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_BYTE:
            sb.Append(prefix).AppendFormat("%s.writeByteArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_SHORT:
            sb.Append(prefix).AppendFormat("%s.writeShortArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_INT:
        case TypeKind::TYPE_FILEDESCRIPTOR:
            sb.Append(prefix).AppendFormat("%s.writeIntArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_LONG:
            sb.Append(prefix).AppendFormat("%s.writeLongArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_FLOAT:
            sb.Append(prefix).AppendFormat("%s.writeFloatArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_DOUBLE:
            sb.Append(prefix).AppendFormat("%s.writeDoubleArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_STRING:
            sb.Append(prefix).AppendFormat("%s.writeStringArray(%s);\n", parcelName.string(), name.string());
            break;
        case TypeKind::TYPE_SEQUENCEABLE:
            sb.Append(prefix).AppendFormat("%s.writeSequenceableArray(%s);\n", parcelName.string(), name.string());
            break;
        default:
            break;
    }
}

void ASTArrayType::EmitCMallocVar(const String &name, const String &lenName, bool isClient, const String &ecName,
    const String &gotoLabel, StringBuilder &sb, const String &prefix) const
{
    String varName = isClient ? String::Format("*%s", name.string()) : name;
    String lenVarName = isClient ? String::Format("*%s", lenName.string()) : lenName;

    sb.Append(prefix).AppendFormat("%s = (%s*)OsalMemCalloc(sizeof(%s) * (%s));\n", varName.string(),
        elementType_->EmitCType().string(), elementType_->EmitCType().string(), lenVarName.string());
    sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", varName.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: malloc %s failed\", __func__);\n", varName.string());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_MALLOC_FAIL;\n", ecName.string());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::RegisterWriteMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const
{
    elementType_->RegisterWriteMethod(language, mode, methods);
    if (elementType_->IsPod()) {
        RegisterWritePodArrayMethod(language, mode, methods);
    } else if (elementType_->IsStringType()) {
        RegisterWriteStringArrayMethod(language, mode, methods);
    }
}

void ASTArrayType::RegisterReadMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const
{
    elementType_->RegisterReadMethod(language, mode, methods);
    if (elementType_->IsPod()) {
        RegisterReadPodArrayMethod(language, mode, methods);
    } else if (elementType_->IsStringType()) {
        RegisterReadStringArrayMethod(language, mode, methods);
    }
}

void ASTArrayType::RegisterWritePodArrayMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const
{
    using namespace std::placeholders;
    String methodName = "WritePodArray";
    switch (language) {
        case Options::Language::C:
            methods.emplace(methodName, std::bind(&ASTArrayType::EmitCWriteMethods, this, _1, _2, _3, _4));
            break;
        case Options::Language::CPP:
            methods.emplace(methodName, std::bind(&ASTArrayType::EmitCppWriteMethods, this, _1, _2, _3, _4));
            break;
        default:
            break;
    }
}

void ASTArrayType::RegisterWriteStringArrayMethod(
    Options::Language language, SerMode mode, UtilMethodMap &methods) const
{
    using namespace std::placeholders;
    if (language == Options::Language::C && elementType_->IsStringType()) {
        methods.emplace("WriteStringArray", std::bind(&ASTArrayType::EmitCWriteStrArrayMethods, this, _1, _2, _3, _4));
    }
}

void ASTArrayType::RegisterReadPodArrayMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const
{
    using namespace std::placeholders;
    String methodName = "ReadPodArray";
    switch (language) {
        case Options::Language::C: {
            auto readMethod = mode == SerMode::PROXY_SER ?
                std::bind(&ASTArrayType::EmitCReadMethods, this, _1, _2, _3, _4) :
                std::bind(&ASTArrayType::EmitCStubReadMethods, this, _1, _2, _3, _4);
            methods.emplace(methodName, readMethod);
            break;
        }
        case Options::Language::CPP:
            methods.emplace(methodName, std::bind(&ASTArrayType::EmitCppReadMethods, this, _1, _2, _3, _4));
            break;
        default:
            break;
    }
}

void ASTArrayType::RegisterReadStringArrayMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const
{
    using namespace std::placeholders;
    if (language == Options::Language::C && elementType_->IsStringType()) {
        auto readMethod = mode == SerMode::PROXY_SER ?
            std::bind(&ASTArrayType::EmitCReadStrArrayMethods, this, _1, _2, _3, _4) :
            std::bind(&ASTArrayType::EmitCStubReadStrArrayMethods, this, _1, _2, _3, _4);
        methods.emplace("ReadStringArray", readMethod);
    }
}

void ASTArrayType::EmitCWriteMethods(
    StringBuilder &sb, const String &prefix, const String &methodPrefix, bool isDecl) const
{
    String methodName = String::Format("%sWritePodArray", methodPrefix.string());
    sb.Append(prefix).AppendFormat("static bool %s(", methodName.string());
    sb.Append("struct HdfSBuf *parcel, const void *data, uint32_t elementSize, uint32_t count)");
    if (isDecl) {
        sb.Append(";\n");
        return;
    } else {
        sb.Append("\n");
    }

    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("if (!HdfSbufWriteUint32(parcel, count)) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to write array size\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    sb.Append(prefix + TAB).Append("if (data == NULL && count == 0) {\n");
    sb.Append(prefix + TAB + TAB).Append("return true;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix + TAB).Append("if (!HdfSbufWriteBuffer(");
        sb.Append("parcel, (const void *)data, elementSize * count)) {\n");
    } else {
        sb.Append(prefix + TAB).Append("if (!HdfSbufWriteUnpadBuffer(");
        sb.Append("parcel, (const uint8_t *)data, elementSize * count)) {\n");
    }

    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to write array\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    sb.Append(prefix + TAB).Append("return true;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCReadMethods(
    StringBuilder &sb, const String &prefix, const String &methodPrefix, bool isDecl) const
{
    String methodName = String::Format("%sReadPodArray", methodPrefix.string());
    sb.Append(prefix).AppendFormat("static bool %s(", methodName.string());
    sb.Append("struct HdfSBuf *parcel, void *data, uint32_t elementSize, uint32_t *count)");
    if (isDecl) {
        sb.Append(";\n");
        return;
    } else {
        sb.Append("\n");
    }

    sb.Append(prefix).Append("{\n");
    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix + TAB).Append("void *dataPtr = NULL;\n");
        sb.Append(prefix + TAB).Append("uint32_t dataLen = 0;\n");
    }
    sb.Append(prefix + TAB).Append("uint32_t elementCount = 0;\n");
    sb.Append(prefix + TAB).Append("if (!HdfSbufReadUint32(parcel, &elementCount)) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to read array size\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    sb.Append(prefix + TAB).Append("if (elementCount == 0) {\n");
    sb.Append(prefix + TAB + TAB).Append("goto FINISHED;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix + TAB).Append("if (!HdfSbufReadBuffer(parcel, (const void **)&dataPtr, &dataLen)) {\n");
    } else {
        sb.Append(prefix + TAB)
            .Append("const void * dataPtr = HdfSbufReadUnpadBuffer(parcel, elementSize * elementCount);\n");
        sb.Append(prefix + TAB).Append("if (dataPtr == NULL) {\n");
    }

    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to read array\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    sb.Append(prefix + TAB).Append("if (memcpy_s(");
    sb.Append("data, elementSize * elementCount, dataPtr, elementSize * elementCount) != EOK) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to copy array data\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    sb.Append("FINISHED:\n");
    sb.Append(prefix + TAB).Append("*count = elementCount;\n");
    sb.Append(prefix + TAB).Append("return true;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCStubReadMethods(
    StringBuilder &sb, const String &prefix, const String &methodPrefix, bool isDecl) const
{
    String methodName = String::Format("%sReadPodArray", methodPrefix.string());
    sb.Append(prefix).AppendFormat("static bool %s(", methodName.string());
    sb.Append("struct HdfSBuf *parcel, void **data, uint32_t elementSize, uint32_t *count)");
    if (isDecl) {
        sb.Append(";\n");
        return;
    } else {
        sb.Append("\n");
    }
    sb.Append(prefix).Append("{\n");
    EmitCStubReadMethodBody(sb, prefix + TAB);
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCStubReadMethodBody(StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).Append("const void * dataPtr = NULL;\n");
    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix).Append("uint32_t dataLen = 0;\n");
    }
    sb.Append(prefix).Append("void *memPtr = NULL;\n");
    sb.Append(prefix).Append("uint32_t elementCount = 0;\n");
    sb.Append(prefix).Append("if (count == NULL || data == NULL || elementSize == 0) {\n");
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: invalid param\", __func__);\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).Append("if (!HdfSbufReadUint32(parcel, &elementCount)) {\n");
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to read element count\", __func__);\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).Append("if (elementCount == 0) {\n");
    sb.Append(prefix + TAB).Append("*count = elementCount;\n");
    sb.Append(prefix + TAB).Append("return true;\n");
    sb.Append(prefix).Append("}\n\n");
    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix).Append("if (!HdfSbufReadBuffer(parcel, (const void **)&dataPtr, &dataLen)) {\n");
    } else {
        sb.Append(prefix).Append("dataPtr = HdfSbufReadUnpadBuffer(parcel, elementSize * elementCount);\n");
        sb.Append(prefix).Append("if (dataPtr == NULL) {\n");
    }
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to read buffer data\", __func__);\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).Append("memPtr = OsalMemCalloc(elementSize * elementCount);\n");
    sb.Append(prefix).Append("if (memPtr == NULL) {\n");
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to malloc buffer\", __func__);\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix)
        .Append("if (memcpy_s(memPtr, elementSize * elementCount, dataPtr, elementSize * elementCount) != EOK) {\n");
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to memcpy buffer\", __func__);\n");
    sb.Append(prefix + TAB).Append("OsalMemFree(memPtr);\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).Append("*data = memPtr;\n");
    sb.Append(prefix).Append("*count = elementCount;\n");
    sb.Append(prefix).Append("return true;\n");
}

void ASTArrayType::EmitCWriteStrArrayMethods(
    StringBuilder &sb, const String &prefix, const String &methodPrefix, bool isDecl) const
{
    String methodName = String::Format("%sWriteStringArray", methodPrefix.string());
    sb.Append(prefix).AppendFormat("static bool %s(", methodName.string());
    sb.Append("struct HdfSBuf *parcel, char **data, uint32_t count)");
    if (isDecl) {
        sb.Append(";\n");
        return;
    } else {
        sb.Append("\n");
    }
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("uint32_t i = 0;\n");
    sb.Append(prefix + TAB).Append("if (parcel == NULL) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: invalid sbuf\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).Append("if (!HdfSbufWriteUint32(parcel, count)) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to write count of array\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).Append("if (count == 0) {\n");
    sb.Append(prefix + TAB + TAB).Append("return true;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).Append("if (data == NULL) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: invalid array object\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).Append("for (i = 0; i < count; i++) {\n");
    sb.Append(prefix + TAB + TAB).Append("if (!HdfSbufWriteString(parcel, data[i])) {\n");
    sb.Append(prefix + TAB + TAB + TAB)
        .Append("HDF_LOGE(\"%{public}s: failed to write element of array\", __func__);\n");
    sb.Append(prefix + TAB + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).Append("return true;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCReadStrArrayMethods(
    StringBuilder &sb, const String &prefix, const String &methodPrefix, bool isDecl) const
{
    String methodName = String::Format("%sReadStringArray", methodPrefix.string());
    sb.Append(prefix).AppendFormat("static bool %s(", methodName.string());
    sb.Append("struct HdfSBuf *parcel, char **data, uint32_t *count)");
    if (isDecl) {
        sb.Append(";\n");
        return;
    } else {
        sb.Append("\n");
    }
    sb.Append(prefix).Append("{\n");
    EmitCReadStrArrayMethodBody(sb, prefix + TAB);
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCReadStrArrayMethodBody(StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).Append("uint32_t i = 0;\n");
    sb.Append(prefix).Append("uint32_t dataCount = 0;\n");
    EmitCCheckParamOfReadStringArray(sb, prefix);
    sb.Append(prefix).Append("if (!HdfSbufReadUint32(parcel, &dataCount)) {\n");
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to read count of array\", __func__);\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).Append("if (dataCount == 0) {\n");
    sb.Append(prefix + TAB).Append("*count = dataCount;\n");
    sb.Append(prefix + TAB).Append("return true;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).Append("for (i = 0; i < dataCount; i++) {\n");
    sb.Append(prefix + TAB).Append("char *elementStr = NULL;\n");
    sb.Append(prefix + TAB).Append("uint32_t strLen = 0;\n");
    sb.Append(prefix + TAB).Append("const char *str = HdfSbufReadString(parcel);\n");
    sb.Append(prefix + TAB).Append("if (str == NULL) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to read string\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("goto ERROR;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("strLen = strlen(str);\n");
    sb.Append(prefix + TAB).Append("elementStr = (char *)OsalMemCalloc(strLen + 1);\n");
    sb.Append(prefix + TAB).Append("if (elementStr == NULL) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to malloc element of array\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("goto ERROR;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("if (strcpy_s(elementStr, strLen + 1, str) != EOK) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed copy element of array\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("OsalMemFree(elementStr);\n");
    sb.Append(prefix + TAB + TAB).Append("goto ERROR;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("data[i] = elementStr;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).Append("*count = dataCount;\n");
    sb.Append(prefix).Append("return true;\n\n");
    sb.Append("ERROR:\n");
    sb.Append(prefix).Append("for (i = 0; i < dataCount; ++i) {\n");
    sb.Append(prefix + TAB).Append("if (data[i] != NULL) {\n");
    sb.Append(prefix + TAB + TAB).Append("OsalMemFree(data[i]);\n");
    sb.Append(prefix + TAB + TAB).Append("data[i] = NULL;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix).Append("}\n");
    sb.Append(prefix).Append("*count = 0;\n");
    sb.Append(prefix).Append("return false;\n");
}

void ASTArrayType::EmitCCheckParamOfReadStringArray(StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).Append("if (parcel == NULL) {\n");
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: invalid sbuf\", __func__);\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).Append("if (data == NULL || count == NULL) {\n");
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: invalid array object\", __func__);\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n\n");
}

void ASTArrayType::EmitCStubReadStrArrayMethods(
    StringBuilder &sb, const String &prefix, const String &methodPrefix, bool isDecl) const
{
    String methodName = String::Format("%sReadStringArray", methodPrefix.string());
    sb.Append(prefix).AppendFormat("static bool %s(", methodName.string());
    sb.Append("struct HdfSBuf *parcel, char ***data, uint32_t *count)");
    if (isDecl) {
        sb.Append(";\n");
        return;
    } else {
        sb.Append("\n");
    }
    sb.Append(prefix).Append("{\n");
    EmitCStubReadStrArrayMethodBody(sb, prefix + TAB);
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCStubReadStrArrayMethodBody(StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).Append("uint32_t i = 0;\n");
    sb.Append(prefix).Append("char **dataPtr = NULL;\n");
    sb.Append(prefix).Append("uint32_t dataCount = 0;\n");
    EmitCCheckParamOfReadStringArray(sb, prefix);
    sb.Append(prefix).Append("if (!HdfSbufReadUint32(parcel, &dataCount)) {\n");
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to read count of array\", __func__);\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).Append("if (dataCount == 0) {\n");
    sb.Append(prefix + TAB).Append("*count = dataCount;\n");
    sb.Append(prefix + TAB).Append("return true;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).Append("dataPtr = (char **)OsalMemCalloc(sizeof(char *) * dataCount);\n");
    sb.Append(prefix).Append("if (dataPtr == NULL) {\n");
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to malloc array\", __func__);\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).Append("for (i = 0; i < dataCount; i++) {\n");
    sb.Append(prefix + TAB).Append("char *elementStr = NULL;\n");
    sb.Append(prefix + TAB).Append("uint32_t strLen = 0;\n");
    sb.Append(prefix + TAB).Append("const char *str = HdfSbufReadString(parcel);\n");
    sb.Append(prefix + TAB).Append("if (str == NULL) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to malloc array\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("goto ERROR;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).Append("strLen = strlen(str);\n");
    sb.Append(prefix + TAB).Append("elementStr = (char *)OsalMemCalloc(strLen + 1);\n");
    sb.Append(prefix + TAB).Append("if (elementStr == NULL) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to malloc element of array\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("goto ERROR;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).Append("if (strcpy_s(elementStr, strLen + 1, str) != EOK) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed copy element of array\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("OsalMemFree(elementStr);\n");
    sb.Append(prefix + TAB + TAB).Append("goto ERROR;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("dataPtr[i] = elementStr;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).Append("*count = dataCount;\n");
    sb.Append(prefix).Append("*data = dataPtr;\n");
    sb.Append(prefix).Append("return true;\n\n");
    EmitCStubReadStrArrayFree(sb, prefix);
}

void ASTArrayType::EmitCStubReadStrArrayFree(StringBuilder &sb, const String &prefix) const
{
    sb.Append("ERROR:\n");
    sb.Append(prefix).Append("if (dataPtr != NULL) {\n");
    sb.Append(prefix + TAB).Append("for (i = 0; i < dataCount; i++) {\n");
    sb.Append(prefix + TAB + TAB).Append("if (dataPtr[i] != NULL) {\n");
    sb.Append(prefix + TAB + TAB + TAB).Append("OsalMemFree(dataPtr[i]);\n");
    sb.Append(prefix + TAB + TAB + TAB).Append("dataPtr[i] = NULL;\n");
    sb.Append(prefix + TAB + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("OsalMemFree(dataPtr);\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).Append("*count = 0;\n");
    sb.Append(prefix).Append("return false;\n");
}

void ASTArrayType::EmitCppWriteMethods(
    StringBuilder &sb, const String &prefix, const String &methodPrefix, bool isDecl) const
{
    String methodName = String::Format("%sWritePodArray", methodPrefix.string());
    sb.Append(prefix).AppendFormat("template<typename ElementType>\n");
    sb.Append(prefix).AppendFormat(
        "static bool %s(MessageParcel &parcel, const std::vector<ElementType> &data)", methodName.string());
    if (isDecl) {
        sb.Append(";\n");
        return;
    } else {
        sb.Append("\n");
    }

    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("if (!parcel.WriteUint32(data.size())) {\n");
    sb.Append(prefix + TAB + TAB).AppendFormat("HDF_LOGE(\"%{public}s: failed to write data size\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("if (data.empty()) {\n");
    sb.Append(prefix + TAB + TAB).Append("return true;\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("if (!parcel.WriteUnpadBuffer(");
    sb.Append("(const void*)data.data(), sizeof(ElementType) * data.size())) {\n");
    sb.Append(prefix + TAB + TAB).AppendFormat("HDF_LOGE(\"%{public}s: failed to write array\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("return true;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTArrayType::EmitCppReadMethods(
    StringBuilder &sb, const String &prefix, const String &methodPrefix, bool isDecl) const
{
    String methodName = String::Format("%sReadPodArray", methodPrefix.string());
    sb.Append(prefix).AppendFormat("template<typename ElementType>\n");
    sb.Append(prefix).AppendFormat(
        "static bool %s(MessageParcel &parcel, std::vector<ElementType> &data)", methodName.string());
    if (isDecl) {
        sb.Append(";\n");
        return;
    } else {
        sb.Append("\n");
    }

    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("data.clear();\n");
    sb.Append(prefix + TAB).Append("uint32_t size = parcel.ReadUint32();\n");
    sb.Append(prefix + TAB).Append("if (size == 0) {\n");
    sb.Append(prefix + TAB + TAB).Append("return true;\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("const ElementType *dataPtr = reinterpret_cast<const ElementType*>(");
    sb.Append("parcel.ReadUnpadBuffer(sizeof(ElementType) * size));\n");
    sb.Append(prefix + TAB).Append("if (dataPtr == nullptr) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGI(\"%{public}s: failed to read data\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("data.assign(dataPtr, dataPtr + size);\n");
    sb.Append(prefix + TAB).Append("return true;\n");
    sb.Append(prefix).Append("}\n");
}

bool ASTListType::IsArrayType()
{
    return false;
}

bool ASTListType::IsListType()
{
    return true;
}

String ASTListType::ToString() const
{
    return String::Format("List<%s>", elementType_->ToString().string());
}

TypeKind ASTListType::GetTypeKind()
{
    return TypeKind::TYPE_LIST;
}

String ASTListType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    return String::Format("List<%s>", elementType_->EmitJavaType(mode, true).string());
}

void ASTListType::EmitJavaWriteVar(
    const String &parcelName, const String &name, StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).AppendFormat("%s.writeInt(%s.size());\n", parcelName.string(), name.string());
    sb.Append(prefix).AppendFormat(
        "for (%s element : %s) {\n", elementType_->EmitJavaType(TypeMode::NO_MODE).string(), name.string());
    elementType_->EmitJavaWriteVar(parcelName, "element", sb, prefix + TAB);
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitJavaReadVar(
    const String &parcelName, const String &name, StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).AppendFormat("int %sSize = %s.readInt();\n", name.string(), parcelName.string());
    sb.Append(prefix).AppendFormat("for (int i = 0; i < %sSize; ++i) {\n", name.string());

    elementType_->EmitJavaReadInnerVar(parcelName, "value", false, sb, prefix + TAB);
    sb.Append(prefix + TAB).AppendFormat("%s.add(value);\n", name.string());
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitJavaReadInnerVar(
    const String &parcelName, const String &name, bool isInner, StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).AppendFormat("%s %s = new Array%s();\n", EmitJavaType(TypeMode::NO_MODE).string(), name.string(),
        EmitJavaType(TypeMode::NO_MODE).string());
    sb.Append(prefix).AppendFormat("int %sSize = %s.readInt();\n", name.string(), parcelName.string());
    sb.Append(prefix).AppendFormat("for (int i = 0; i < %sSize; ++i) {\n", name.string());
    elementType_->EmitJavaReadInnerVar(parcelName, "value", true, sb, prefix + TAB);
    sb.Append(prefix + TAB).AppendFormat("%s.add(value);\n", name.string());
    sb.Append(prefix).Append("}\n");
}
} // namespace HDI
} // namespace OHOS