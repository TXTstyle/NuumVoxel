#include "Nuum.hpp"

#include "bgfx/bgfx.h"
#include "bgfx/platform.h"
#include "bgfx/defines.h"
#include "bx/platform.h"
#include "glm/fwd.hpp"
#include "glm/matrix.hpp"
#include "imgui.h"
#include <SDL_syswm.h>
#include <SDL_video.h>
#include <iostream>
#include "Vertex.hpp"

#include "imgui_impl_sdl2.h"
#include "imgui_impl_bgfx.h"
#include "spirv/vs_ray.sc.bin.h"
#include "spirv/fs_ray.sc.bin.h"

#if BX_PLATFORM_OSX
#include "metal/vs_ray.sc.bin.h"
#include "metal/fs_ray.sc.bin.h"
#endif

#if BX_PLATFORM_WINDOWS
#include "dx11/vs_ray.sc.bin.h"
#include "dx11/fs_ray.sc.bin.h"
#endif

static const ScreenVertex screenVertices[] = {
    {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},  //
    {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},   //
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}, //
    {{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}}};

static const uint16_t screenIndices[] = {0, 1, 2, //
                                         2, 1, 3};

Nuum::Nuum() {}

Nuum::~Nuum() {}

void Nuum::InitBgfx(SDL_Window* window, SDL_SysWMinfo& wmInfo) {
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

    init.resolution.width = width;
    init.resolution.height = height;
    init.resolution.reset = BGFX_RESET_FLIP_AFTER_RENDER;
    bgfx::renderFrame();
    if (!bgfx::init(init)) {
        std::cerr << "Failed to initialize bgfx!\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }
    bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_PROFILER);
}

void Nuum::InitImGui(SDL_Window* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = &ImGui::GetIO();
    ImGui::StyleColorsDark();
    io->Fonts->AddFontFromFileTTF("assets/fonts/ttf/JetBrainsMono-Regular.ttf",
                                  16.0f);
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL2_InitForOther(window);
    ImGui_Implbgfx_Init(250);
}

void Nuum::InitShaders() {
    frameBuffer =
        bgfx::createFrameBuffer((u_int16_t)width, (u_int16_t)height,
                                bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT);

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    program = bgfx::createProgram(
        bgfx::createShader(bgfx::makeRef(vs_ray_spv, sizeof(vs_ray_spv))),
        bgfx::createShader(bgfx::makeRef(fs_ray_spv, sizeof(fs_ray_spv))),
        true);
#elif BX_PLATFORM_WINDOWS
    computeProgram = bgfx::createProgram(
        bgfx::createShader(bgfx::makeRef(vs_ray_dx11, sizeof(vs_ray_dx11))),
        bgfx::createShader(bgfx::makeRef(fs_ray_dx11, sizeof(fs_ray_dx11))),
        true);
#elif BX_PLATFORM_OSX
    computeProgram = bgfx::createProgram(
        bgfx::createShader(bgfx::makeRef(vs_ray_mtl, sizeof(vs_ray_mtl))),
        bgfx::createShader(bgfx::makeRef(fs_ray_mtl, sizeof(fs_ray_mtl))),
        true);
#endif

    u_camPos = bgfx::createUniform("u_camPos", bgfx::UniformType::Vec4);
    u_camMat = bgfx::createUniform("u_camMat", bgfx::UniformType::Mat4);
    u_gridSize = bgfx::createUniform("u_gridSize", bgfx::UniformType::Vec4);

    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    vertexBuffer = bgfx::createVertexBuffer(
        bgfx::makeRef(screenVertices, sizeof(screenVertices)), layout);
    indexBuffer = bgfx::createIndexBuffer(
        bgfx::makeRef(screenIndices, sizeof(screenIndices)));
}

void Nuum::RenderViewport() {
    bgfx::setViewRect(0, 0, 0, bgfx::BackbufferRatio::Equal);
    bgfx::setViewFrameBuffer(0, frameBuffer);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f,
                       0);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA);
    bgfx::setVertexBuffer(0, vertexBuffer);
    bgfx::setIndexBuffer(indexBuffer);
    auto voxelSize = voxelManager.getSize();
    glm::vec4 grid =
        glm::vec4(voxelSize.x, voxelSize.y, voxelSize.z, gridSize[3]);
    bgfx::setUniform(u_gridSize, &grid[0], 1);
    bgfx::setUniform(u_camPos, &glm::vec4(camera.GetPosition(), 1.0f)[0]);
    glm::mat4 invMat = glm::transpose(camera.GetInvViewProj());
    bgfx::setUniform(u_camMat, &invMat[0][0], 1);
    bgfx::setTexture(0, voxelManager.getVoxelTextureUniform(),
                     voxelManager.getTextureHandle());
    bgfx::submit(0, program);
}

void Nuum::RenderViewportWindow() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport", nullptr,
                 ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoCollapse);

    ImGui::SetWindowSize(viewportSize, ImGuiCond_FirstUseEver);

    ImVec2 newViewportSize = ImGui::GetContentRegionAvail();
    if ((viewportSize.x != newViewportSize.x ||
         viewportSize.y != newViewportSize.y) &&
        !ImGui::IsMouseDown(0)) {
        viewportSize = newViewportSize;
        bgfx::destroy(frameBuffer);
        frameBuffer = bgfx::createFrameBuffer(
            (u_int16_t)viewportSize.x, (u_int16_t)viewportSize.y,
            bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT);
        viewportAspectRatio = viewportSize.x / viewportSize.y;
    }

    ImGui::Image(bgfx::getTexture(frameBuffer).idx, viewportSize);

    isHoveringViewport =
        ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
                               ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    if (isHoveringViewport) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 imagePos = ImGui::GetItemRectMin();
        viewportMousePos.x = (mousePos.x - imagePos.x);
        viewportMousePos.y = (mousePos.y - imagePos.y);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void Nuum::RenderDebugWindow() {
    if (!openDebugWindow) {
        return;
    }
    ImGui::Begin("Nuum", &openDebugWindow);
    ImGui::SliderFloat("Zoom Sensitivity", &camera.GetScrollSensitivity(), 0.1f,
                       10.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &camera.GetMouseSensitivity(), 0.1f,
                       10.0f);
    ImGui::InputFloat3("Grid Size", &gridSize[0]);
    if (ImGui::Button("Resize")) {
        voxelManager.Resize(gridSize[0], gridSize[1], gridSize[2]);
    }
    ImGui::SliderFloat("Voxel Size", &gridSize[3], 0.1f, 10.0f);
    if (ImGui::InputFloat("FOV", &camera.GetFov())) {
        camera.SetUpdateState(true);
    }
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Mouse Position: %.1f, %.1f", io->MousePos.x, io->MousePos.y);
    ImGui::Text("Viewport Mouse Position: %.1f, %.1f", viewportMousePos.x,
                viewportMousePos.y);
    ImGui::Text("Is Hovering Viewport: %s",
                isHoveringViewport ? "true" : "false");
    const auto& voxelSize = voxelManager.getSize();
    ImGui::Text("Voxel Size: %.1f, %.1f, %.1f", voxelSize.x, voxelSize.y,
                voxelSize.z);
    ImGui::End();
}

void Nuum::RenderDockspace() {
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
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
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("MainDockspace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // Dockspace
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

    // Menubar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Ctrl+Q")) {
                running = false;
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (!runOnce) {
                    int res =
                        serializer.Export(voxelManager, paletteManager, true);
                    if (res == 0)
                        SDL_SetWindowTitle(
                            window, ("Nuum - " + serializer.GetPath()).c_str());
                    runOnce = true;
                }
            }
            if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
                if (!runOnce) {
                    int res = serializer.Export(voxelManager, paletteManager);
                    if (res == 0)
                        SDL_SetWindowTitle(
                            window, ("Nuum - " + serializer.GetPath()).c_str());
                    runOnce = true;
                }
            }
            if (ImGui::MenuItem("Save As NUPR", nullptr)) {
                if (!runOnce) {
                    int res =
                        serializer.ExportToNUPR(voxelManager, paletteManager);
                    runOnce = true;
                }
            }
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                int res = serializer.Import(voxelManager, paletteManager);
                if (res == 0)
                    SDL_SetWindowTitle(
                        window, ("Nuum - " + serializer.GetPath()).c_str());
            }
            if (ImGui::MenuItem("Open OBJ", "Ctrl+Shift+O")) {
                int res =
                    serializer.ImportFromObj(voxelManager, paletteManager);
                if (res == 0)
                    SDL_SetWindowTitle(
                        window, ("Nuum - " + serializer.GetPath()).c_str());
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Camera", "C", openCameraWindow)) {
                openCameraWindow = !openCameraWindow;
            }
            if (ImGui::MenuItem("Debug", "D", openDebugWindow)) {
                openDebugWindow = !openDebugWindow;
            }
            if (ImGui::MenuItem("Palettes", "P", openPaletteWindow)) {
                openPaletteWindow = !openPaletteWindow;
            }
            if (ImGui::MenuItem("ToolBox", "T", openToolBoxWindow)) {
                openToolBoxWindow = !openToolBoxWindow;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void Nuum::HandleEvents() {
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
        // if imgui wants input skip the rest
        if (ImGui::IsAnyItemActive()) {
            continue;
        }
        // Handle keyboard input
        if (event.type == SDL_KEYUP) {
            if (event.key.keysym.sym == SDLK_q &&
                SDL_GetModState() & KMOD_CTRL) {
                running = false;
                continue;
            }
            if (event.key.keysym.sym == SDLK_c) {
                openCameraWindow = !openCameraWindow;
                continue;
            }
            if (event.key.keysym.sym == SDLK_d) {
                openDebugWindow = !openDebugWindow;
                continue;
            }
            if (event.key.keysym.sym == SDLK_p) {
                openPaletteWindow = !openPaletteWindow;
                continue;
            }
            if (event.key.keysym.sym == SDLK_t) {
                openToolBoxWindow = !openToolBoxWindow;
                continue;
            }
            if (event.key.keysym.sym == SDLK_s &&
                SDL_GetModState() & KMOD_CTRL) {
                if (!runOnce) {
                    int res =
                        serializer.Export(voxelManager, paletteManager, true);
                    if (res == 0)
                        SDL_SetWindowTitle(
                            window, ("Nuum - " + serializer.GetPath()).c_str());
                    runOnce = true;
                }
                continue;
            }
            if (event.key.keysym.sym == SDLK_s &&
                SDL_GetModState() & (KMOD_CTRL | KMOD_SHIFT)) {
                if (!runOnce) {
                    int res = serializer.Export(voxelManager, paletteManager);
                    if (res == 0)
                        SDL_SetWindowTitle(
                            window, ("Nuum - " + serializer.GetPath()).c_str());
                    runOnce = true;
                }
                continue;
            }
            if (event.key.keysym.sym == SDLK_o &&
                SDL_GetModState() & (KMOD_CTRL | KMOD_SHIFT)) {
                if (!runOnce) {
                    int res =
                        serializer.ImportFromObj(voxelManager, paletteManager);
                    if (res == 0)
                        SDL_SetWindowTitle(
                            window, ("Nuum - " + serializer.GetPath()).c_str());
                    runOnce = true;
                }
                continue;
            }
            if (event.key.keysym.sym == SDLK_o &&
                SDL_GetModState() & KMOD_CTRL) {
                if (!runOnce) {
                    int res = serializer.Import(voxelManager, paletteManager);
                    if (res == 0)
                        SDL_SetWindowTitle(
                            window, ("Nuum - " + serializer.GetPath()).c_str());
                    runOnce = true;
                }
                continue;
            }
        }
        // Skip if not hovering viewport
        if (!isHoveringViewport) {
            continue;
        }
        if (event.type == SDL_MOUSEMOTION) {
            camera.HandelMouseMotion(event.motion.state, event.motion.xrel,
                                     event.motion.yrel);
        }
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT && !runOnce) {
                bool hasShiftModifier = SDL_GetModState() & KMOD_SHIFT;
                glm::vec2 viewport =
                    glm::vec2(viewportSize.x, viewportSize.y);

                auto hit = voxelManager.Raycast(
                    viewportMousePos / viewport, camera.GetPosition(),
                    camera.GetInvViewProj(), gridSize[3]);

                if (hit.has_value()) {
                    toolBox.useTool(hit.value(), voxelManager, paletteManager,
                                    hasShiftModifier);
                }
                runOnce = true;
            }
        }
        if (event.type == SDL_MOUSEWHEEL) {
            camera.HandelMouseWheel(event.wheel.y);
        }
    }
}

int Nuum::Init(int argc, char** argv) {
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    // Prefer Wayland over X11 if available, on linux
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland,x11");
#endif

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_DisplayMode displayMode;
    SDL_GetDesktopDisplayMode(0, &displayMode);
    width = width > (displayMode.w * 0.6f) ? displayMode.w * 0.6f : width;
    height = height > (displayMode.h * 0.6f) ? displayMode.h * 0.6f : height;
    viewportAspectRatio = viewportSize.x / viewportSize.y;
    window =
        SDL_CreateWindow("Nuum", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         width, height, SDL_WINDOW_RESIZABLE);

    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo)) {
        std::cerr << "SDL_GetWindowWMInfo failed: " << SDL_GetError()
                  << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    InitBgfx(window, wmInfo);
    InitShaders();
    InitImGui(window);
    viewport = ImGui::GetMainViewport();

    paletteManager.Init();
    voxelManager.Init(64, 64, 64, &paletteManager);
    toolBox.Init();

    SDL_Window* serializerWindow = nullptr;
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    // NativeFileDialog does not support Wayland
    if (wmInfo.subsystem != SDL_SYSWM_WAYLAND) {
        serializerWindow = window;
    }
#else
    serializerWindow = window;
#endif
    serializer.Init(serializerWindow);

    camera.Init();

    return 0;
}

void Nuum::Run() {
    while (running) {
        HandleEvents();

        // ImGui
        ImGui_ImplSDL2_NewFrame();
        ImGui_Implbgfx_NewFrame();
        ImGui::NewFrame();

        RenderDockspace();
        ImGui::ShowDemoWindow();
        RenderViewportWindow();
        camera.RenderDebugWindow(&openCameraWindow);
        RenderDebugWindow();
        paletteManager.RenderWindow(&openPaletteWindow);
        serializer.RenderWindow();
        toolBox.RenderWindow(&openToolBoxWindow);

        ImGui::Render();
        ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

        // Update
        camera.Update(viewportAspectRatio);

        // Render
        bgfx::touch(0);
        paletteManager.UpdateColorData();
        RenderViewport();

        bgfx::frame();
        runOnce = false;
    }
}

void Nuum::Shutdown() {
    serializer.Destroy();
    voxelManager.Destroy();
    paletteManager.Destroy();
    toolBox.Destroy();
    bgfx::destroy(u_camPos);
    bgfx::destroy(u_camMat);
    bgfx::destroy(u_gridSize);
    bgfx::destroy(program);
    bgfx::destroy(vertexBuffer);
    bgfx::destroy(indexBuffer);
    bgfx::destroy(frameBuffer);
    ImGui_ImplSDL2_Shutdown();
    ImGui_Implbgfx_Shutdown();
    ImGui::DestroyContext();
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
}
