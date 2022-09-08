/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_buffer_handle_type.h"

namespace OHOS {
namespace HDI {
bool ASTBufferHandleType::IsBufferHandleType()
{
    return true;
}

std::string ASTBufferHandleType::ToString() const
{
    return "BufferHandle";
}

TypeKind ASTBufferHandleType::GetTypeKind()
{
    return TypeKind::TYPE_BUFFER_HANDLE;
}

std::string ASTBufferHandleType::EmitCType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return "BufferHandle*";
        case TypeMode::PARAM_IN:
            return "const BufferHandle*";
        case TypeMode::PARAM_OUT:
            return "BufferHandle**";
        case TypeMode::LOCAL_VAR:
            return "BufferHandle*";
        default:
            return "unknow type";
    }
}

std::string ASTBufferHandleType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return "sptr<HdiBufferHandle>";
        case TypeMode::PARAM_IN:
            return "const sptr<HdiBufferHandle>&";
        case TypeMode::PARAM_OUT:
            return "sptr<HdiBufferHandle>&";
        case TypeMode::LOCAL_VAR:
            return "sptr<HdiBufferHandle>";
        default:
            return "unknow type";
    }
}

void ASTBufferHandleType::EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteStrongParcelable(%s)) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTBufferHandleType::EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool initVariable, unsigned int innerLevel) const
{
    sb.Append(prefix);
    if (initVariable) {
        sb.AppendFormat("%s ", EmitCppType().c_str());
    }
    sb.AppendFormat("%s = %s.ReadStrongParcelable<HdiBufferHandle>();\n", name.c_str(), parcelName.c_str());
}

void ASTBufferHandleType::EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteStrongParcelable(%s)) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTBufferHandleType::EmitCppUnMarshalling(const std::string &parcelName, const std::string &name,
    StringBuilder &sb, const std::string &prefix, bool emitType, unsigned int innerLevel) const
{
    sb.Append(prefix);
    if (emitType) {
        sb.AppendFormat("%s ", EmitCppType().c_str());
    }
    sb.AppendFormat("%s = %s.ReadStrongParcelable<HdiBufferHandle>();\n", name.c_str(), parcelName.c_str());
}
} // namespace HDI
} // namespace OHOS