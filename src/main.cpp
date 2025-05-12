#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/bx.h>
#include <bx/math.h>
#include <glm/glm.hpp>

#include <backends/imgui_impl_sdl2.h>
#include "bgfx/defines.h"
#include "imgui_impl_bgfx.h"

#include <imgui.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "spirv/fs_screen.sc.bin.h"
#include "spirv/vs_screen.sc.bin.h"
#include "spirv/cs_ray.sc.bin.h"

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
    io.Fonts->AddFontFromFileTTF("assets/fonts/ttf/JetBrainsMono-Regular.ttf",
                                 16.0f);
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL2_InitForOther(window);
    ImGui_Implbgfx_Init(250);

    bgfx::TextureHandle texture = load_texture("assets/brick.jpg");
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

    bgfx::ProgramHandle program = bgfx::createProgram(
        bgfx::createShader(bgfx::makeRef(vs_screen_spv, sizeof(vs_screen_spv))),
        bgfx::createShader(bgfx::makeRef(fs_screen_spv, sizeof(fs_screen_spv))),
        true);

    // voxel data for 3D texture
    uint8_t voxelData[16 * 16 * 16 * 4];
    for (int z = 0; z < 16; ++z) {
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                int index = (z * 16 * 16 + y * 16 + x) * 4;
                voxelData[index] = x * 16 + y;     // R
                voxelData[index + 1] = y * 16 + z; // G
                voxelData[index + 2] = z * 16 + x; // B
                voxelData[index + 3] = 255;        // A
            }
        }
    }

    bgfx::TextureHandle voxelTexture = bgfx::createTexture3D(
        16, 16, 16, false, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT,
        bgfx::copy(voxelData, sizeof(voxelData)));
    bgfx::UniformHandle s_voxelTexture =
        bgfx::createUniform("s_voxelTexture", bgfx::UniformType::Sampler);

    bgfx::TextureHandle outputTexture =
        bgfx::createTexture2D(1280, 720, false, 1, bgfx::TextureFormat::RGBA32F,
                              BGFX_TEXTURE_COMPUTE_WRITE);

    int width = 1280, height = 720;
    const bgfx::Caps* caps = bgfx::getCaps();
    std::cout << "Renderer: " << bgfx::getRendererName(caps->rendererType)
              << std::endl;
    std::cout << "Homogeneous Depth: "
              << (caps->homogeneousDepth ? "true" : "false") << std::endl;

    bgfx::ProgramHandle computeProgram = bgfx::createProgram(
        bgfx::createShader(bgfx::makeRef(cs_ray_spv, sizeof(cs_ray_spv))),
        true);

    bgfx::UniformHandle u_camPos =
        bgfx::createUniform("u_camPos", bgfx::UniformType::Vec4);
    float camPos[4] = {5.0f, 0.0f, 0.0f, 1.0f};
    bgfx::UniformHandle u_viewInv =
        bgfx::createUniform("u_viewInv", bgfx::UniformType::Mat4);
    bgfx::UniformHandle u_projInv =
        bgfx::createUniform("u_projInv", bgfx::UniformType::Mat4);
    bgfx::UniformHandle u_gridSize =
        bgfx::createUniform("u_gridSize", bgfx::UniformType::Vec4);
    float gridSize[4] = {16.0f, 16.0f, 16.0f, 1.0f};

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
                    bgfx::destroy(outputTexture);
                    outputTexture = bgfx::createTexture2D(
                        uint16_t(width), uint16_t(height), false, 1,
                        bgfx::TextureFormat::RGBA32F,
                        BGFX_TEXTURE_COMPUTE_WRITE);
                }
            }
        }

        ImGui_ImplSDL2_NewFrame();
        ImGui_Implbgfx_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Nuum");
        ImGui::Text("Hello from ImGui inside Nuum!");
        ImGui::SliderFloat3("CamPos", &camPos[0], -10.0f, 10.0f);
        ImGui::InputFloat3("Grid Size", &gridSize[0]);
        ImGui::SliderFloat("Voxel Size", &gridSize[3], 0.1f, 10.0f);
        ImGui::End();

        ImGui::Render();
        ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
        bgfx::touch(0);

        float projection[16];
        float view[16];
        bx::Vec3 camPosVec(camPos[0], camPos[1], camPos[2]);
        bx::mtxLookAt(view, camPosVec, bx::Vec3(0.0f, 0.0f, 0.0f));
        float aspect = float(width) / float(height);
        bx::mtxProj(projection, 50.0f, aspect, 1.0f, 100.0f,
                    caps->homogeneousDepth);
        float viewInv[16];
        bx::mtxInverse(viewInv, view);
        bgfx::setUniform(u_viewInv, &viewInv, 1);
        float projInv[16];
        bx::mtxInverse(projInv, projection);
        bgfx::setUniform(u_projInv, &projInv, 1);
        bgfx::setUniform(u_gridSize, &gridSize);
        bgfx::setUniform(u_camPos, &camPos);
        bgfx::setImage(0, outputTexture, 0, bgfx::Access::Write);
        bgfx::setTexture(1, s_voxelTexture, voxelTexture);
        bgfx::dispatch(0, computeProgram, width, height, 1);

        bgfx::touch(1);
        bgfx::setViewClear(1, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x00ff00ff,
                           1.0f, 0);
        bgfx::setViewRect(1, 0, 0, width, height);
        bgfx::setViewFrameBuffer(1, BGFX_INVALID_HANDLE);
        bgfx::setViewClear(1, BGFX_CLEAR_COLOR, 0xff0000ff, 1.0f, 0);
        bgfx::setTexture(0, u_texture, outputTexture);
        bgfx::setVertexBuffer(0, vbh);
        bgfx::setIndexBuffer(ibh);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                       BGFX_STATE_MSAA |
                       BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA,
                                             BGFX_STATE_BLEND_INV_SRC_ALPHA));
        float identity[16];
        bx::mtxIdentity(identity);
        bgfx::setViewTransform(0, identity, identity);
        bgfx::submit(1, program);

        bgfx::dbgTextClear();
        bgfx::dbgTextPrintf(2, 2, 0xf0, "Nuum");

        bgfx::frame();
    }

    bgfx::destroy(program);
    bgfx::destroy(ibh);
    bgfx::destroy(vbh);
    bgfx::destroy(u_texture);
    bgfx::destroy(texture);
    bgfx::destroy(s_voxelTexture);
    bgfx::destroy(voxelTexture);
    bgfx::destroy(u_camPos);
    bgfx::destroy(u_viewInv);
    bgfx::destroy(u_projInv);
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
