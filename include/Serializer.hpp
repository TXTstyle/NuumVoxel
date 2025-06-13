#pragma once

#include "PaletteManager.hpp"
#include "VoxelManager.hpp"
#include "FileDialog.hpp"
#include <string>
#include <glm/glm.hpp>

class Serializer {
  private:
    std::string path = "";
    FileDialog fileDialog;

    bool showModal = false;
    std::string logString = "";
    std::string errorText = "";
    SDL_Window* window = nullptr;

  public:
    Serializer();
    ~Serializer();

    int Import(VoxelManager& voxelManager, PaletteManager& paletteManager);
    int Export(VoxelManager& voxelManager, PaletteManager& paletteManager,
               const bool save = false);

    void Init(SDL_Window* window);
    void Destroy();
    void RenderWindow();

    std::string& GetPath() {
        return path;
    }
};
