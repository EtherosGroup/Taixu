cmake_minimum_required(VERSION 3.28)

project(Taixu
    VERSION 0.1.0
    DESCRIPTION "Taixu 游戏引擎"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 单配置生成器未指定构建类型时的默认值
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "构建类型" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# 依赖拉取到构建目录；third_party/ 只放覆盖层与本地补丁
set(FETCHCONTENT_QUIET OFF)

option(TXU_BUILD_TESTS "构建单元测试" ON)
option(TXU_WARNINGS_AS_ERRORS "把编译告警当作错误" OFF)
option(TXU_ENABLE_SANITIZERS "启用 AddressSanitizer 与 UndefinedBehaviorSanitizer" OFF)

# 所有引擎目标继承的编译选项，由预设决定是否加严
add_library(taixu_compile_options INTERFACE)
target_compile_features(taixu_compile_options INTERFACE cxx_std_20)
target_compile_options(taixu_compile_options INTERFACE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wnon-virtual-dtor>
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->)

if(TXU_WARNINGS_AS_ERRORS)
    target_compile_options(taixu_compile_options INTERFACE
        $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Werror>
        $<$<CXX_COMPILER_ID:MSVC>:/WX>)
endif()

if(TXU_ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Vulkan 依赖（volk / vk-bootstrap / VMA），见 cmake/TaixuDependencies.cmake
include(cmake/TaixuDependencies.cmake)

add_subdirectory(engine)
add_subdirectory(tools)

if(TXU_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
