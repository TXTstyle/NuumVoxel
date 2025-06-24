#include "ToolBox.hpp"
#include "glm/fwd.hpp"
#include "imgui.h"
#include <iostream>
#include <vector>

ToolBox::ToolBox() {}

ToolBox::~ToolBox() {}

void ToolBox::Init() {}

void ToolBox::Destroy() {}

int ToolBox::useTool(const HitInfo& hit, VoxelManager& voxelManager,
                     PaletteManager& paletteManager, bool altAction) {
    switch (selectedTool) {
    case 0: // Pencil
        return usePencil(hit, voxelManager, paletteManager, altAction);
    case 1: // Bucket
        return useBucket(hit, voxelManager, paletteManager, altAction);
    case 2: // Brush
        return useBrush(hit, voxelManager, paletteManager, altAction);
    default:
        return -1; // Invalid tool
    }
}

int ToolBox::useBucket(const HitInfo& hit, VoxelManager& voxelManager,
                       PaletteManager& paletteManager, bool altAction) {}

int ToolBox::usePencil(const HitInfo& hit, VoxelManager& voxelManager,
                       PaletteManager& paletteManager, bool altAction) {
    if (altAction) {
        voxelManager.setVoxel(hit.pos.x, hit.pos.y, hit.pos.z, 0.0f);
    } else {
        glm::ivec3 placeVoxel = hit.pos;
        if (!hit.edge)
            placeVoxel += hit.normal;

        voxelManager.setVoxel(placeVoxel.x, placeVoxel.y, placeVoxel.z,
                              hit.value);
    }
    return 0;
}

int ToolBox::useBrush(const HitInfo& hit, VoxelManager& voxelManager,
                      PaletteManager& paletteManager, bool altAction) {
    float color =
        altAction ? -255.0f
                  : static_cast<float>(
                        paletteManager.GetCurrentPalette().getSelectedIndex());
    auto& size = voxelManager.getSize();
    glm::ivec3 center = hit.pos + hit.normal;
    glm::ivec3 start = center - glm::ivec3(brushSide / 2 );
    glm::ivec3 end = center + glm::ivec3(brushSide / 2 + 1);

    // if start or end is out of bounds, clamp them
    start = glm::clamp(start, glm::ivec3(0), glm::ivec3(size) - glm::ivec3(1));
    end = glm::clamp(end, glm::ivec3(0), glm::ivec3(size) - glm::ivec3(1));
    // Calculate length of the brush sides
    glm::ivec3 brushSides = end - start;
    std::vector<float> voxels(brushSides.x * brushSides.y * brushSides.z, 0.0f);
    std::cout << "Brush sides: " << brushSides.x << ", " << brushSides.y << ", "
              << brushSides.z << ", Start: " << start.x << ", " << start.y
              << ", " << start.z << ", End: " << end.x << ", " << end.y << ", "
              << end.z << ", Voxels count:" << voxels.size() << std::endl;

    for (int x = 0; x < brushSides.x; x++) {
        for (int y = 0; y < brushSides.y; y++) {
            for (int z = 0; z < brushSides.z; z++) {
                // Check if inside the shpere with radius brushSide
                glm::vec3 pos = glm::vec3(start) + glm::vec3(x, y, z);
                float distance = glm::length(pos - glm::vec3(center));
                if (distance >= static_cast<float>(brushSide/2.0f)) {
                    continue; // Skip voxels outside the brush radius
                }
                // Calculate the index in the voxel array
                int index = x * brushSides.y * brushSides.z +
                            y * brushSides.z + z;
                // std::cout << "Index: " << index << ", Position: " << pos.x
                //           << ", " << pos.y << ", " << pos.z << std::endl;
                voxels[index] = color / 255.0f;
            }
        }
    }
    // Set the voxels in the voxel manager
    voxelManager.setVoxelAABB(std::move(voxels), start, end);

    return 0;
}

void ToolBox::RenderWindow(bool* open) {
    if (!*open) {
        return;
    }

    ImGui::Begin("ToolBox", open);

    ImGui::Text("Select Tool:");
    for (size_t i = 0; i < toolNames.size(); ++i) {
        if (ImGui::RadioButton(toolNames[i].c_str(), selectedTool == i)) {
            selectedTool = i; // Update selected tool
        }
    }

    ImGui::Separator();
    ImGui::Text("Tool Settings:");
    switch (selectedTool) {
    // case 0: // Pencil
    //     break;
    case 1: // Bucket
        if (ImGui::RadioButton("Fill Color", fillColor)) {
            fillColor = true; // Set to fill color
        }
        if (ImGui::RadioButton("Place Voxels", !fillColor)) {
            fillColor = false; // Set to place voxels
        }
        break;
    case 2: // Brush
        if (ImGui::SliderInt("##BrushSize", &brushSize, 1, 6)) {
            brushSide = brushSize * 2 + 1;
        }
        break;
    default: // No settings for tool
        break;
    }

    ImGui::End();
}
