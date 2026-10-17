# Windows 构建骨架，未验证
# 目标工具链：MSVC 或 clang-cl

set(CMAKE_SYSTEM_NAME Windows)

if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER clang-cl)
endif()
if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER clang-cl)
endif()

message(FATAL_ERROR
    "windows-msvc 预设尚未验证\n"
    "需要在 Windows 主机上运行，或在 Linux 上配置 clang-cl + Windows SDK\n")
