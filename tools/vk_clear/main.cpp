// Vulkan 清屏：窗口 + surface + swapchain + 2 帧并行提交
//
// - SDL3 在 Wayland 下创建带 Vulkan 支持的窗口
// - SDL_Vulkan_CreateSurface 拿到 VK_KHR_wayland_surface
// - vk-bootstrap 选中带 surface 的物理设备
// - acquire / submit / present 的同步语义正确
//
// 不引入 shader：用 vkCmdClearColorImage 直接清屏

// volk 必须最先包含：它自己定义 VK_NO_PROTOTYPES
#include <volk.h>

#include <VkBootstrap.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

constexpr uint32_t kWindowWidth = 1280;
constexpr uint32_t kWindowHeight = 720;
constexpr int kFramesInFlight = 2;

// 深蓝
constexpr VkClearColorValue kClearColor = {.float32 = {0.06f, 0.10f, 0.18f, 1.0f}};

constexpr VkImageSubresourceRange kColorRange = {
    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .baseMipLevel = 0,
    .levelCount = 1,
    .baseArrayLayer = 0,
    .layerCount = 1};

[[noreturn]] void fail(const char* message) {
    std::fprintf(stderr, "错误：%s\n", message);
    std::exit(1);
}

[[noreturn]] void fail_vk(const char* message, VkResult result) {
    std::fprintf(stderr, "错误：%s（VkResult = %d）\n", message, static_cast<int>(result));
    std::exit(1);
}

void check_vk(VkResult result, const char* message) {
    if (result != VK_SUCCESS) {
        fail_vk(message, result);
    }
}

// 把交换链图像从 UNDEFINED 转到 TRANSFER_DST，清屏，再转到 PRESENT_SRC
void record_clear(VkCommandBuffer command_buffer, VkImage image) {
    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = kColorRange};

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);

    vkCmdClearColorImage(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &kClearColor, 1, &kColorRange);

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = 0;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);
}

}  // namespace

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fail(SDL_GetError());
    }

    SDL_Window* window =
        SDL_CreateWindow("Taixu - Vulkan clear", static_cast<int>(kWindowWidth), static_cast<int>(kWindowHeight),
                         SDL_WINDOW_VULKAN);
    if (window == nullptr) {
        fail(SDL_GetError());
    }

    if (volkInitialize() != VK_SUCCESS) {
        fail("volkInitialize 失败：找不到 Vulkan loader");
    }

    // instance：扩展列表由 SDL 按当前 video driver 提供
    uint32_t extension_count = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extension_count);
    if (extensions == nullptr) {
        fail(SDL_GetError());
    }

    vkb::InstanceBuilder instance_builder;
    instance_builder.set_app_name("taixu-vk-clear")
        .set_engine_name("taixu")
        .require_api_version(1, 4, 0)
        .request_validation_layers()
        .use_default_debug_messenger();

    for (uint32_t i = 0; i < extension_count; ++i) {
        instance_builder.enable_extension(extensions[i]);
    }

    auto instance_ret = instance_builder.build();
    if (!instance_ret) {
        std::fprintf(stderr, "创建 instance 失败: %s\n", instance_ret.error().message().c_str());
        return 1;
    }

    vkb::Instance instance = instance_ret.value();
    volkLoadInstance(instance);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        fail(SDL_GetError());
    }

    // 物理设备：这次有 surface，vk-bootstrap 会检查队列是否支持呈现
    auto physical_ret = vkb::PhysicalDeviceSelector{instance}
                            .set_surface(surface)
                            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                            .select();
    if (!physical_ret) {
        std::fprintf(stderr, "选择物理设备失败: %s\n", physical_ret.error().message().c_str());
        return 1;
    }

    auto device_ret = vkb::DeviceBuilder{physical_ret.value()}.build();
    if (!device_ret) {
        std::fprintf(stderr, "创建逻辑设备失败: %s\n", device_ret.error().message().c_str());
        return 1;
    }

    vkb::Device device = device_ret.value();
    volkLoadDevice(device);

    auto swapchain_ret = vkb::SwapchainBuilder{device, surface}
                             .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                             .build();
    if (!swapchain_ret) {
        std::fprintf(stderr, "创建交换链失败: %s\n", swapchain_ret.error().message().c_str());
        return 1;
    }

    vkb::Swapchain swapchain = swapchain_ret.value();

    auto images_ret = swapchain.get_images();
    auto views_ret = swapchain.get_image_views();
    if (!images_ret || !views_ret) {
        fail("获取交换链图像失败");
    }

    std::vector<VkImage> images = images_ret.value();
    std::vector<VkImageView> views = views_ret.value();
    const size_t image_count = images.size();

    auto queue_index_ret = device.get_queue_index(vkb::QueueType::graphics);
    auto queue_ret = device.get_queue(vkb::QueueType::graphics);
    auto present_queue_ret = device.get_queue(vkb::QueueType::present);
    if (!queue_index_ret || !queue_ret || !present_queue_ret) {
        fail("获取队列失败");
    }

    VkQueue graphics_queue = queue_ret.value();
    VkQueue present_queue = present_queue_ret.value();

    VkCommandPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_index_ret.value()};

    VkCommandPool command_pool = VK_NULL_HANDLE;
    check_vk(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool), "创建命令池");

    VkCommandBufferAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = kFramesInFlight};

    std::vector<VkCommandBuffer> command_buffers(kFramesInFlight);
    check_vk(vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()), "分配命令缓冲");

    std::vector<VkSemaphore> image_available(kFramesInFlight);
    std::vector<VkFence> in_flight(kFramesInFlight);
    // render_finished 按交换链图像数量分配：图像在 present 完成前不会被再次 acquire，
    // 这样复用信号量是安全的
    std::vector<VkSemaphore> render_finished(image_count);

    VkSemaphoreCreateInfo semaphore_info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr,
                                         .flags = 0};
    VkFenceCreateInfo fence_info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = nullptr,
                                 .flags = VK_FENCE_CREATE_SIGNALED_BIT};

    for (int i = 0; i < kFramesInFlight; ++i) {
        check_vk(vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available[static_cast<size_t>(i)]),
                 "创建 image_available 信号量");
        check_vk(vkCreateFence(device, &fence_info, nullptr, &in_flight[static_cast<size_t>(i)]), "创建 fence");
    }
    for (size_t i = 0; i < image_count; ++i) {
        check_vk(vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished[i]), "创建 render_finished 信号量");
    }

    std::printf("交换链：%u 张图像，%ux%u\n", static_cast<unsigned>(image_count), swapchain.extent.width,
                swapchain.extent.height);

    bool running = true;
    uint32_t frame = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }
        if (!running) {
            break;
        }

        const size_t slot = frame;

        check_vk(vkWaitForFences(device, 1, &in_flight[slot], VK_TRUE, UINT64_MAX), "等待 fence");

        uint32_t image_index = 0;
        const VkResult acquire =
            vkAcquireNextImageKHR(device, swapchain.swapchain, UINT64_MAX, image_available[slot], VK_NULL_HANDLE,
                                  &image_index);
        if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
            fail("交换链过期（窗口尺寸变化）；当前版本不支持重建");
        }
        if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
            fail_vk("获取交换链图像", acquire);
        }

        check_vk(vkResetFences(device, 1, &in_flight[slot]), "重置 fence");
        check_vk(vkResetCommandBuffer(command_buffers[slot], 0), "重置命令缓冲");

        VkCommandBufferBeginInfo begin_info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .pNext = nullptr,
                                            .flags = 0, .pInheritanceInfo = nullptr};
        check_vk(vkBeginCommandBuffer(command_buffers[slot], &begin_info), "开始记录命令");

        record_clear(command_buffers[slot], images[image_index]);

        check_vk(vkEndCommandBuffer(command_buffers[slot]), "结束记录命令");

        VkSemaphoreSubmitInfo wait_info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                        .pNext = nullptr,
                                        .semaphore = image_available[slot],
                                        .value = 0,
                                        .stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                        .deviceIndex = 0};

        VkSemaphoreSubmitInfo signal_info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                          .pNext = nullptr,
                                          .semaphore = render_finished[image_index],
                                          .value = 0,
                                          .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                          .deviceIndex = 0};

        VkCommandBufferSubmitInfo command_buffer_info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                                                      .pNext = nullptr,
                                                      .commandBuffer = command_buffers[slot],
                                                      .deviceMask = 0};

        VkSubmitInfo2 submit_info{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                                  .pNext = nullptr,
                                  .flags = 0,
                                  .waitSemaphoreInfoCount = 1,
                                  .pWaitSemaphoreInfos = &wait_info,
                                  .commandBufferInfoCount = 1,
                                  .pCommandBufferInfos = &command_buffer_info,
                                  .signalSemaphoreInfoCount = 1,
                                  .pSignalSemaphoreInfos = &signal_info};

        check_vk(vkQueueSubmit2(graphics_queue, 1, &submit_info, in_flight[slot]), "提交命令");

        VkPresentInfoKHR present_info{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                      .pNext = nullptr,
                                      .waitSemaphoreCount = 1,
                                      .pWaitSemaphores = &render_finished[image_index],
                                      .swapchainCount = 1,
                                      .pSwapchains = &swapchain.swapchain,
                                      .pImageIndices = &image_index,
                                      .pResults = nullptr};

        const VkResult present = vkQueuePresentKHR(present_queue, &present_info);
        if (present == VK_ERROR_OUT_OF_DATE_KHR) {
            fail("交换链过期（窗口尺寸变化）；当前版本不支持重建");
        }
        if (present != VK_SUCCESS && present != VK_SUBOPTIMAL_KHR) {
            fail_vk("呈现", present);
        }

        frame = (frame + 1) % static_cast<uint32_t>(kFramesInFlight);
    }

    check_vk(vkDeviceWaitIdle(device), "等待设备空闲");

    for (int i = 0; i < kFramesInFlight; ++i) {
        vkDestroySemaphore(device, image_available[static_cast<size_t>(i)], nullptr);
        vkDestroyFence(device, in_flight[static_cast<size_t>(i)], nullptr);
    }
    for (size_t i = 0; i < image_count; ++i) {
        vkDestroySemaphore(device, render_finished[i], nullptr);
    }

    vkDestroyCommandPool(device, command_pool, nullptr);

    swapchain.destroy_image_views(views);
    vkb::destroy_swapchain(swapchain);
    vkb::destroy_device(device);
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
    vkb::destroy_instance(instance);

    SDL_DestroyWindow(window);
    SDL_Quit();

    std::printf("结果：成功\n");
    return 0;
}
