#pragma once

#include "PaletteManager.hpp"
#include "VoxelManager.hpp"
#include "FileDialog.hpp"
#include <array>
#include <string>
#include <glm/glm.hpp>

struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
};

class Serializer {
  private:
    std::string path = "";
    FileDialog fileDialog;

    bool showModal = false;
    std::string logString = "";
    std::string errorText = "";
    SDL_Window* window = nullptr;

    BoundingBox CalculateTriangleBoundingBox(const glm::vec3& v0,
                                             const glm::vec3& v1,
                                             const glm::vec3& v2);
    int LoadObjFile(const std::string& path, BoundingBox& bbox,
                    std::vector<std::array<glm::vec3, 3>>& triangles);

  public:
    Serializer();
    ~Serializer();

    int Import(VoxelManager& voxelManager, PaletteManager& paletteManager);
    int Export(VoxelManager& voxelManager, PaletteManager& paletteManager,
               const bool save = false);
    int ImportFromObj(VoxelManager& voxelManager,
                      PaletteManager& paletteManager);

    void Init(SDL_Window* window);
    void Destroy();
    void RenderWindow();

    std::string& GetPath() { return path; }
};
