find_package(SDL3 CONFIG REQUIRED)

add_executable(txu_sdl_probe
    sdl_probe/main.cpp)

target_link_libraries(txu_sdl_probe PRIVATE SDL3::SDL3 taixu_compile_options)

add_executable(txu_vk_probe
    vk_probe/main.cpp)

target_link_libraries(txu_vk_probe PRIVATE taixu_vulkan taixu_compile_options)
