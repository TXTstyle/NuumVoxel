#include "VoxelManager.hpp"
#include "bgfx/bgfx.h"
#include "glm/common.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/vector_relational.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <optional>
#include <vector>
#include <bx/bx.h>

VoxelManager::VoxelManager() {}

VoxelManager::~VoxelManager() {}

void VoxelManager::Init(uint32_t width, uint32_t height, uint32_t depth,
                        PaletteManager* paletteManager) {
    this->width = width;
    this->height = height;
    this->depth = depth;
    this->brickWidth = (width + brickSize - 1) / brickSize;
    this->brickHeight = (height + brickSize - 1) / brickSize;
    this->brickDepth = (depth + brickSize - 1) / brickSize;
    this->paletteManager = paletteManager;

    // voxel data for 3D texture
    voxelData.resize(width * height * depth, 0);
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int index = (z * height * width + y * width + x);
                if (x < width / 3) {
                    voxelData[index] = (index % 17) / 255.0f; // R32F
                } else {
                    voxelData[index] = 0.0f;
                }
            }
        }
    }

    // occupancy data for 3D texture
    occupancyData.resize(brickWidth * brickHeight * brickDepth, 0);

    for (int z = 0; z < brickDepth; ++z) {
        for (int y = 0; y < brickHeight; ++y) {
            for (int x = 0; x < brickWidth; ++x) {
                bool occupied = false;

                for (int bz = 0; bz < brickSize && !occupied; ++bz) {
                    for (int by = 0; by < brickSize && !occupied; ++by) {
                        for (int bx = 0; bx < brickSize && !occupied; ++bx) {
                            int voxelX = x * brickSize + bx;
                            int voxelY = y * brickSize + by;
                            int voxelZ = z * brickSize + bz;

                            if (voxelX < width && voxelY < height &&
                                voxelZ < depth) {
                                int index = (voxelZ * height * width +
                                             voxelY * width + voxelX);
                                if (voxelData[index] > 0.003f) { // 1/255
                                    occupied = true;
                                }
                            }
                        }
                    }
                }

                occupancyData[z * brickHeight * brickWidth + y * brickWidth +
                              x] = occupied ? 255 : 0;
            }
        }
    }

    const bgfx::Memory* mem =
        bgfx::makeRef(voxelData.data(), width * height * depth * sizeof(float));
    if (!mem) {
        std::cerr << "Failed to allocate memory for voxel texture."
                  << std::endl;
        return;
    }

    const bgfx::Memory* occupancyMem = bgfx::makeRef(
        occupancyData.data(), brickWidth * brickHeight * brickDepth);
    if (!occupancyMem) {
        std::cerr << "Failed to allocate memory for occupancy texture."
                  << std::endl;
        return;
    }

    textureHandle = bgfx::createTexture3D(
        width, height, depth, false, bgfx::TextureFormat::R32F, 0, nullptr);
    bgfx::updateTexture3D(textureHandle, 0, 0, 0, 0, width, height, depth, mem);
    s_voxelTexture =
        bgfx::createUniform("s_voxelTexture", bgfx::UniformType::Sampler);

    brickTextureHandle = bgfx::createTexture3D(
        brickWidth, brickHeight, brickDepth, false, bgfx::TextureFormat::R8,
        BGFX_TEXTURE_COMPUTE_WRITE | BGFX_SAMPLER_UVW_CLAMP, nullptr);
    s_brickVoxelTexture =
        bgfx::createUniform("s_brickVoxelTexture", bgfx::UniformType::Sampler);
    bgfx::updateTexture3D(brickTextureHandle, 0, 0, 0, 0, brickWidth,
                          brickHeight, brickDepth, occupancyMem);
}

void VoxelManager::Destroy() {
    if (s_voxelTexture.idx != bgfx::kInvalidHandle) {
        bgfx::destroy(s_voxelTexture);
        s_voxelTexture.idx = bgfx::kInvalidHandle;
    }
    if (textureHandle.idx != bgfx::kInvalidHandle) {
        bgfx::destroy(textureHandle);
        textureHandle.idx = bgfx::kInvalidHandle;
    }
    if (s_brickVoxelTexture.idx != bgfx::kInvalidHandle) {
        bgfx::destroy(s_brickVoxelTexture);
        s_brickVoxelTexture.idx = bgfx::kInvalidHandle;
    }
    if (brickTextureHandle.idx != bgfx::kInvalidHandle) {
        bgfx::destroy(brickTextureHandle);
        brickTextureHandle.idx = bgfx::kInvalidHandle;
    }
}

void VoxelManager::setVoxel(uint32_t x, uint32_t y, uint32_t z, float value) {
    if (x >= width || y >= height || z >= depth) {
        return; // Out of bounds
    }
    int index = z * width * height + y * width + x;
    voxelData[index] = value;

    float voxColor = static_cast<float>(value);
    const bgfx::Memory* updateMem = bgfx::copy(&voxColor, sizeof(voxColor));

    // Update just the one voxel in the 3D texture
    bgfx::updateTexture3D(textureHandle, 0, x, y, z, 1, 1, 1, updateMem);

    // Update the occupancy data
    int brickX = x / brickSize;
    int brickY = y / brickSize;
    int brickZ = z / brickSize;
    int brickIndex =
        brickZ * brickHeight * brickWidth + brickY * brickWidth + brickX;
    // if the voxel is in unoccupied brick, mark it as occupied, vice versa
    if (value > 0.003f && !occupancyData[brickIndex]) {
        occupancyData[brickIndex] = 255; // Mark as occupied
        uint8_t occupancyValue = occupancyData[brickIndex];
        const bgfx::Memory* occupancyUpdateMem =
            bgfx::copy(&occupancyValue, sizeof(uint8_t));
        bgfx::updateTexture3D(brickTextureHandle, 0, brickX, brickY, brickZ, 1,
                              1, 1, occupancyUpdateMem);
    } else if (value <= 0.003f && occupancyData[brickIndex]) {
        // check if last voxel in brick
        for (int bz = 0; bz < brickSize; ++bz) {
            for (int by = 0; by < brickSize; ++by) {
                for (int bx = 0; bx < brickSize; ++bx) {
                    int voxelX = brickX * brickSize + bx;
                    int voxelY = brickY * brickSize + by;
                    int voxelZ = brickZ * brickSize + bz;

                    if (voxelX < width && voxelY < height && voxelZ < depth) {
                        int index =
                            (voxelZ * height * width + voxelY * width + voxelX);
                        if (voxelData[index] > 0.003f) {
                            return; // Still occupied
                        }
                    }
                }
            }
        }
        occupancyData[brickIndex] = 0; // Mark as unoccupied
        uint8_t occupancyValue = occupancyData[brickIndex];
        const bgfx::Memory* occupancyUpdateMem =
            bgfx::copy(&occupancyValue, sizeof(uint8_t));
        bgfx::updateTexture3D(brickTextureHandle, 0, brickX, brickY, brickZ, 1,
                              1, 1, occupancyUpdateMem);
    }
}

void VoxelManager::setVoxelAABB(const std::vector<float> data,
                                const glm::ivec3& aabbMin,
                                const glm::ivec3& aabbMax) {
    if (data.size() != (aabbMax.x - aabbMin.x) * (aabbMax.y - aabbMin.y) *
                           (aabbMax.z - aabbMin.z)) {
        std::cerr << "Data size does not match AABB dimensions." << std::endl;
        std::cerr << "Expected: "
                  << (aabbMax.x - aabbMin.x) * (aabbMax.y - aabbMin.y) *
                         (aabbMax.z - aabbMin.z)
                  << ", Got: " << data.size() << std::endl;
        return; // Size mismatch
    }
    // iterate through the AABB and set voxels
    for (int x = aabbMin.x; x < aabbMax.x; x++) {
        for (int y = aabbMin.y; y < aabbMax.y; y++) {
            for (int z = aabbMin.z; z < aabbMax.z; z++) {
                int index = z * width * height + y * width + x;
                if (index < 0 || index >= voxelData.size()) {
                    std::cerr << "Index out of bounds: " << index << std::endl;
                    continue; // Out of bounds
                }
                uint32_t newVoxelIndex =
                    (z - aabbMin.z) * (aabbMax.y - aabbMin.y) *
                        (aabbMax.x - aabbMin.x) +
                    (y - aabbMin.y) * (aabbMax.x - aabbMin.x) + (x - aabbMin.x);
                if (voxelData[index] < 0.003f && voxelData[index] > -0.003f) {
                    voxelData[index] = data[newVoxelIndex];
                    // std::cout << "Setting voxel at (" << x << ", " << y << ", "
                    //           << z << ") to " << data[newVoxelIndex]
                    //           << std::endl;
                } else if (data[newVoxelIndex] == -1.0f) {
                    // If the color is -1.0f, remove the voxel
                    voxelData[index] = 0.0f;
                }
            }
        }
    }

    // Update the texture with the new voxel data
    // Create a cutout of the voxel data for the specified AABB
    int w = aabbMax.x - aabbMin.x;
    int h = aabbMax.y - aabbMin.y;
    int d = aabbMax.z - aabbMin.z;

    std::cout << "Updating AABB: "
              << "Min: (" << aabbMin.x << ", " << aabbMin.y << ", " << aabbMin.z
              << "), Max: (" << aabbMax.x << ", " << aabbMax.y << ", "
              << aabbMax.z << "), Size: (" << w << ", " << h << ", " << d << ")"
              << std::endl;

    // Basic bounds checking
    assert(aabbMin.x >= 0 && aabbMax.x <= width);
    assert(aabbMin.y >= 0 && aabbMax.y <= height);
    assert(aabbMin.z >= 0 && aabbMax.z <= depth);

    std::vector<float> cutout(w * h * d);

    for (int z = 0; z < d; z++) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                size_t src_index = (x + aabbMin.x) + (y + aabbMin.y) * width +
                                   (z + aabbMin.z) * width * height;
                size_t dst_index = x + y * w + z * w * h;
                cutout[dst_index] = voxelData[src_index];
                std::cout << "Cutout voxel at (" << x << ", " << y << ", " << z
                          << ") from source index " << src_index
                          << " to destination index " << dst_index
                          << " with value: " << cutout[dst_index] << std::endl;
            }
        }
    }

    // Update the 3D texture with the new cutout data
    const bgfx::Memory* mem =
        bgfx::makeRef(cutout.data(), cutout.size() * sizeof(float));
    if (!mem) {
        std::cerr << "Failed to allocate memory for voxel texture update."
                  << std::endl;
        return;
    }
    // std::cout << "Updating texture with cutout data of size: "
    //           << cutout.size() * sizeof(float) << " bytes." << std::endl;
    bgfx::updateTexture3D(textureHandle, 0, aabbMin.x, aabbMin.y, aabbMin.z, w,
                          h, d, mem);

    return;

    // Update the occupancy data for the bricks affected by the AABB
    glm::ivec3 aabbMinBrick = aabbMin / static_cast<int>(brickSize);
    glm::ivec3 aabbMaxBrick =
        (aabbMax + glm::ivec3(brickSize - 1)) / static_cast<int>(brickSize);

    int aabbBrickWidth = aabbMaxBrick.x - aabbMinBrick.x;
    int aabbBrickHeight = aabbMaxBrick.y - aabbMinBrick.y;
    int aabbBrickDepth = aabbMaxBrick.z - aabbMinBrick.z;
    std::vector<uint8_t> occupancySub(
        aabbBrickWidth * aabbBrickHeight * aabbBrickDepth, 0);

    std::cout << "Updating occupancy data for AABB: "
              << "Min: (" << aabbMinBrick.x << ", " << aabbMinBrick.y << ", "
              << aabbMinBrick.z << "), Max: (" << aabbMaxBrick.x << ", "
              << aabbMaxBrick.y << ", " << aabbMaxBrick.z << "), Size: ("
              << aabbBrickWidth << ", " << aabbBrickHeight << ", "
              << aabbBrickDepth << ")" << std::endl;

    for (int z = aabbMinBrick.z;
         z < std::min(aabbMaxBrick.z, static_cast<int>(brickDepth)); ++z) {
        for (int y = aabbMinBrick.y;
             y < std::min(aabbMaxBrick.y, static_cast<int>(brickHeight)); ++y) {
            for (int x = aabbMinBrick.x;
                 x < std::min(aabbMaxBrick.x, static_cast<int>(brickWidth));
                 ++x) {
                if (x >= brickWidth || y >= brickHeight || z >= brickDepth) {
                    std::cerr << "Brick index out of bounds: (" << x << ", "
                              << y << ", " << z << ")" << std::endl;
                    continue; // Out of bounds
                }
                int brickIndex =
                    z * brickDepth * brickHeight + y * brickWidth + x;
                bool occupied = false;

                for (int bz = 0; bz < brickSize && !occupied; ++bz) {
                    for (int by = 0; by < brickSize && !occupied; ++by) {
                        for (int bx = 0; bx < brickSize && !occupied; ++bx) {
                            int voxelX = x * brickSize + bx;
                            int voxelY = y * brickSize + by;
                            int voxelZ = z * brickSize + bz;

                            if (voxelX >= width || voxelY >= height ||
                                voxelZ >= depth)
                                continue;

                            int index = (voxelZ * height * width +
                                         voxelY * width + voxelX);
                            if (voxelData[index] > 0.003f) { // 1/255
                                occupied = true;
                            }
                        }
                    }
                }

                occupancyData[brickIndex] = occupied ? 255 : 0;

                int localZ = z - aabbMinBrick.z;
                int localY = y - aabbMinBrick.y;
                int localX = x - aabbMinBrick.x;

                occupancySub[localZ * aabbBrickHeight * aabbBrickWidth +
                             localY * aabbBrickWidth + localX] =
                    occupancyData[brickIndex];
            }
        }
    }

    // Update the brick texture with the new occupancy data
    // For only the affected bricks
    const bgfx::Memory* occupancyMem = bgfx::makeRef(
        occupancySub.data(), occupancySub.size() * sizeof(uint8_t));
    if (!occupancyMem) {
        std::cerr << "Failed to allocate memory for occupancy texture update."
                  << std::endl;
        return;
    }
    // bgfx::updateTexture3D(brickTextureHandle, 0, aabbMinBrick.x,
    // aabbMinBrick.y,
    //                       aabbMinBrick.z, aabbBrickWidth, aabbBrickHeight,
    //                       aabbBrickDepth, occupancyMem);
}

uint16_t VoxelManager::getVoxel(uint32_t x, uint32_t y, uint32_t z) const {
    if (x >= width || y >= height || z >= depth) {
        return 0; // Out of bounds
    }
    int index = z * width * height + y * width + x;
    return 0; // FIXME: Return the actual voxel value
}

void VoxelManager::newVoxelData(std::vector<uint8_t>& newVoxelData, uint32_t w,
                                uint32_t h, uint32_t d) {
    if (newVoxelData.size() != w * h * d) {
        std::cerr << "New voxel data size does not match specified dimensions."
                  << std::endl;
        return; // Size mismatch
    }
    // Resize the voxel data vector to match the new dimensions
    voxelData.clear();
    voxelData.resize(w * h * d, 0.0f);

    // Convert uint8_t voxel data to float (0.0f - 1.0f)
    for (size_t i = 0; i < newVoxelData.size(); ++i) {
        voxelData[i] = static_cast<float>(newVoxelData[i]) / 255.0f;
    }

    // Update the texture
    if (textureHandle.idx != bgfx::kInvalidHandle) {
        bgfx::destroy(textureHandle); // Destroy old texture if it exists
    }
    textureHandle =
        bgfx::createTexture3D(w, h, d, false, bgfx::TextureFormat::R32F,
                              BGFX_TEXTURE_COMPUTE_WRITE, nullptr);
    const bgfx::Memory* mem =
        bgfx::makeRef(voxelData.data(), voxelData.size() * sizeof(float));
    bgfx::updateTexture3D(textureHandle, 0, 0, 0, 0, w, h, d, mem);
}

void VoxelManager::Resize(uint32_t newWidth, uint32_t newHeight,
                          uint32_t newDepth) {
    if (newWidth == width && newHeight == height && newDepth == depth) {
        return; // No change in size
    }

    // Resize the voxel data vector, without
    std::vector<float> newVoxelData(newWidth * newHeight * newDepth, 0.0f);
    // Copy existing data into the new vector
    for (uint32_t z = 0; z < std::min(depth, newDepth); ++z) {
        for (uint32_t y = 0; y < std::min(height, newHeight); ++y) {
            for (uint32_t x = 0; x < std::min(width, newWidth); ++x) {
                int oldIndex = z * width * height + y * width + x;
                int newIndex = z * newWidth * newHeight + y * newWidth + x;
                newVoxelData[newIndex] = voxelData[oldIndex];
            }
        }
    }
    voxelData = std::move(newVoxelData);

    const bgfx::Memory* newMem = bgfx::makeRef(
        voxelData.data(), newWidth * newHeight * newDepth * sizeof(float));

    // Destroy the old texture and update the handle
    bgfx::destroy(textureHandle);
    textureHandle = bgfx::createTexture3D(newWidth, newHeight, newDepth, false,
                                          bgfx::TextureFormat::R32F,
                                          BGFX_TEXTURE_COMPUTE_WRITE, nullptr);
    bgfx::updateTexture3D(textureHandle, 0, 0, 0, 0, newWidth, newHeight,
                          newDepth, newMem);

    width = newWidth;
    height = newHeight;
    depth = newDepth;

    // Update brick dimensions
    brickWidth = (width + brickSize - 1) / brickSize;
    brickHeight = (height + brickSize - 1) / brickSize;
    brickDepth = (depth + brickSize - 1) / brickSize;

    // Resize the occupancy data vector
    occupancyData.resize(brickWidth * brickHeight * brickDepth, 0);

    // Recalculate occupancy data
    for (int z = 0; z < brickDepth; ++z) {
        for (int y = 0; y < brickHeight; ++y) {
            for (int x = 0; x < brickWidth; ++x) {
                bool occupied = false;

                for (int bz = 0; bz < brickSize && !occupied; ++bz) {
                    for (int by = 0; by < brickSize && !occupied; ++by) {
                        for (int bx = 0; bx < brickSize && !occupied; ++bx) {
                            int voxelX = x * brickSize + bx;
                            int voxelY = y * brickSize + by;
                            int voxelZ = z * brickSize + bz;

                            if (voxelX < width && voxelY < height &&
                                voxelZ < depth) {
                                int index = (voxelZ * height * width +
                                             voxelY * width + voxelX);
                                if (voxelData[index] > 0.003f) { // 1/255
                                    occupied = true;
                                }
                            }
                        }
                    }
                }

                occupancyData[z * brickHeight * brickWidth + y * brickWidth +
                              x] = occupied ? 255 : 0;
            }
        }
    }

    // Update the brick texture
    if (brickTextureHandle.idx != bgfx::kInvalidHandle) {
        bgfx::destroy(brickTextureHandle); // Destroy old brick texture
    }
    const bgfx::Memory* occupancyMem = bgfx::makeRef(
        occupancyData.data(), brickWidth * brickHeight * brickDepth);
    brickTextureHandle = bgfx::createTexture3D(
        brickWidth, brickHeight, brickDepth, false, bgfx::TextureFormat::R8,
        BGFX_TEXTURE_COMPUTE_WRITE | BGFX_SAMPLER_UVW_CLAMP, nullptr);
    bgfx::updateTexture3D(brickTextureHandle, 0, 0, 0, 0, brickWidth,
                          brickHeight, brickDepth, occupancyMem);
}

std::optional<HitInfo> VoxelManager::Raycast(const glm::vec2& mousePos,
                                             const glm::vec2& windowSize,
                                             const glm::vec3& rayOrigin,
                                             const glm::mat3& camMat,
                                             const float voxelScale) {
    // Set grid bounds and size
    glm::vec3 gridSize(this->width, this->height, this->depth);
    const glm::vec3 gridMin =
        glm::vec3(-1.0f, 0.0f, -1.0f) * gridSize * voxelScale * 1.0f / 16.0f;
    const glm::vec3 gridMax =
        glm::vec3(1.0f, 2.0f, 1.0f) * gridSize * voxelScale * 1.0f / 16.0f;

    // Calculate ray direction from camera matrix
    float fov = 45.0f;
    glm::vec2 uv = ((mousePos + glm::vec2(0.5f)) / windowSize) * 2.0f - 1.0f;
    uv.y = -uv.y;
    float aspectRatio = windowSize.x / windowSize.y;
    float scale = glm::tan(glm::radians(fov) * 0.5f);
    uv.x *= scale * aspectRatio;
    uv.y *= scale;

    glm::vec3 rayDirCam = glm::normalize(glm::vec3(uv.x, uv.y, -1.0f));
    glm::vec3 rayDir = glm::normalize(camMat * rayDirCam);

    glm::vec3 invDir = 1.0f / rayDir;
    glm::vec3 t0 = (gridMin - rayOrigin) * invDir;
    glm::vec3 t1 = (gridMax - rayOrigin) * invDir;
    glm::vec3 tsmaller = glm::min(t0, t1);
    glm::vec3 tbigger = glm::max(t0, t1);
    float tmin =
        std::max(0.0f, std::max(tsmaller.x, std::max(tsmaller.y, tsmaller.z)));
    float tmax = std::min(tbigger.x, std::min(tbigger.y, tbigger.z));

    if (tmax < tmin) {
        return std::nullopt; // No intersection
    }

    const float epsilon = 0.00001f; // Small offset to avoid precision issues

    // Calculate the starting position of the ray
    glm::vec3 startPos = rayOrigin + rayDir * (tmin + epsilon);

    glm::vec3 voxelSize = (gridMax - gridMin) / gridSize;

    glm::ivec3 voxel = glm::ivec3(glm::floor((startPos - gridMin) / voxelSize));

    // DDA setup
    glm::ivec3 step = glm::sign(rayDir);
    glm::vec3 tDelta = glm::abs(voxelSize / rayDir);

    glm::vec3 voxelBoundary(0.0f);
    for (int i = 0; i < 3; ++i) {
        voxelBoundary[i] =
            gridMin[i] + (rayDir[i] > 0 ? (voxel[i] + 1) * voxelSize[i]
                                        : voxel[i] * voxelSize[i]);
    }

    glm::vec3 tMax = (voxelBoundary - rayOrigin) / rayDir;

    glm::ivec3 lastVoxel = voxel;
    int lastAxis = -1;
    const int maxSteps = 256;
    for (int i = 0; i < maxSteps; ++i) {
        if (glm::any(glm::lessThan(voxel, glm::ivec3(0))) ||
            glm::any(glm::greaterThanEqual(voxel, glm::ivec3(gridSize)))) {
            break; // Out of bounds
        }

        size_t index = voxel.z * width * height + voxel.y * width + voxel.x;
        if (voxelData[index] > 0.003f) {
            glm::ivec3 normal(0);
            if (lastAxis == 0)
                normal.x = -step.x;
            else if (lastAxis == 1)
                normal.y = -step.y;
            else if (lastAxis == 2)
                normal.z = -step.z;

            float colIndex = static_cast<float>(
                paletteManager->GetCurrentPalette().getSelectedIndex());

            HitInfo info;
            info.pos = voxel;
            info.normal = normal;
            info.value = colIndex / 255.0f;
            info.edge = false;
            return std::make_optional(info);
        }
        lastVoxel = voxel;

        // Step to next voxel and record last axis stepped
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            voxel.x += step.x;
            tMax.x += tDelta.x;
            lastAxis = 0;
        } else if (tMax.y < tMax.z) {
            voxel.y += step.y;
            tMax.y += tDelta.y;
            lastAxis = 1;
        } else {
            voxel.z += step.z;
            tMax.z += tDelta.z;
            lastAxis = 2;
        }
    }
    // Check if lastVoxel is valid and not out of bounds
    if (glm::all(glm::greaterThanEqual(lastVoxel, glm::ivec3(0))) &&
        glm::all(glm::lessThan(lastVoxel, glm::ivec3(width, height, depth)))) {
        float colIndex = static_cast<float>(
            paletteManager->GetCurrentPalette().getSelectedIndex());

        glm::ivec3 normal(0);
        if (lastAxis == 0)
            normal.x = -step.x;
        else if (lastAxis == 1)
            normal.y = -step.y;
        else if (lastAxis == 2)
            normal.z = -step.z;

        HitInfo info;
        info.normal = normal;
        info.pos = lastVoxel;
        info.value = colIndex / 255.0f;
        info.edge = true;
        return std::make_optional(info);
    }

    return std::nullopt; // No hit found
}
