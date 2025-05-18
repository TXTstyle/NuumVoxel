#include "VoxelManager.hpp"
#include "bgfx/bgfx.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

VoxelManager::VoxelManager(uint32_t width, uint32_t height, uint32_t depth) {
    this->width = width;
    this->height = height;
    this->depth = depth;

    voxelData.resize(width * height * depth, 0);

    // voxel data for 3D texture
    voxelArray = (uint8_t*)malloc(width * height * depth * 4 * sizeof(uint8_t));
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int index = (z * height * width + y * width + x) * 4;
                voxelArray[index] = (uint8_t)(x * width + y);      // R
                voxelArray[index + 1] = (uint8_t)(y * height + z); // G
                voxelArray[index + 2] = (uint8_t)(z * depth + x);  // B
                voxelArray[index + 3] = 255;                       // A
            }
        }
    }

    mem = bgfx::makeRef(voxelArray, width * height * depth * 4);
    if (!mem) {
        std::cerr << "Failed to allocate memory for voxel texture."
                  << std::endl;
        return;
    }
    // memcpy(mem->data, voxelArray, width * height * depth * 4);
    textureHandle = bgfx::createTexture3D(
        width, height, depth, false, bgfx::TextureFormat::RGBA8, 0, nullptr);
    bgfx::updateTexture3D(textureHandle, 0, 0, 0, 0, width, height, depth, mem);
    s_voxelTexture =
        bgfx::createUniform("s_voxelTexture", bgfx::UniformType::Sampler);

}

VoxelManager::~VoxelManager() {}

void VoxelManager::destroy() {
    if (s_voxelTexture.idx != bgfx::kInvalidHandle) {
        bgfx::destroy(s_voxelTexture);
        s_voxelTexture.idx = bgfx::kInvalidHandle;
    }
    if (textureHandle.idx != bgfx::kInvalidHandle) {
        bgfx::destroy(textureHandle);
        textureHandle.idx = bgfx::kInvalidHandle;
    }
    mem = nullptr;
    if (voxelArray) {
        delete[] voxelArray;
        voxelArray = nullptr;
    }
    voxelData.clear();
}

void VoxelManager::setVoxel(uint32_t x, uint32_t y, uint32_t z,
                            uint16_t value) {
    if (x >= width || y >= height || z >= depth) {
        return; // Out of bounds
    }
    int index = z * width * height + y * width + x;
    voxelData[index] = value;
    mem->data[index * 4 + 0] = 255; // R
    mem->data[index * 4 + 1] = 0;   // G
    mem->data[index * 4 + 2] = 0;   // B
}

uint16_t VoxelManager::getVoxel(uint32_t x, uint32_t y, uint32_t z) const {
    if (x >= width || y >= height || z >= depth) {
        return 0; // Out of bounds
    }
    int index = z * width * height + y * width + x;
    return voxelData[index];
}

void VoxelManager::resize(uint32_t newWidth, uint32_t newHeight,
                          uint32_t newDepth) {}

int DDA3D_HitOffset(const glm::vec3& origin, const glm::vec3& direction,
                    const glm::ivec3& gridSize, const glm::vec3& voxelSize,
                    const glm::vec3& gridOrigin,
                    std::vector<uint16_t>& voxelData) {
    glm::vec3 pos = (origin - gridOrigin) / voxelSize;
    glm::ivec3 voxel = glm::floor(pos);

    glm::ivec3 step = glm::sign(direction);
    glm::vec3 deltaDist = glm::abs(voxelSize / direction);

    glm::vec3 nextVoxelBoundary = glm::vec3(voxel.x + (step.x > 0 ? 1 : 0),
                                            voxel.y + (step.y > 0 ? 1 : 0),
                                            voxel.z + (step.z > 0 ? 1 : 0));

    glm::vec3 tMax = (nextVoxelBoundary - pos) * deltaDist;

    glm::ivec3 lastVoxel = voxel;

    while (voxel.x >= 0 && voxel.x < gridSize.x && voxel.y >= 0 &&
           voxel.y < gridSize.y && voxel.z >= 0 && voxel.z < gridSize.z) {

        int index =
            voxel.z * gridSize.x * gridSize.y + voxel.y * gridSize.x + voxel.x;

        if (voxelData[index] != 0) {
            // Determine normal direction from which we entered the voxel
            glm::ivec3 normal = glm::ivec3(0);
            if (tMax.x < tMax.y && tMax.x < tMax.z)
                normal.x = -step.x;
            else if (tMax.y < tMax.z)
                normal.y = -step.y;
            else
                normal.z = -step.z;

            glm::ivec3 surfaceVoxel = voxel + normal;

            if (surfaceVoxel.x >= 0 && surfaceVoxel.y >= 0 &&
                surfaceVoxel.z >= 0 && surfaceVoxel.x < gridSize.x &&
                surfaceVoxel.y < gridSize.y && surfaceVoxel.z < gridSize.z) {
                return surfaceVoxel.z * gridSize.x * gridSize.y +
                       surfaceVoxel.y * gridSize.x + surfaceVoxel.x;
            } else {
                return -1; // Out of bounds
            }
        }

        lastVoxel = voxel;

        // Advance to next voxel
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            tMax.x += deltaDist.x;
            voxel.x += step.x;
        } else if (tMax.y < tMax.z) {
            tMax.y += deltaDist.y;
            voxel.y += step.y;
        } else {
            tMax.z += deltaDist.z;
            voxel.z += step.z;
        }
    }

    // No hit: return last valid voxel before exit
    if (lastVoxel.x >= 0 && lastVoxel.y >= 0 && lastVoxel.z >= 0 &&
        lastVoxel.x < gridSize.x && lastVoxel.y < gridSize.y &&
        lastVoxel.z < gridSize.z) {
        return lastVoxel.z * gridSize.x * gridSize.y +
               lastVoxel.y * gridSize.x + lastVoxel.x;
    }

    return -1;
}

void setVoxelRGBA(uint8_t* voxelData, uint32_t index, uint32_t size, uint8_t r,
                  uint8_t g, uint8_t b, uint8_t a) {
    if (index < 0 || index >= size) {
        std::cerr << "Index out of bounds: " << index << std::endl;
        return; // Out-of-bounds, do nothing
    }
    voxelData[index] = r;
    voxelData[index + 1] = g;
    voxelData[index + 2] = b;
    voxelData[index + 3] = a;
}

void VoxelManager::raycastSetVoxel(glm::vec3& rayOrigin,
                                   glm::vec3& rayDirection, float voxelSize,
                                   uint16_t value) {
    glm::vec3 gridOrigin(0.0f, 0.0f, 0.0f);
    glm::vec3 gridSize(this->width, this->height, this->depth);
    glm::vec3 voxelSizeVec(voxelSize, voxelSize, voxelSize);

    int hitOffset =
        DDA3D_HitOffset(rayOrigin, rayDirection, glm::ivec3(gridSize),
                        voxelSizeVec, gridOrigin, voxelData);
    if (hitOffset != -1) {
        voxelData[hitOffset] = value;
        std::cout << "Hit voxel at index: " << hitOffset << std::endl;

        // Update the texture with the new voxel data
        // setVoxelRGBA(voxelArray, hitOffset * 4, width*height*depth*4, 255, 0,
        // 0, 255);

        std::cout << "Memory: " << (mem->data == nullptr) << std::endl;
        hitOffset = 0;
        // mem->data[0 * 4 + 0] = 255; // R
        // mem->data[0 * 4 + 1] = 0;   // G
        // mem->data[0 * 4 + 2] = 0;   // B
        // mem->data[0 * 4 + 3] = 255; // A
        voxelArray[0 * 4 + 0] = 255; // R
        voxelArray[0 * 4 + 1] = 0;   // G
        voxelArray[0 * 4 + 2] = 0;   // B
        voxelArray[0 * 4 + 3] = 255; // A
        bgfx::updateTexture3D(textureHandle, 0, 0, 0, 0, width, height, depth,
                              mem);
    }
}
