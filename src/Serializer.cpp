#include "Serializer.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include <fstream>
#include <iostream>
#include <vector>

Serializer::Serializer() {}

Serializer::~Serializer() {}

bool Serializer::Import() {

    // Check if the import path is valid
    if (importPath.empty()) {
        errorText = "Import path is empty!";
        return 1;
    }

    // Attempt to open the file for reading
    std::ifstream file(importPath, std::ios::binary);
    if (!file.is_open()) {
        errorText = "Failed to open file for reading: " + importPath;
        return 1;
    }

    // Read dimensions from the file
    uint16_t w, h, d;
    file.read(reinterpret_cast<char*>(&d), sizeof(uint16_t));
    file.read(reinterpret_cast<char*>(&h), sizeof(uint16_t));
    file.read(reinterpret_cast<char*>(&w), sizeof(uint16_t));

    bool validDimensions =
        (w >= 0 && h >= 0 && d >= 0) && (w < 65535 && h < 65535 && d < 65535);
    if (file.fail() || !validDimensions) {
        errorText = "Failed to read dimensions from file: " + importPath;
        return 1;
    }

    // Set the dimensions
    *width = w;
    *height = h;
    *depth = d;

    // Calculate the size of the voxel data
    voxelData->clear();
    size_t voxelSize = w * h * d;
    voxelData->resize(voxelSize);

    // Read voxel data from the file
    std::vector<uint8_t> intVoxelData(voxelSize);
    file.read(reinterpret_cast<char*>(intVoxelData.data()),
              intVoxelData.size() * sizeof(uint8_t));
    if (file.fail()) {
        errorText = "Failed to read voxel data from file: " + importPath;
        return 1;
    }
    file.close();

    // Convert uint8_t voxel data to float (0.0f - 1.0f)
    for (size_t i = 0; i < voxelSize; ++i) {
        (*voxelData)[i] = static_cast<float>(intVoxelData[i]) / 255.0f;
    }

    // Update the texture
    *textureHandle =
        bgfx::createTexture3D(w, h, d, false, bgfx::TextureFormat::R32F,
                              BGFX_TEXTURE_COMPUTE_WRITE, nullptr);
    const bgfx::Memory* mem =
        bgfx::makeRef(voxelData->data(), voxelData->size() * sizeof(float));
    bgfx::updateTexture3D(*textureHandle, 0, 0, 0, 0, w, h, d, mem);

    return 0;
}

bool Serializer::Export() {

    // Check if the export path is valid
    if (exportPath.empty()) {
        errorText = "Export path is empty!";
        return 1;
    }

    // Attempt to open the file for writing
    std::ofstream file(exportPath);
    if (!file.is_open()) {
        errorText = "Failed to open file for writing: " + exportPath;
        return 1;
    }

    // Read voxel data from the texture
    auto voxelSize = voxelData->size();
    file.write(reinterpret_cast<const char*>(&(*depth)), sizeof(uint16_t));
    file.write(reinterpret_cast<const char*>(&(*height)), sizeof(uint16_t));
    file.write(reinterpret_cast<const char*>(&(*width)), sizeof(uint16_t));

    auto intVoxelData = std::vector<uint8_t>(voxelSize);
    for (size_t i = 0; i < voxelSize; ++i) {
        // Convert float voxel data to uint8_t (0-255)
        intVoxelData[i] = static_cast<uint8_t>((*voxelData)[i] * 255.0f);
    }

    // Write voxel data to the file, binary
    file.write(reinterpret_cast<const char*>(intVoxelData.data()),
               intVoxelData.size() * sizeof(uint8_t));
    if (file.fail()) {
        errorText = "Failed to write voxel data to file: " + exportPath;
        file.close();
        return 1;
    }
    file.close();

    return 0;
}

void Serializer::Init(std::vector<float>* voxelData,
                      bgfx::TextureHandle* textureHandle, uint32_t* width,
                      uint32_t* height, uint32_t* depth) {
    this->voxelData = voxelData;
    this->textureHandle = textureHandle;
    this->width = width;
    this->height = height;
    this->depth = depth;
}

void Serializer::RenderWindow(bool* open) {
    if (!*open)
        return;

    ImGui::Begin("Import/Export", open);

    ImGui::Text("Import/Export Voxel Data");
    ImGui::Separator();

    // Input field for Import Path
    ImGui::InputText("Import path", &importPath);

    if (ImGui::Button("Import")) {
        if (!Import()) {
            resultText = "Import successful!";
        } else {
            resultText = "Import failed!";
        }
        showModal = true;
        currentModalTitle = "Import Result";
    }

    // Input field for Export Path
    ImGui::InputText("Export path", &exportPath);
    if (ImGui::Button("Export")) {
        if (!Export()) {
            resultText = "Export successful!";
        } else {
            resultText = "Export failed!";
        }
        showModal = true;
        currentModalTitle = "Export Result";
    }

    // Generic modal popup
    if (showModal) {
        ImGui::OpenPopup(currentModalTitle.c_str());
    }

    if (ImGui::BeginPopupModal(currentModalTitle.c_str(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::Text("%s", resultText.c_str());
        if (!errorText.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s",
                               errorText.c_str());
        }
        if (ImGui::Button("OK")) {
            showModal = false;
            ImGui::CloseCurrentPopup();
            errorText.clear();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}
