#pragma once

#include "Palette.hpp"
#include "PaletteManager.hpp"
#include <cstdint>
#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <vector>

class VoxelManager {
  private:
    uint32_t width, height, depth;
    uint32_t brickWidth, brickHeight, brickDepth;
    std::vector<float> voxelData;
    std::vector<uint8_t> occupancyData;
    PaletteManager* paletteManager;

    bgfx::TextureHandle textureHandle;
    bgfx::UniformHandle s_voxelTexture;

    bgfx::TextureHandle brickTextureHandle;
    bgfx::UniformHandle s_brickVoxelTexture;

    constexpr static uint32_t brickSize = 8;

  public:
    VoxelManager();
    ~VoxelManager();
    void Init(uint32_t width, uint32_t height, uint32_t depth,
              PaletteManager* paletteManager);
    void Destroy();

    void setVoxel(uint32_t x, uint32_t y, uint32_t z, float value);
    uint16_t getVoxel(uint32_t x, uint32_t y, uint32_t z) const;
    const glm::vec4 getSize() const {
        return glm::vec4(width, height, depth, 1.0f);
    }
    inline void setSize(uint32_t newWidth, uint32_t newHeight,
                        uint32_t newDepth) {
        width = newWidth;
        height = newHeight;
        depth = newDepth;
        brickWidth = (width + brickSize - 1) / brickSize;
        brickHeight = (height + brickSize - 1) / brickSize;
        brickDepth = (depth + brickSize - 1) / brickSize;
    }
    void Resize(uint32_t newWidth, uint32_t newHeight, uint32_t newDepth);

    void newVoxelData(std::vector<uint8_t>& newVoxelData, uint32_t w,
                      uint32_t h, uint32_t d);

    void raycastSetVoxel(const glm::vec2& mousePos, const glm::vec2& windowSize,
                         const glm::vec3& rayOrigin, const glm::mat3& camMat,
                         const float voxelScale = 0.0625f,
                         const bool deleteVoxel = false);

    inline bgfx::TextureHandle& getTextureHandle() { return textureHandle; }

    inline bgfx::UniformHandle& getVoxelTextureUniform() {
        return s_voxelTexture;
    }
    inline bgfx::TextureHandle& getBrickTextureHandle() {
        return brickTextureHandle;
    }
    inline bgfx::UniformHandle& getBrickVoxelTextureUniform() {
        return s_brickVoxelTexture;
    }

    inline std::vector<float>& getVoxel() { return voxelData; }

    inline uint32_t* getWidth() { return &width; }
    inline uint32_t* getHeight() { return &height; }
    inline uint32_t* getDepth() { return &depth; }
};
