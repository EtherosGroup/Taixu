# Vulkan 依赖：volk + vk-bootstrap + VMA
#
# 版本与系统 Vulkan 头 / loader（1.4.341）对齐
# 源码拉取到构建目录（build/<preset>/_deps）

include(FetchContent)

# volk 自带 VK_NO_PROTOTYPES 与全局函数指针
# 直接使用系统 /usr/include/vulkan
set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
FetchContent_Declare(volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG vulkan-sdk-1.4.341.0
    GIT_SHALLOW TRUE)

# vk-bootstrap 需要 Vulkan::Headers 目标。不指定 VK_BOOTSTRAP_VULKAN_HEADER_DIR，
# 让它依次尝试 VulkanHeaders 包、CMake 内置 FindVulkan、最后 FetchContent 拉取
# 对应 tag 的 Vulkan-Headers（绝对版本，不依赖机器路径）
FetchContent_Declare(vk-bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
    GIT_TAG v1.4.341
    GIT_SHALLOW TRUE)

FetchContent_Declare(vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.4.0
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(volk vk-bootstrap vma)

# 第三方头文件产生的告警不参与 -Werror（与 doctest 的处理一致）
# vk-bootstrap 的 VkBootstrap.h 在 -Wshadow 下有告警，属于上游代码
foreach(_txu_dep IN ITEMS volk vk-bootstrap VulkanMemoryAllocator)
    if(TARGET ${_txu_dep})
        get_target_property(_txu_dep_includes ${_txu_dep} INTERFACE_INCLUDE_DIRECTORIES)
        if(_txu_dep_includes)
            set_target_properties(${_txu_dep} PROPERTIES
                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_txu_dep_includes}")
        endif()
    endif()
endforeach()

# 引擎图形层与工具统一链接这个接口目标
add_library(taixu_vulkan INTERFACE)
target_link_libraries(taixu_vulkan INTERFACE
    volk::volk
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator)
