#pragma once

#include "PaletteManager.hpp"
#include "VoxelManager.hpp"
#include <array>
#include <cstddef>

class ToolBox {
  private:
    std::array<std::string, 3> toolNames = {"Pencil", "Bucket", "Brush"};
    size_t selectedTool = 0;

    int brushSize = 4;
    int brushSide = brushSize * 2 + 1;
    bool fillColor = false;

    int useBucket(const HitInfo& hit, VoxelManager& voxelManager, PaletteManager& paletteManager,
                  bool altAction = false);
    int usePencil(const HitInfo& hit, VoxelManager& voxelManager, PaletteManager& paletteManager,
                  bool altAction = false);
    int useBrush(const HitInfo& hit, VoxelManager& voxelManager, PaletteManager& paletteManager,
                 bool altAction = false);

  public:
    ToolBox();
    ~ToolBox();

    void Init();
    void Destroy();

    int useTool(const HitInfo& hit, VoxelManager& voxelManager, PaletteManager& paletteManager,
                bool altAction = false);

    void RenderWindow(bool* open);
};
