#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>

// 环境自检：SDL 版本、能否初始化视频子系统、实际使用的 video driver

int main() {
    std::printf("SDL 头文件版本: %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);

    const int runtime = SDL_GetVersion();
    std::printf("SDL 运行库版本: %d.%d.%d\n", SDL_VERSIONNUM_MAJOR(runtime), SDL_VERSIONNUM_MINOR(runtime),
                SDL_VERSIONNUM_MICRO(runtime));

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init(SDL_INIT_VIDEO) 失败: %s\n", SDL_GetError());
        return 1;
    }

    const char* driver = SDL_GetCurrentVideoDriver();
    std::printf("当前 video driver: %s\n", driver != nullptr ? driver : "(null)");
    std::printf("原生 Wayland: %s\n", (driver != nullptr && std::strcmp(driver, "wayland") == 0) ? "是" : "否");

    SDL_Quit();
    return 0;
}
