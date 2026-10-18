#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <cstring>

#include "taixu/core/version.hpp"

TEST_CASE("版本号与头文件宏一致") { CHECK(std::strcmp(taixu::core::version_string(), "0.1.0") == 0); }

TEST_CASE("构建标识非空且带编译器名")
{
    const char* id = taixu::core::build_id();
    CHECK(id != nullptr);
    CHECK(std::strlen(id) > 0);
#if defined(__clang__)
    CHECK(std::strncmp(id, "clang", 5) == 0);
#endif
}
