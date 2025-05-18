#include <SDL.h>
#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_syswm.h>
#include <array>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/bx.h>
#include <bx/math.h>
#include <glm/glm.hpp>

#include <backends/imgui_impl_sdl2.h>
#include "Palette.hpp"
#include "VoxelManager.hpp"
#include "bgfx/defines.h"
#include "glm/geometric.hpp"
#include "glm/matrix.hpp"
#include "glm/trigonometric.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "imgui_impl_bgfx.h"

#include <imgui.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "spirv/fs_screen.sc.bin.h"
#include "spirv/vs_screen.sc.bin.h"
#include "spirv/cs_ray.sc.bin.h"

#if BX_PLATFORM_OSX
#include "metal/vs_screen.sc.bin.h"
#include "metal/fs_screen.sc.bin.h"
#include "metal/cs_ray.sc.bin.h"
#endif

#if BX_PLATFORM_WINDOWS
#include "dx11/vs_screen.sc.bin.h"
#include "dx11/fs_screen.sc.bin.h"
#include "dx11/cs_ray.sc.bin.h"
#endif

struct ScreenVertex {
    glm::vec3 pos;
    glm::vec2 texCoord;

    inline bool operator==(const ScreenVertex& other) const {
        return pos == other.pos && texCoord == other.texCoord;
    }
};

static const ScreenVertex screenVertices[] = {
    {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},  //
    {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},   //
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}, //
    {{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}}};

static const uint16_t screenIndices[] = {0, 1, 2, //
                                         2, 1, 3};

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

    init.resolution.width = 1600;
    init.resolution.height = 900;
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

// Load a texture from file
bgfx::TextureHandle load_texture(const char* filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
    if (!data) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    bgfx::TextureHandle texture = bgfx::createTexture2D(
        uint16_t(width), uint16_t(height), false, 1, bgfx::TextureFormat::RGBA8,
        0, bgfx::copy(data, width * height * 4));
    stbi_image_free(data);
    return texture;
}

void RenderDockingUI() {
    // Setup flags
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    // Disable title bar, borders, etc.
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |=
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("MainDockspace", nullptr, window_flags);
    ImGui::PopStyleVar(2);

    // Create dockspace
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

    ImGui::End();
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    int width = 1600, height = 900;
    SDL_Window* window =
        SDL_CreateWindow("Nuum", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         width, height, SDL_WINDOW_RESIZABLE);

    init_bgfx(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    io.Fonts->AddFontFromFileTTF("assets/fonts/ttf/JetBrainsMono-Regular.ttf",
                                 16.0f);
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL2_InitForOther(window);
    ImGui_Implbgfx_Init(250);

    bgfx::UniformHandle u_texture =
        bgfx::createUniform("u_texture", bgfx::UniformType::Sampler);

    bgfx::VertexLayout screenVertexLayout;
    screenVertexLayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(screenVertices, sizeof(screenVertices)),
        screenVertexLayout);
    bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(
        bgfx::makeRef(screenIndices, sizeof(screenIndices)));

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    bgfx::ProgramHandle program = bgfx::createProgram(
        bgfx::createShader(bgfx::makeRef(vs_screen_spv, sizeof(vs_screen_spv))),
        bgfx::createShader(bgfx::makeRef(fs_screen_spv, sizeof(fs_screen_spv))),
        true);
#elif BX_PLATFORM_WINDOWS
    bgfx::ProgramHandle program = bgfx::createProgram(
        bgfx::createShader(
            bgfx::makeRef(vs_screen_dx11_spv, sizeof(vs_screen_dx11_spv))),
        bgfx::createShader(
            bgfx::makeRef(fs_screen_dx11_spv, sizeof(fs_screen_dx11_spv))),
        true);
#elif BX_PLATFORM_OSX
    bgfx::ProgramHandle program = bgfx::createProgram(
        bgfx::createShader(bgfx::makeRef(vs_screen_mtl, sizeof(vs_screen_mtl))),
        bgfx::createShader(bgfx::makeRef(fs_screen_mtl, sizeof(fs_screen_mtl))),
        true);
#endif

    VoxelManager voxelManager(16, 16, 16);

    bgfx::TextureHandle outputTexture = bgfx::createTexture2D(
        width, height, false, 1, bgfx::TextureFormat::RGBA32F,
        BGFX_TEXTURE_COMPUTE_WRITE);

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    bgfx::ProgramHandle computeProgram = bgfx::createProgram(
        bgfx::createShader(bgfx::makeRef(cs_ray_spv, sizeof(cs_ray_spv))),
        true);
#elif BX_PLATFORM_WINDOWS
    bgfx::ProgramHandle computeProgram = bgfx::createProgram(
        bgfx::createShader(bgfx::makeRef(cs_ray_dx11, sizeof(cs_ray_dx11))),
        true);
#elif BX_PLATFORM_OSX
    bgfx::ProgramHandle computeProgram = bgfx::createProgram(
        bgfx::createShader(bgfx::makeRef(cs_ray_mtl, sizeof(cs_ray_mtl))),
        true);
#endif

    bgfx::UniformHandle u_camPos =
        bgfx::createUniform("u_camPos", bgfx::UniformType::Vec4);
    glm::vec3 camPos = {3.0f, 3.0f, 0.0f};
    bgfx::UniformHandle u_camMat =
        bgfx::createUniform("u_camMat", bgfx::UniformType::Mat3);
    bgfx::UniformHandle u_gridSize =
        bgfx::createUniform("u_gridSize", bgfx::UniformType::Vec4);
    glm::vec4 gridSize = {16.0f, 16.0f, 16.0f, 1.0f};
    float radius = 5.0f;
    // glm::vec2 rotation = {0.66f, 0.89f};
    glm::vec2 rotation = {0.0f, 0.0f};

    float mouseSensitivity = 1.0f;
    float scrollSensitivity = 5.0f;
    ImVec2 avail = {0.0f, 0.0f};
    bool isHoveringViewport = false;
    glm::vec2 viewportMousePos = {0.0f, 0.0f};

    Palette palette("Default", 16);
    voxelManager.setPalette(&palette);

    glm::vec3 target = {0.0f, 1.0f, 0.0f};
    glm::vec3 camF = glm::normalize(target - camPos);
    glm::vec3 camR =
        glm::normalize(glm::cross(camF, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 camU = glm::cross(camR, camF);
    glm::mat3 camMat = glm::mat3(camR, camU, -camF);

    glm::vec3 rayDirection = {0.0f, 0.0f, 0.0f};
    bool mouseDown = false;

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
            if (event.type == SDL_MOUSEMOTION) {
                if (event.motion.xrel == 0 && event.motion.yrel == 0) {
                    continue;
                }
                bool hasNoModifiers =
                    !(SDL_GetModState() &
                      (KMOD_SHIFT | KMOD_CTRL | KMOD_ALT | KMOD_GUI));
                if (event.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT) &&
                    hasNoModifiers && isHoveringViewport &&
                    !ImGui::IsAnyItemActive()) {
                    rotation.x += event.motion.yrel * 0.01f * mouseSensitivity;
                    rotation.y += event.motion.xrel * -0.01f * mouseSensitivity;
                    if (rotation.x > 1.57f) {
                        rotation.x = 1.57f;
                    } else if (rotation.x < -1.57f) {
                        rotation.x = -1.57f;
                    }
                    if (rotation.y > 2 * 3.14f) {
                        rotation.y = 0;
                    } else if (rotation.y < -2 * 3.14f) {
                        rotation.y = 0;
                    }
                }
                if (event.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT) &&
                    SDL_GetModState() & KMOD_CTRL && isHoveringViewport &&
                    !ImGui::IsAnyItemActive()) {
                    glm::vec3 mouseDirWorld = camR * float(-event.motion.xrel) +
                                              camU * float(event.motion.yrel);
                    target += mouseDirWorld * 0.001f * radius;
                }
            }
            if (event.type == SDL_MOUSEWHEEL) {
                if (isHoveringViewport && !ImGui::IsAnyItemActive()) {
                    radius -= event.wheel.y * 0.1f * scrollSensitivity;
                    if (radius < 0.25f) {
                        radius = 0.25f;
                    }
                }
            }
            if (event.button.button == SDL_BUTTON_LEFT && isHoveringViewport &&
                !ImGui::IsAnyItemActive()) {
                if (event.type == SDL_MOUSEBUTTONDOWN) {
                    mouseDown = true;
                }
            }
            if (event.type == SDL_MOUSEBUTTONUP) {
                mouseDown = false;
            }
        }

        if (mouseDown) {
            glm::vec3 rayOrigin = camPos;

            glm::vec2 pixelCoords = viewportMousePos;
            glm::vec2 imageSize = glm::vec2(avail.x, avail.y);

            glm::vec2 rayNDC = (pixelCoords / imageSize) * 2.0f - 1.0f;
            rayNDC.y *= -1.0f;

            float fov = glm::radians(33.0f);
            float aspectRatio = imageSize.x / imageSize.y;
            float scale = tan(fov * 0.5f);

            glm::vec3 rayCameraSpace = {rayNDC.x * scale * aspectRatio,
                                        rayNDC.y * scale, -1.0f};

            rayDirection = glm::normalize(camMat * rayCameraSpace);

            voxelManager.placeVoxelAdjacent(rayOrigin, rayDirection,
                                            gridSize[3], palette.getSelectedColor());
            std::cout << "Raycast dir: " << glm::to_string(rayDirection)
                      << std::endl;
        }

        ImGui_ImplSDL2_NewFrame();
        ImGui_Implbgfx_NewFrame();
        ImGui::NewFrame();

        RenderDockingUI();

        // Set viewport for Nuum window
        ImGui::Begin("Nuum");
        ImGui::SliderFloat2("CamPos", &rotation[0], -1.57f, 1.57f);
        ImGui::SliderFloat("Zoom Sensitivity", &scrollSensitivity, 0.1f, 10.0f);
        ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.1f, 10.0f);
        ImGui::InputFloat3("Grid Size", &gridSize[0]);
        ImGui::SliderFloat("Voxel Size", &gridSize[3], 0.1f, 10.0f);
        ImGui::InputFloat3("View Center", &target[0]);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Mouse Position: %.1f, %.1f", io.MousePos.x, io.MousePos.y);
        ImGui::Text("Viewport Mouse Position: %.1f, %.1f", viewportMousePos.x,
                    viewportMousePos.y);
        ImGui::Text("Is Hovering Viewport: %s",
                    isHoveringViewport ? "true" : "false");
        ImGui::Text("Mouse Down: %s", mouseDown ? "true" : "false");
        ImGui::End();

        ImGui::Begin("Cam debug info");
        ImGui::Text("CamPos: %.1f, %.1f, %.1f", camPos.x, camPos.y, camPos.z);
        ImGui::Text("CamF: %.1f, %.1f, %.1f", camF.x, camF.y, camF.z);
        ImGui::Text("CamR: %.1f, %.1f, %.1f", camR.x, camR.y, camR.z);
        ImGui::Text("CamU: %.1f, %.1f, %.1f", camU.x, camU.y, camU.z);
        ImGui::Text("Viewport Mouse Position: %.1f, %.1f", viewportMousePos.x,
                    viewportMousePos.y);
        ImGui::Text("Viewport Size: %.1f, %.1f", avail.x, avail.y);
        ImGui::Text("Voxel Size: %.1f", gridSize[3]);
        ImGui::Text("Ray Direction: %.1f, %.1f, %.1f", rayDirection.x,
                    rayDirection.y, rayDirection.z);

        ImGui::End();

        palette.renderWindow();

        ImGui::ShowDemoWindow();

        // styple viewport no padding
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport", nullptr,
                     ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoCollapse);

        ImGui::SetWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);

        ImVec2 newAvail = ImGui::GetContentRegionAvail();
        if ((avail.x != newAvail.x || avail.y != newAvail.y) &&
            !ImGui::IsMouseDown(0)) {
            avail = newAvail;
            bgfx::destroy(outputTexture);
            outputTexture = bgfx::createTexture2D(
                uint16_t(avail.x), uint16_t(avail.y), false, 1,
                bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_COMPUTE_WRITE);
        }

        ImGui::Image(outputTexture.idx, avail);
        isHoveringViewport = ImGui::IsWindowHovered();

        if (isHoveringViewport) {
            ImVec2 mousePos = ImGui::GetMousePos();
            viewportMousePos.x = (mousePos.x - ImGui::GetWindowPos().x);
            viewportMousePos.y = (mousePos.y - ImGui::GetWindowPos().y);
        }

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Render();
        ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
        bgfx::touch(0);

        float pitch = rotation.x; // rotation.x = 0
        float yaw = rotation.y;   // rotation.y = 0

        camPos.x = target.x + radius * cos(pitch) * sin(yaw);
        camPos.y = target.y + radius * sin(pitch);
        camPos.z = target.z + radius * cos(pitch) * cos(yaw);

        camF = glm::normalize(target - camPos);
        camR = glm::normalize(glm::cross(camF, glm::vec3(0.0f, 1.0f, 0.0f)));
        camU = glm::cross(camR, camF);
        camMat = glm::mat3(camR, camU, -camF);

        bgfx::setUniform(u_camMat, &glm::transpose(camMat)[0][0], 1);
        bgfx::setUniform(u_gridSize, &gridSize);
        bgfx::setUniform(u_camPos, &glm::vec4(camPos, 1.0f)[0]);
        bgfx::setImage(0, outputTexture, 0, bgfx::Access::Write);
        bgfx::setTexture(1, voxelManager.getVoxelTextureUniform(),
                         voxelManager.getTextureHandle());
        bgfx::dispatch(0, computeProgram, avail.x, avail.y, 1);

        bgfx::frame();
    }

    voxelManager.destroy();
    bgfx::destroy(program);
    bgfx::destroy(ibh);
    bgfx::destroy(vbh);
    bgfx::destroy(u_texture);
    bgfx::destroy(u_camPos);
    bgfx::destroy(u_camMat);
    bgfx::destroy(u_gridSize);
    bgfx::destroy(computeProgram);
    bgfx::destroy(outputTexture);
    ImGui_ImplSDL2_Shutdown();
    ImGui_Implbgfx_Shutdown();
    ImGui::DestroyContext();
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
