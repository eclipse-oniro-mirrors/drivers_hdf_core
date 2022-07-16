/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_attribute.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
static void AttributesFormat(StringBuilder &sb, const std::vector<String> attrs)
{
    if (attrs.empty()) {
        return;
    }

    sb.Append("[");
    for (size_t i = 0; i < attrs.size(); i++) {
        sb.Append(attrs[i]);
        if (i + 1 < attrs.size()) {
            sb.Append(", ");
        }
    }
    sb.Append("]");
}

String ASTTypeAttr::ToString() const
{
    StringBuilder sb;
    std::vector<String> attrVec;
    if (isFull_) {
        attrVec.push_back("full");
    }

    if (isLite_) {
        attrVec.push_back("lite");
    }

    AttributesFormat(sb, attrVec);
    return sb.ToString();
}

String ASTTypeAttr::Dump(const String &prefix)
{
    return prefix + ToString();
}

String ASTInfAttr::ToString() const
{
    StringBuilder sb;
    std::vector<String> attrVec;
    if (isFull_) {
        attrVec.push_back("full");
    }

    if (isLite_) {
        attrVec.push_back("lite");
    }

    if (isCallback_) {
        attrVec.push_back("callback");
    }

    if (isOneWay_) {
        attrVec.push_back("oneway");
    }

    AttributesFormat(sb, attrVec);
    return sb.ToString();
}

String ASTInfAttr::Dump(const String &prefix)
{
    return prefix + ToString();
}

String ASTMethodAttr::ToString() const
{
    StringBuilder sb;
    std::vector<String> attrVec;
    if (isFull_) {
        attrVec.push_back("full");
    }

    if (isLite_) {
        attrVec.push_back("lite");
    }

    if (isOneWay_) {
        attrVec.push_back("oneway");
    }

    AttributesFormat(sb, attrVec);
    return sb.ToString();
}

String ASTMethodAttr::Dump(const String &prefix)
{
    return prefix + ToString();
}

String ASTParamAttr::ToString() const
{
    return String::Format("[%s]", (value_ == ParamAttr::PARAM_IN) ? "in" : "out");
}

String ASTParamAttr::Dump(const String &prefix)
{
    return prefix + ToString();
}
} // namespace HDI
} // namespace OHOS