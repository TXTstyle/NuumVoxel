#pragma once

#include "Camera.hpp"
#include "VoxelManager.hpp"
#include "imgui.h"
#include <SDL.h>
#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <vector>

class Nuum {
  private:
    SDL_Window* window;
    uint16_t width = 1600, height = 900;
    ImGuiIO* io;
    ImGuiViewport* viewport;

    SDL_Event event;
    bool running = true;

    bool openCameraWindow = true;
    bool openDebugWindow = true;

    bgfx::VertexLayout screenVertexLayout;
    bgfx::VertexBufferHandle vbh;
    bgfx::IndexBufferHandle ibh;
    bgfx::ProgramHandle program;

    bgfx::UniformHandle u_camPos;
    bgfx::UniformHandle u_camMat;
    bgfx::UniformHandle u_gridSize;

    bgfx::ProgramHandle computeProgram;
    bgfx::TextureHandle outputTexture;

    glm::vec4 gridSize = {16.0f, 16.0f, 16.0f, 1.0f};
    glm::vec2 viewportMousePos = {0.0f, 0.0f};
    bool isHoveringViewport;
    ImVec2 viewportSize = ImVec2(width * 0.5f, height * 0.5f);

    Camera camera;
    VoxelManager voxelManager;
    std::vector<Palette> palettes;

    void InitBgfx(SDL_Window* window);
    void InitImGui(SDL_Window* window);
    void InitShaders();

    void RenderViewport();

    // ImGui
    void RenderViewportWindow();
    void HandleEvents();
    void RenderDockspace();
    void RenderDebugWindow();

  public:
    Nuum();
    ~Nuum();

    int Init(int argc, char** argv);
    void Run();
    void Shutdown();
};
