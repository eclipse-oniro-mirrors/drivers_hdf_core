/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_JAVA_CODE_EMITTER_H
#define OHOS_HDI_JAVA_CODE_EMITTER_H

#include <cctype>

#include "ast/ast.h"
#include "codegen/code_emitter.h"
#include "util/autoptr.h"
#include "util/light_refcount_base.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
class JavaCodeEmitter : public CodeEmitter {
public:
    ~JavaCodeEmitter() override = default;

protected:
    bool CreateDirectory();

    void EmitLicense(StringBuilder &sb);

    void EmitPackage(StringBuilder &sb);

    void EmitInterfaceMethodCommands(StringBuilder &sb, const std::string &prefix) override;

    std::string MethodName(const std::string &name) const;

    std::string SpecificationParam(StringBuilder &paramSb, const std::string &prefix) const;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_JAVA_CODE_EMITTER_H