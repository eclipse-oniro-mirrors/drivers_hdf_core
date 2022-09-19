/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "lexer/token.h"

#include <unordered_map>

#include "util/common.h"
#include "util/file.h"
#include "util/string_builder.h"
#include "util/string_helper.h"

namespace OHOS {
namespace HDI {
std::string Token::Dump()
{
    StringBuilder sb;
    sb.AppendFormat(
        "{kind_:%u, row_:%u, col_:%u, value_:%s}", (size_t)kind_, location_.row_, location_.col_, value_.c_str());
    return sb.ToString();
}

std::string LocInfo(const Token &token)
{
    size_t index = token.location_.filePath_.rfind(SEPARATOR);
    std::string fileName =
        (index == std::string::npos) ? token.location_.filePath_ : token.location_.filePath_.substr(index + 1);
    return StringHelper::Format("%s:%u:%u", fileName.c_str(), token.location_.row_, token.location_.col_);
}
} // namespace HDI
} // namespace OHOS