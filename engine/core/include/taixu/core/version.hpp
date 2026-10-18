#pragma once

// 引擎版本与构建标识

#define TXU_VERSION_MAJOR 0
#define TXU_VERSION_MINOR 1
#define TXU_VERSION_PATCH 0

namespace taixu::core
{

    // 版本号字符串，形如 "0.1.0"
    const char* version_string() noexcept;

    // 编译器与构建配置标识，形如 "clang 20.1.8 release"
    const char* build_id() noexcept;

} // namespace taixu::core
