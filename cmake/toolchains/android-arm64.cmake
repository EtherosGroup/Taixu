# Android 构建骨架，未验证
# 目标：NDK 交叉编译到 arm64-v8a

set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_VERSION 26)
set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)

if(DEFINED ENV{ANDROID_NDK_ROOT})
    set(CMAKE_ANDROID_NDK "$ENV{ANDROID_NDK_ROOT}")
elseif(DEFINED ENV{ANDROID_NDK})
    set(CMAKE_ANDROID_NDK "$ENV{ANDROID_NDK}")
else()
    message(FATAL_ERROR
        "android-arm64 预设尚未验证\n"
        "未发现 NDK：ANDROID_NDK_ROOT 与 ANDROID_NDK 均未设置\n")
endif()
