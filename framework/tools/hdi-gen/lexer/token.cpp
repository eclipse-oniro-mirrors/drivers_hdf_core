/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "lexer/token.h"

#include <unordered_map>

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
    size_t index = token.location_.filePath_.rfind(File::separator);
    std::string fileName =
        (index == std::string::npos) ? token.location_.filePath_ : token.location_.filePath_.substr(index + 1);
    return StringHelper::Format("%s:%u:%u", fileName.c_str(), token.location_.row_, token.location_.col_);
}

std::string TokenTypeToString(TokenType type)
{
    static std::unordered_map<TokenType, std::string> tokenStrMap = {
        {TokenType::BOOLEAN,              "boolean"       },
        {TokenType::BYTE,                 "byte"          },
        {TokenType::SHORT,                "short"         },
        {TokenType::INT,                  "int"           },
        {TokenType::LONG,                 "long"          },
        {TokenType::STRING,               "String"        },
        {TokenType::FLOAT,                "float"         },
        {TokenType::DOUBLE,               "double"        },
        {TokenType::FD,                   "FileDescriptor"},
        {TokenType::ASHMEM,               "Ashmem"        },
        {TokenType::BUFFER_HANDLE,        "BufferHandle"  },
        {TokenType::LIST,                 "List"          },
        {TokenType::MAP,                  "Map"           },
        {TokenType::SMQ,                  "SharedMemQueue"},
        {TokenType::CHAR,                 "char"          },
        {TokenType::UNSIGNED,             "unsigned"      },
        {TokenType::ENUM,                 "enum"          },
        {TokenType::STRUCT,               "struct"        },
        {TokenType::UNION,                "union"         },
        {TokenType::PACKAGE,              "package"       },
        {TokenType::SEQ,                  "sequenceable"  },
        {TokenType::IMPORT,               "import"        },
        {TokenType::INTERFACE,            "interface"     },
        {TokenType::EXTENDS,              "extends"       },
        {TokenType::ONEWAY,               "oneway"        },
        {TokenType::CALLBACK,             "callback"      },
        {TokenType::FULL,                 "full"          },
        {TokenType::LITE,                 "lite"          },
        {TokenType::IN,                   "in"            },
        {TokenType::OUT,                  "out"           },
        {TokenType::DOT,                  "."             },
        {TokenType::COMMA,                ","             },
        {TokenType::COLON,                ":"             },
        {TokenType::ASSIGN,               "="             },
        {TokenType::SEMICOLON,            ";"             },
        {TokenType::BRACES_LEFT,          "{"             },
        {TokenType::BRACES_RIGHT,         "}"             },
        {TokenType::BRACKETS_LEFT,        "["             },
        {TokenType::BRACKETS_RIGHT,       "]"             },
        {TokenType::PARENTHESES_LEFT,     "("             },
        {TokenType::PARENTHESES_RIGHT,    ")"             },
        {TokenType::ANGLE_BRACKETS_LEFT,  "<"             },
        {TokenType::ANGLE_BRACKETS_RIGHT, ">"             },
    };
    auto it = tokenStrMap.find(type);
    return (it != tokenStrMap.end()) ? it->second : "";
}

int TokenTypeToChar(TokenType type)
{
    static std::unordered_map<TokenType, char> tokenCharMap = {
        {TokenType::DOT,                  '.'},
        {TokenType::COMMA,                '.'},
        {TokenType::COLON,                ':'},
        {TokenType::ASSIGN,               '='},
        {TokenType::SEMICOLON,            ';'},
        {TokenType::BRACES_LEFT,          '{'},
        {TokenType::BRACES_RIGHT,         '}'},
        {TokenType::BRACKETS_LEFT,        '['},
        {TokenType::BRACKETS_RIGHT,       ']'},
        {TokenType::PARENTHESES_LEFT,     '('},
        {TokenType::PARENTHESES_RIGHT,    ')'},
        {TokenType::ANGLE_BRACKETS_LEFT,  '<'},
        {TokenType::ANGLE_BRACKETS_RIGHT, '>'},
    };

    auto it = tokenCharMap.find(type);
    return (it != tokenCharMap.end()) ? it->second : -1;
}
} // namespace HDI
} // namespace OHOS