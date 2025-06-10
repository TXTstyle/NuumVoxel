#pragma once

#include "bgfx/bgfx.h"
#include <string>
#include <glm/glm.hpp>
#include <vector>

class Serializer {
  private:
    std::string importPath = "";
    std::string exportPath = "";
    std::vector<float>* voxelData = nullptr;
    bgfx::TextureHandle* textureHandle = nullptr;
    uint32_t *width = nullptr, *height = nullptr, *depth = nullptr;

    bool showModal = false;
    std::string currentModalTitle = "";
    std::string resultText = "";
    std::string errorText = "";

    bool Import();
    bool Export();

  public:
    Serializer();
    ~Serializer();

    void Init(std::vector<float>* voxelData, bgfx::TextureHandle* textureHandle, uint32_t* width,
              uint32_t* height, uint32_t* depth);
    void RenderWindow(bool* open);
};
