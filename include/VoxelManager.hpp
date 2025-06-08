#pragma once

#include "Palette.hpp"
#include <cstdint>
#include <vector>
#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

// Placement modes enum
enum class PlacementMode {
    ADJACENT, // Place voxel adjacent to hit voxel
    ON_GRID,  // Place on grid even if no hit
    REPLACE   // Replace the hit voxel
};

class VoxelManager {
  private:
    uint32_t width, height, depth;
    Palette* palette;

    const bgfx::Memory* mem;
    bgfx::TextureHandle textureHandle;
    bgfx::UniformHandle s_voxelTexture;

  public:
    VoxelManager();
    VoxelManager(VoxelManager&&) = default;
    VoxelManager(const VoxelManager&) = default;
    VoxelManager& operator=(VoxelManager&&) = default;
    VoxelManager& operator=(const VoxelManager&) = default;
    ~VoxelManager();
    void Init(uint32_t width, uint32_t height, uint32_t depth);
    void Destroy();

    void setVoxel(uint32_t x, uint32_t y, uint32_t z, uint16_t value);
    uint16_t getVoxel(uint32_t x, uint32_t y, uint32_t z) const;
    glm::vec4 getSize() const {
        return glm::vec4(width, height, depth, 1.0f);
    }

    // void raycastSetVoxel(glm::vec3& rayOrigin, glm::vec3& rayDirection, float voxelSize,
    //                      uint16_t value);
    void raycastSetVoxel(glm::vec3& rayOrigin, glm::vec3& rayDirection, float voxelSize, uint16_t value, PlacementMode mode = PlacementMode::ADJACENT);

    void Resize(uint32_t newWidth, uint32_t newHeight, uint32_t newDepth);

    
    void placeVoxelAdjacent(glm::vec3& rayOrigin, glm::vec3& rayDirection, float voxelSize, uint16_t value);
    void placeVoxelOnGrid(glm::vec3& rayOrigin, glm::vec3& rayDirection, float voxelSize, uint16_t value);
    void replaceVoxel(glm::vec3& rayOrigin, glm::vec3& rayDirection, float voxelSize, uint16_t value);

    inline void setPalette(Palette* newPalette) {
        palette = newPalette;
    }

    inline bgfx::TextureHandle& getTextureHandle() {
        return textureHandle;
    }

    inline bgfx::UniformHandle& getVoxelTextureUniform() {
        return s_voxelTexture;
    }

    inline const bgfx::Memory* getVoxelData() {
        return mem;
    }
};
