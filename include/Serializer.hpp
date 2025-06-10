#pragma once

#include "PaletteManager.hpp"
#include "VoxelManager.hpp"
#include <string>
#include <glm/glm.hpp>

class Serializer {
  private:
    std::string importPath = "";
    std::string exportPath = "";
    VoxelManager* voxelManager = nullptr;
    PaletteManager* paletteManager = nullptr;

    bool showModal = false;
    std::string currentModalTitle = "";
    std::string resultText = "";
    std::string errorText = "";

    bool Import();
    bool Export();

  public:
    Serializer();
    ~Serializer();

    void Init(VoxelManager* voxelManager, PaletteManager* paletteManager);
    void RenderWindow(bool* open);
};
