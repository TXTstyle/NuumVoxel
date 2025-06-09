#include "VoxelManager.hpp"
#include "bgfx/bgfx.h"
#include <cstdlib>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <functional>
#include <iostream>
#include <vector>
#include <bx/bx.h>

#include "bgfx/defines.h"
#include "spirv/cs_resize.sc.bin.h"
#if BX_PLATFORM_WINDOWS
#include "dx11/cs_ray_dx11.sc.bin.h"
#elif BX_PLATFORM_OSX
#include "metal/cs_ray_mtl.sc.bin.h"
#endif

VoxelManager::VoxelManager() {}

VoxelManager::~VoxelManager() {}

void VoxelManager::Init(uint32_t width, uint32_t height, uint32_t depth) {
    this->width = width;
    this->height = height;
    this->depth = depth;

    // voxel data for 3D texture
    float* voxelArray = (float*)calloc(width * height * depth, sizeof(float));
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int index = (z * height * width + y * width + x);
                voxelArray[index] = (index % 17) / 255.0f; // R32F
            }
        }
    }

    mem = bgfx::copy(voxelArray, width * height * depth * sizeof(float));
    if (!mem) {
        std::cerr << "Failed to allocate memory for voxel texture."
                  << std::endl;
        return;
    }
    textureHandle = bgfx::createTexture3D(
        width, height, depth, false, bgfx::TextureFormat::R32F, 0, nullptr);
    bgfx::updateTexture3D(textureHandle, 0, 0, 0, 0, width, height, depth, mem);
    s_voxelTexture =
        bgfx::createUniform("s_voxelTexture", bgfx::UniformType::Sampler);
    // Create a compute shader program for resizing
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    resizeProgram = bgfx::createProgram(
        bgfx::createShader(bgfx::makeRef(cs_resize_spv, sizeof(cs_resize_spv))),
        true);
#elif BX_PLATFORM_WINDOWS
    resizeProgram =
        bgfx::createProgram(bgfx::createShader(bgfx::makeRef(
                                cs_resize_dx11, sizeof(cs_resize_dx11))),
                            true);
#elif BX_PLATFORM_OSX
    resizeProgram = bgfx::createProgram(
        bgfx::createShader(bgfx::makeRef(cs_resize_mtl, sizeof(cs_resize_mtl))),
        true);
#endif
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
    bgfx::destroy(resizeProgram);
    mem = nullptr;
}

void VoxelManager::setVoxel(uint32_t x, uint32_t y, uint32_t z,
                            uint16_t value) {
    if (x >= width || y >= height || z >= depth) {
        return; // Out of bounds
    }
    int index = z * width * height + y * width + x;

    // Create a small memory block for the update
    auto paletteColor = palette->getColors()[value];
    uint8_t voxColor[4] = {};           // RGBA
    voxColor[0] = paletteColor.r * 255; // R
    voxColor[1] = paletteColor.g * 255; // G
    voxColor[2] = paletteColor.b * 255; // B
    std::cout << "Set voxel at: " << x << ", " << y << ", " << z
              << " to color: " << +voxColor[0] << ", " << +voxColor[1] << ", "
              << +voxColor[2] << ", " << +voxColor[3] << std::endl;

    const bgfx::Memory* updateMem = bgfx::copy(voxColor, sizeof(voxColor));

    // Update just the one voxel in the 3D texture
    bgfx::updateTexture3D(textureHandle, 0, x, y, z, 1, 1, 1, updateMem);
}

uint16_t VoxelManager::getVoxel(uint32_t x, uint32_t y, uint32_t z) const {
    if (x >= width || y >= height || z >= depth) {
        return 0; // Out of bounds
    }
    int index = z * width * height + y * width + x;
    return 0; // FIXME: Return the actual voxel value
}

void VoxelManager::Resize(uint32_t newWidth, uint32_t newHeight,
                          uint32_t newDepth) {
    if (newWidth == width && newHeight == height && newDepth == depth) {
        return; // No change in size
    }

    // Create a new texture with the new size
    bgfx::TextureHandle newTexture =
        bgfx::createTexture3D(newWidth, newHeight, newDepth, false,
                              bgfx::TextureFormat::R32F, BGFX_TEXTURE_COMPUTE_WRITE, nullptr);

    // Dispatch the compute shader to resize the voxel data
    bgfx::setImage(0, newTexture, 0, bgfx::Access::Write);
    bgfx::setImage(1, textureHandle, 0, bgfx::Access::Read);
    bgfx::dispatch(1, resizeProgram, newWidth, newHeight, newDepth);

    // Destroy the old texture and update the handle
    bgfx::destroy(textureHandle);
    textureHandle = newTexture;


    width = newWidth;
    height = newHeight;
    depth = newDepth;
}

// Type for voxel sampler: returns true if voxel is hit (non-empty)
using VoxelSampler = std::function<bool(const glm::ivec3& voxel)>;

// Helper function to find the nearest solid voxel within a search radius
bool findNearbyVoxel(const glm::ivec3& centerVoxel, int searchRadius,
                     const glm::ivec3& gridSize, VoxelSampler isVoxelSolid,
                     glm::ivec3& foundVoxel) {
    // Search in non-diagonal directions
    for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
        for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
            for (int dz = -searchRadius; dz <= searchRadius; ++dz) {
                if (std::abs(dx) + std::abs(dy) + std::abs(dz) > searchRadius)
                    continue;

                glm::ivec3 voxel =
                    centerVoxel + glm::ivec3(dx, dy, dz); // Offset voxel

                // Check if the voxel is within bounds
                if (voxel.x >= 0 && voxel.y >= 0 && voxel.z >= 0 &&
                    voxel.x < gridSize.x && voxel.y < gridSize.y &&
                    voxel.z < gridSize.z) {
                    // Check if the voxel is solid
                    if (isVoxelSolid(voxel)) {
                        foundVoxel = voxel;
                        return true; // Found a solid voxel
                    }
                }
            }
        }
    }
    return false;
}

bool raycastVoxelGrid(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                      const glm::vec3& volumeMin, const glm::vec3& volumeMax,
                      const glm::ivec3& gridSize, VoxelSampler isVoxelSolid,
                      glm::ivec3& hitVoxel, glm::vec3& hitPoint,
                      glm::ivec3& hitNormal, int maxSteps = 512) {
    // Handle zero components in ray direction to avoid division by zero
    glm::vec3 invDir;
    invDir.x = (std::abs(rayDir.x) < 1e-6) ? 1e6 * glm::sign(rayDir.x)
                                           : 1.0f / rayDir.x;
    invDir.y = (std::abs(rayDir.y) < 1e-6) ? 1e6 * glm::sign(rayDir.y)
                                           : 1.0f / rayDir.y;
    invDir.z = (std::abs(rayDir.z) < 1e-6) ? 1e6 * glm::sign(rayDir.z)
                                           : 1.0f / rayDir.z;

    // Calculate intersection with AABB
    glm::vec3 t0 = (volumeMin - rayOrigin) * invDir;
    glm::vec3 t1 = (volumeMax - rayOrigin) * invDir;

    // Ensure t0 contains the nearest intersection point
    glm::vec3 tmin = glm::min(t0, t1);
    glm::vec3 tmax = glm::max(t0, t1);

    // Find the furthest near intersection and the closest far intersection
    float tNear = std::max(std::max(tmin.x, tmin.y), tmin.z);
    float tFar = std::min(std::min(tmax.x, tmax.y), tmax.z);

    // Check if ray intersects the volume
    if (tNear > tFar || tFar < 0.0f) {
        return false; // No intersection
    }

    // Initialize the starting point to be at the ray entry point
    float t = std::max(tNear, 0.0f);
    glm::vec3 pos = rayOrigin + rayDir * t;

    // Calculate voxel grid parameters
    glm::vec3 voxelSize = (volumeMax - volumeMin) / glm::vec3(gridSize);

    // Determine which voxel we're in
    glm::ivec3 voxel = glm::floor((pos - volumeMin) / voxelSize);

    // Direction to step through the grid (1 or -1 for each axis)
    glm::ivec3 step = glm::sign(rayDir);

    // Distance along the ray to move one voxel in each direction
    glm::vec3 tDelta = glm::abs(voxelSize * invDir);

    // Distance to the next voxel boundary in each direction
    glm::vec3 nextBoundary =
        volumeMin +
        (glm::vec3(voxel) + glm::max(glm::vec3(0.0f), glm::vec3(step))) *
            voxelSize;
    glm::vec3 tNext = (nextBoundary - rayOrigin) * invDir;

    // Initialize hitNormal to zero
    hitNormal = glm::ivec3(0);

    // Store the first voxel we hit within the grid (even if not solid)
    // This will be used as a fallback for placement
    glm::ivec3 firstGridVoxel = voxel;
    glm::vec3 firstGridPoint = pos;
    bool foundAnyGridVoxel = false;

    // Traverse the grid
    for (int i = 0; i < maxSteps; ++i) {
        // Check if we're inside the grid
        if (voxel.x >= 0 && voxel.y >= 0 && voxel.z >= 0 &&
            voxel.x < gridSize.x && voxel.y < gridSize.y &&
            voxel.z < gridSize.z) {

            // If this is the first voxel we hit within the grid, store it
            if (!foundAnyGridVoxel) {
                firstGridVoxel = voxel;
                firstGridPoint = pos;
                foundAnyGridVoxel = true;
            }

            // Check if current voxel is solid
            if (isVoxelSolid(voxel)) {
                hitVoxel = voxel;
                hitPoint = pos;
                return true; // Hit found
            }

            // Enhanced: Check for nearby solid voxels within a small radius
            glm::ivec3 nearbyVoxel;
            if (findNearbyVoxel(voxel, 1, gridSize, isVoxelSolid,
                                nearbyVoxel)) {
                // Calculate the normal direction (from nearby voxel to current
                // voxel)
                hitNormal = glm::sign(glm::vec3(voxel - nearbyVoxel));
                hitVoxel = nearbyVoxel;
                hitPoint = pos;
                return true; // Nearby hit found
            }
        } else if (foundAnyGridVoxel) {
            // We've exited the grid after being inside it, so we can use the
            // first grid voxel as a fallback
            hitVoxel = firstGridVoxel;
            hitPoint = firstGridPoint;
            // Set normal based on ray direction (opposite to ray direction)
            hitNormal = -glm::sign(rayDir);
            return true;
        }

        // Find the closest boundary
        if (tNext.x < tNext.y && tNext.x < tNext.z) {
            // X-axis crossing
            t = tNext.x;
            pos = rayOrigin + rayDir * t;
            voxel.x += step.x;
            tNext.x += tDelta.x;
            hitNormal = glm::ivec3(-step.x, 0, 0);
        } else if (tNext.y < tNext.z) {
            // Y-axis crossing
            t = tNext.y;
            pos = rayOrigin + rayDir * t;
            voxel.y += step.y;
            tNext.y += tDelta.y;
            hitNormal = glm::ivec3(0, -step.y, 0);
        } else {
            // Z-axis crossing
            t = tNext.z;
            pos = rayOrigin + rayDir * t;
            voxel.z += step.z;
            tNext.z += tDelta.z;
            hitNormal = glm::ivec3(0, 0, -step.z);
        }

        // Check if we've gone beyond the far intersection point
        if (t > tFar) {
            if (foundAnyGridVoxel) {
                // If we've hit any grid voxel, use it as a fallback
                hitVoxel = firstGridVoxel;
                hitPoint = firstGridPoint;
                hitNormal = -glm::sign(rayDir);
                return true;
            }
            return false;
        }
    }

    // If we've hit any grid voxel during traversal, use it as a fallback
    if (foundAnyGridVoxel) {
        hitVoxel = firstGridVoxel;
        hitPoint = firstGridPoint;
        hitNormal = -glm::sign(rayDir);
        return true;
    }

    return false; // Max steps reached, no hit
}

void VoxelManager::raycastSetVoxel(glm::vec3& rayOrigin,
                                   glm::vec3& rayDirection, float voxelSize,
                                   uint16_t value, PlacementMode mode) {
    // Set grid bounds and size
    glm::vec3 gridSize(this->width, this->height, this->depth);
    glm::vec3 gridMin = glm::vec3(-1.0f, 0.0f, -1.0f) * voxelSize;
    glm::vec3 gridMax = glm::vec3(1.0f, 2.0f, 1.0f) * voxelSize;

    glm::ivec3 hitVoxel;
    glm::vec3 hitPoint;
    glm::ivec3 hitNormal;

    // Perform raycasting with enhanced placement capabilities
    bool hit = raycastVoxelGrid(
        rayOrigin, rayDirection, gridMin, gridMax,
        glm::ivec3(width, height, depth),
        [&](const glm::ivec3& voxel) {
            // Check if the voxel is solid (non-empty)
            uint32_t index =
                voxel.x + voxel.y * width + voxel.z * width * height;
            // if (index >= voxelData.size())
            //     return false;
            // return voxelData[index] != 0;
            return true;
        },
        hitVoxel, hitPoint, hitNormal);

    if (hit) {
        // std::cout << "Hit voxel at: " << hitVoxel.x << ", " << hitVoxel.y
        //           << ", " << hitVoxel.z << std::endl;
        // std::cout << "Hit point: " << hitPoint.x << ", " << hitPoint.y << ",
        // "
        //           << hitPoint.z << std::endl;
        // std::cout << "Hit normal: " << hitNormal.x << ", " << hitNormal.y
        //           << ", " << hitNormal.z << std::endl;

        glm::ivec3 targetVoxel;

        // Determine target voxel based on placement mode
        switch (mode) {
        case PlacementMode::ADJACENT:
            // Add a new voxel adjacent to the hit face (using the normal)
            targetVoxel = hitVoxel + hitNormal;
            break;

        case PlacementMode::ON_GRID:
            // Use the hit voxel directly
            targetVoxel = hitVoxel;
            break;

        case PlacementMode::REPLACE:
            // Replace the hit voxel
            targetVoxel = hitVoxel;
            break;
        }

        // Check if the target voxel is within bounds
        if (targetVoxel.x >= 0 && targetVoxel.x < width && targetVoxel.y >= 0 &&
            targetVoxel.y < height && targetVoxel.z >= 0 &&
            targetVoxel.z < depth) {

            // Place the voxel
            setVoxel(targetVoxel.x, targetVoxel.y, targetVoxel.z, value);

            std::cout << "Placed voxel at: " << targetVoxel.x << ", "
                      << targetVoxel.y << ", " << targetVoxel.z << std::endl;
        }
    } else {
        // Fallback: If we can determine a valid grid position, place a voxel
        // there
        glm::vec3 t0 = (gridMin - rayOrigin) / rayDirection;
        glm::vec3 t1 = (gridMax - rayOrigin) / rayDirection;

        // Ensure t0 contains the nearest intersection point
        glm::vec3 tmin = glm::min(t0, t1);
        glm::vec3 tmax = glm::max(t0, t1);

        // Find the furthest near intersection
        float tNear = std::max(std::max(tmin.x, tmin.y), tmin.z);

        if (tNear >= 0) { // Only if we're intersecting the grid
            // Calculate the intersection point
            glm::vec3 intersectionPoint = rayOrigin + rayDirection * tNear;

            // Calculate voxel grid parameters
            glm::vec3 voxelSizeVec =
                (gridMax - gridMin) / glm::vec3(width, height, depth);

            // Determine which voxel we're in
            glm::ivec3 targetVoxel =
                glm::floor((intersectionPoint - gridMin) / voxelSizeVec);

            // Check if within bounds
            if (targetVoxel.x >= 0 && targetVoxel.x < width &&
                targetVoxel.y >= 0 && targetVoxel.y < height &&
                targetVoxel.z >= 0 && targetVoxel.z < depth) {

                // Place the voxel
                setVoxel(targetVoxel.x, targetVoxel.y, targetVoxel.z, value);

                std::cout << "Placed voxel at grid intersection: "
                          << targetVoxel.x << ", " << targetVoxel.y << ", "
                          << targetVoxel.z << std::endl;
            }
        }
    }
}

// Add different placement modes for more flexibility
void VoxelManager::placeVoxelAdjacent(glm::vec3& rayOrigin,
                                      glm::vec3& rayDirection, float voxelSize,
                                      uint16_t value) {
    raycastSetVoxel(rayOrigin, rayDirection, voxelSize, value,
                    PlacementMode::ADJACENT);
}

void VoxelManager::placeVoxelOnGrid(glm::vec3& rayOrigin,
                                    glm::vec3& rayDirection, float voxelSize,
                                    uint16_t value) {
    raycastSetVoxel(rayOrigin, rayDirection, voxelSize, value,
                    PlacementMode::ON_GRID);
}

void VoxelManager::replaceVoxel(glm::vec3& rayOrigin, glm::vec3& rayDirection,
                                float voxelSize, uint16_t value) {
    raycastSetVoxel(rayOrigin, rayDirection, voxelSize, value,
                    PlacementMode::REPLACE);
}
