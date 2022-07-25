/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_fd_type.h"

namespace OHOS {
namespace HDI {
bool ASTFdType::IsFdType()
{
    return true;
}

String ASTFdType::ToString() const
{
    return "FileDescriptor";
}

TypeKind ASTFdType::GetTypeKind()
{
    return TypeKind::TYPE_FILEDESCRIPTOR;
}

String ASTFdType::EmitCType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return "int";
        case TypeMode::PARAM_IN:
            return "int";
        case TypeMode::PARAM_OUT:
            return "int*";
        case TypeMode::LOCAL_VAR:
            return "int";
        default:
            return "unknow type";
    }
}

String ASTFdType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return "int";
        case TypeMode::PARAM_IN:
            return "int";
        case TypeMode::PARAM_OUT:
            return "int&";
        case TypeMode::LOCAL_VAR:
            return "int";
        default:
            return "unknow type";
    }
}

String ASTFdType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    return isInnerType ? "Integer" : "int";
}

void ASTFdType::EmitCWriteVar(const String &parcelName, const String &name, const String &ecName,
    const String &gotoLabel, StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).AppendFormat("if (!WriteFileDescriptor(%s, %s)) {\n", parcelName.string(), name.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");
}

void ASTFdType::EmitCProxyReadVar(const String &parcelName, const String &name, bool isInnerType, const String &ecName,
    const String &gotoLabel, StringBuilder &sb, const String &prefix) const
{
    if (isInnerType) {
        sb.Append(prefix).AppendFormat("if (!ReadFileDescriptor(%s, &%s)) {\n", parcelName.string(), name.string());
    } else {
        sb.Append(prefix).AppendFormat("if (!ReadFileDescriptor(%s, %s)) {\n", parcelName.string(), name.string());
    }
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");
}

void ASTFdType::EmitCStubReadVar(const String &parcelName, const String &name, const String &ecName,
    const String &gotoLabel, StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).AppendFormat("if (!ReadFileDescriptor(%s, &%s)) {\n", parcelName.string(), name.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");
}

void ASTFdType::EmitCppWriteVar(const String &parcelName, const String &name, StringBuilder &sb, const String &prefix,
    unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!WriteFileDescriptor(%s, %s)) {\n", parcelName.string(), name.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTFdType::EmitCppReadVar(const String &parcelName, const String &name, StringBuilder &sb, const String &prefix,
    bool initVariable, unsigned int innerLevel) const
{
    if (initVariable) {
        sb.Append(prefix).AppendFormat("%s %s = -1;\n", EmitCppType().string(), name.string());
    } else {
        sb.Append(prefix).AppendFormat("%s = -1;\n", name.string());
    }

    sb.Append(prefix).AppendFormat("if (!ReadFileDescriptor(%s, %s)) {\n", parcelName.string(), name.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).AppendFormat("}\n");
}

void ASTFdType::EmitCMarshalling(const String &name, StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).AppendFormat("if (!WriteFileDescriptor(data, %s)) {\n", name.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTFdType::EmitCUnMarshalling(const String &name, const String &gotoLabel, StringBuilder &sb, const String &prefix,
    std::vector<String> &freeObjStatements) const
{
    sb.Append(prefix).AppendFormat("if (!ReadFileDescriptor(data, &%s)) {\n", name.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");
}

void ASTFdType::EmitCppMarshalling(const String &parcelName, const String &name, StringBuilder &sb,
    const String &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!WriteFileDescriptor(%s, %s)) {\n", parcelName.string(), name.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTFdType::EmitCppUnMarshalling(const String &parcelName, const String &name, StringBuilder &sb,
    const String &prefix, bool emitType, unsigned int innerLevel) const
{
    if (emitType) {
        sb.Append(prefix).AppendFormat("%s %s = -1;\n", EmitCppType().string(), name.string());
    } else {
        sb.Append(prefix).AppendFormat("%s = -1;\n", name.string());
    }

    sb.Append(prefix).AppendFormat("if (!ReadFileDescriptor(%s, %s)) {\n", parcelName.string(), name.string());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).AppendFormat("}\n");
}

void ASTFdType::EmitJavaWriteVar(
    const String &parcelName, const String &name, StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).AppendFormat("%s.writeInt(%s);\n", parcelName.string(), name.string());
}

void ASTFdType::EmitJavaReadVar(
    const String &parcelName, const String &name, StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).AppendFormat("%s = %s.readInt();\n", name.string(), parcelName.string());
}

void ASTFdType::EmitJavaReadInnerVar(
    const String &parcelName, const String &name, bool isInner, StringBuilder &sb, const String &prefix) const
{
    sb.Append(prefix).AppendFormat(
        "%s %s = %s.readInt();\n", EmitJavaType(TypeMode::NO_MODE).string(), name.string(), parcelName.string());
}

void ASTFdType::RegisterWriteMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const
{
    using namespace std::placeholders;
    String methodName = String::Format("Write%s", ToString().string());
    switch (language) {
        case Options::Language::C:
            methods.emplace(methodName, std::bind(&ASTFdType::EmitCWriteMethods, this, _1, _2, _3, _4));
            break;
        case Options::Language::CPP:
            methods.emplace(methodName, std::bind(&ASTFdType::EmitCppWriteMethods, this, _1, _2, _3, _4));
            break;
        default:
            break;
    }
}

void ASTFdType::RegisterReadMethod(Options::Language language, SerMode mode, UtilMethodMap &methods) const
{
    using namespace std::placeholders;
    String methodName = String::Format("Read%s", ToString().string());
    switch (language) {
        case Options::Language::C:
            methods.emplace(methodName, std::bind(&ASTFdType::EmitCReadMethods, this, _1, _2, _3, _4));
            break;
        case Options::Language::CPP:
            methods.emplace(methodName, std::bind(&ASTFdType::EmitCppReadMethods, this, _1, _2, _3, _4));
            break;
        default:
            break;
    }
}

void ASTFdType::EmitCWriteMethods(
    StringBuilder &sb, const String &prefix, const String &methodPrefix, bool isDecl) const
{
    String methodName = String::Format("%sWrite%s", methodPrefix.string(), ToString().string());
    if (isDecl) {
        sb.Append(prefix).AppendFormat("static bool %s(struct HdfSBuf *data, int fd);\n", methodName.string());
        return;
    }
    sb.Append(prefix).AppendFormat("static bool %s(struct HdfSBuf *data, int fd)\n", methodName.string());
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("if (!HdfSbufWriteInt8(data, fd >= 0 ? 1 : 0)) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to write fd vailed\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("if (!HdfSbufWriteFileDescriptor(data, fd)) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to write fd\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTFdType::EmitCReadMethods(
    StringBuilder &sb, const String& prefix, const String& methodPrefix, bool isDecl) const
{
    String methodName = String::Format("%sRead%s", methodPrefix.string(), ToString().string());
    if (isDecl) {
        sb.Append(prefix).AppendFormat("static bool %s(struct HdfSBuf *data, int *fd);\n", methodName.string());
        return;
    }
    sb.Append(prefix).AppendFormat("static bool %s(struct HdfSBuf *data, int *fd)\n", methodName.string());
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("if (data == NULL) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: invalid HdfSBuf obj\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("if (fd == NULL) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: invalid fd pointer\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("*fd = -1;\n");
    sb.Append(prefix + TAB).Append("bool fdValied = false;\n");
    sb.Append(prefix + TAB).Append("if (!HdfSbufReadInt8(data, (int8_t*)& fdValied)) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to read fdValied\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("if (!fdValied) {\n");
    sb.Append(prefix + TAB + TAB).Append("return true;\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("*fd = HdfSbufReadFileDescriptor(data);\n");
    sb.Append(prefix + TAB).Append("if (*fd < 0) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to read fd\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("return true;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTFdType::EmitCppWriteMethods(
    StringBuilder &sb, const String& prefix, const String& methodPrefix, bool isDecl) const
{
    String methodName = String::Format("%sWrite%s", methodPrefix.string(), ToString().string());
    if (isDecl) {
        sb.Append(prefix).AppendFormat("static bool %s(MessageParcel &data, int fd)\n", methodName.string());
        return;
    }
    sb.Append(prefix).AppendFormat("static bool %s(MessageParcel &data, int fd)\n", methodName.string());
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("if (!data.WriteBool(fd >= 0 ? true : false)) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to write fd vailed\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("if (fd < 0) {\n");
    sb.Append(prefix + TAB + TAB).Append("return true;\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("if (!data.WriteFileDescriptor(fd)) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to write fd\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("return true;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTFdType::EmitCppReadMethods(
    StringBuilder &sb, const String& prefix, const String& methodPrefix, bool isDecl) const
{
    String methodName = String::Format("%sRead%s", methodPrefix.string(), ToString().string());
    if (isDecl) {
        sb.Append(prefix).AppendFormat("static bool %s(MessageParcel &data, int &fd)\n", methodName.string());
        return;
    }
    sb.Append(prefix).AppendFormat("static bool %s(MessageParcel &data, int &fd)\n", methodName.string());
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("fd = -1;\n");
    sb.Append(prefix + TAB).Append("if (data.ReadBool()) {\n");
    sb.Append(prefix + TAB + TAB).Append("fd = data.ReadFileDescriptor();\n");
    sb.Append(prefix + TAB + TAB).Append("if (fd < 0) {\n");
    sb.Append(prefix + TAB + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to read fd\", __func__);\n");
    sb.Append(prefix + TAB + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).Append("return true;\n");
    sb.Append(prefix).Append("}\n");
}
} // namespace HDI
} // namespace OHOS