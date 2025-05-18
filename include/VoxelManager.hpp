#pragma once

#include <cstdint>
#include <vector>
#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

class VoxelManager {
  private:
    uint32_t width, height, depth;
    std::vector<uint16_t> voxelData;

    const bgfx::Memory* mem;
    uint8_t* voxelArray;
    bgfx::TextureHandle textureHandle;
    bgfx::UniformHandle s_voxelTexture;

  public:
    VoxelManager(uint32_t width, uint32_t height, uint32_t depth);
    VoxelManager(VoxelManager&&) = default;
    VoxelManager(const VoxelManager&) = default;
    VoxelManager& operator=(VoxelManager&&) = default;
    VoxelManager& operator=(const VoxelManager&) = default;
    ~VoxelManager();
    void destroy();

    void setVoxel(uint32_t x, uint32_t y, uint32_t z, uint16_t value);
    uint16_t getVoxel(uint32_t x, uint32_t y, uint32_t z) const;

    void raycastSetVoxel(glm::vec3& rayOrigin, glm::vec3& rayDirection, float voxelSize,
                         uint16_t value);

    void resize(uint32_t newWidth, uint32_t newHeight, uint32_t newDepth);

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
