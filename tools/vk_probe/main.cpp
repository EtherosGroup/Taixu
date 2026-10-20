// Vulkan 探针：验证 volk + vk-bootstrap 能创建 instance 并选中物理设备

// volk 必须最先包含：它自己定义 VK_NO_PROTOTYPES
#include <volk.h>

#include <VkBootstrap.h>

#include <cstdio>

namespace {

void print_version(const char* label, uint32_t version) {
    std::printf("%s: %u.%u.%u\n", label, VK_API_VERSION_MAJOR(version), VK_API_VERSION_MINOR(version),
                VK_API_VERSION_PATCH(version));
}

}  // namespace

int main() {
    if (volkInitialize() != VK_SUCCESS) {
        std::fprintf(stderr, "volkInitialize 失败：找不到 Vulkan loader\n");
        return 1;
    }

    print_version("Vulkan loader", volkGetInstanceVersion());
    std::fflush(stdout);

    vkb::InstanceBuilder builder;
    auto instance_ret = builder.set_app_name("taixu-vk-probe")
                            .set_engine_name("taixu")
                            .require_api_version(1, 4, 0)
                            .request_validation_layers()
                            .use_default_debug_messenger()
                            .build();

    if (!instance_ret) {
        std::fprintf(stderr, "创建 instance 失败: %s\n", instance_ret.error().message().c_str());
        return 1;
    }

    vkb::Instance instance = instance_ret.value();
    volkLoadInstance(instance);

    vkb::PhysicalDeviceSelector selector{instance};
    // 还没有 surface，先跳过与显示相关的选择条件
    auto physical_ret = selector.defer_surface_initialization()
                            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                            .select();

    if (!physical_ret) {
        std::fprintf(stderr, "选择物理设备失败: %s\n", physical_ret.error().message().c_str());
        vkb::destroy_instance(instance);
        return 1;
    }

    vkb::PhysicalDevice physical = physical_ret.value();

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical.physical_device, &properties);

    std::printf("GPU: %s\n", properties.deviceName);
    print_version("设备 API", properties.apiVersion);
    print_version("驱动版本", properties.driverVersion);

    vkb::destroy_instance(instance);
    std::printf("结果: 成功\n");
    return 0;
}
