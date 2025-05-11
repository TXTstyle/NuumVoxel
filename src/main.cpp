#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/bx.h>

#include <backends/imgui_impl_sdl2.h>
#include "imgui_impl_bgfx.h"

#include <imgui.h>
#include <iostream>

static void init_bgfx(SDL_Window* window) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo)) {
        std::cerr << "SDL_GetWindowWMInfo failed: " << SDL_GetError()
                  << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }
    bgfx::Init init;

    bgfx::PlatformData pd;
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    if (wmInfo.subsystem == SDL_SYSWM_WAYLAND) {
        init.platformData.type = bgfx::NativeWindowHandleType::Wayland;
        init.platformData.ndt = wmInfo.info.wl.display;
        init.platformData.nwh = wmInfo.info.wl.surface;
    } else {
        init.platformData.type = bgfx::NativeWindowHandleType::Default;
        init.platformData.ndt = wmInfo.info.x11.display;
        init.platformData.nwh = (void*)(uintptr_t)wmInfo.info.x11.window;
    }
    init.type = bgfx::RendererType::Vulkan;
#elif BX_PLATFORM_WINDOWS
    init.platformData.nwh = wmInfo.info.win.window;
    init.type = bgfx::RendererType::Direct3D11;
#elif BX_PLATFORM_OSX
    init.platformData.nwh = wmInfo.info.cocoa.window;
    init.type = bgfx::RendererType::Metal;
#endif

    init.resolution.width = 1280;
    init.resolution.height = 720;
    init.resolution.reset = BGFX_RESET_VSYNC;
    bgfx::renderFrame();
    if (!bgfx::init(init)) {
        std::cerr << "Failed to initialize bgfx!\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }
    bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_PROFILER);
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window =
        SDL_CreateWindow("Nuum", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         1280, 720, SDL_WINDOW_RESIZABLE);

    init_bgfx(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    io.Fonts->AddFontFromFileTTF("assets/fonts/ttf/JetBrainsMono-Regular.ttf", 16.0f);
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;


    ImGui_ImplSDL2_InitForOther(window);
    ImGui_Implbgfx_Init(250);

    int width = 1280, height = 720;

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE &&
                    event.window.windowID == SDL_GetWindowID(window)) {
                    running = false;
                }
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    bgfx::reset(event.window.data1, event.window.data2,
                                BGFX_RESET_VSYNC);
                    bgfx::setViewRect(0, 0, 0, uint16_t(event.window.data1),
                                      uint16_t(event.window.data2));
                    width = event.window.data1;
                    height = event.window.data2;
                }
            }
        }

        ImGui_ImplSDL2_NewFrame();
        ImGui_Implbgfx_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

        ImGui::Begin("Nuum");
        ImGui::Text("Hello from ImGui inside Nuum!");
        ImGui::End();

        ImGui::Render();

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x202020ff,
                           1.0f, 0);
        bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));
        bgfx::touch(0); // make sure view is cleared even if no draw calls
        
        ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

        bgfx::frame();
    }

    ImGui_ImplSDL2_Shutdown();
    ImGui_Implbgfx_Shutdown();
    ImGui::DestroyContext();
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
