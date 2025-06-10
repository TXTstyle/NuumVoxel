#include "Serializer.hpp"
#include "Palette.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include <fstream>
#include <iostream>
#include <string>
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
    // Read the version number
    uint16_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(uint16_t));
    if (file.fail()) {
        errorText = "Failed to read version from file: " + importPath;
        return 1;
    }
    if (version != 1) {
        errorText = "Unsupported file version: " + std::to_string(version);
        file.close();
        return 1;
    }

    // Read dimensions from the file
    uint16_t w, h, d;
    file.read(reinterpret_cast<char*>(&d), sizeof(uint16_t));
    file.read(reinterpret_cast<char*>(&h), sizeof(uint16_t));
    file.read(reinterpret_cast<char*>(&w), sizeof(uint16_t));

    bool validDimensions =
        (w > 0 && h > 0 && d > 0) && (w < 65535 && h < 65535 && d < 65535);
    if (file.fail() || !validDimensions) {
        errorText = "Failed to read dimensions from file: " + importPath;
        return 1;
    }

    // Read palette data from the file
    std::string paletteName;
    size_t nameLength;
    file.read(reinterpret_cast<char*>(&nameLength), sizeof(size_t));
    if (file.fail() || nameLength == 0) {
        errorText =
            "Failed to read palette name length from file: " + importPath;
        return 1;
    }
    paletteName.resize(nameLength);
    file.read(paletteName.data(), nameLength);
    if (file.fail()) {
        errorText = "Failed to read palette name from file: " + importPath;
        return 1;
    }
    uint16_t selectedColorIndex;
    file.read(reinterpret_cast<char*>(&selectedColorIndex), sizeof(uint16_t));
    if (file.fail()) {
        errorText =
            "Failed to read selected color index from file: " + importPath;
        return 1;
    }
    uint16_t colorCount;
    file.read(reinterpret_cast<char*>(&colorCount), sizeof(uint16_t));
    if (file.fail() || colorCount == 0) {
        errorText = "Failed to read color count from file: " + importPath;
        return 1;
    }
    std::vector<glm::vec4> colors(colorCount);
    for (uint16_t i = 0; i < colorCount; i++) {
        file.read(reinterpret_cast<char*>(&colors[i]), sizeof(glm::vec4));
        if (file.fail()) {
            errorText = "Failed to read color" + std::to_string(i) +
                        " from file: " + importPath;
            return 1;
        }
    }

    // Create a new palette with the read data
    Palette palette(std::move(paletteName), std::move(colors));
    palette.setSelectedColorIndex(selectedColorIndex);
    paletteManager->ClearPalettes(); // Clear existing palettes
    auto index = paletteManager->AddPalette(std::move(palette));
    paletteManager->SetCurrentPalette(index);
    voxelManager->setPalette(&paletteManager->GetCurrentPalette());

    // Set the dimensions
    voxelManager->setSize(w, h, d);

    // Read voxel data from the file
    std::vector<uint8_t> intVoxelData(w * h * d);
    file.read(reinterpret_cast<char*>(intVoxelData.data()),
              intVoxelData.size() * sizeof(uint8_t));
    if (file.fail()) {
        errorText = "Failed to read voxel data from file: " + importPath;
        return 1;
    }
    file.close();

    voxelManager->newVoxelData(intVoxelData, w, h, d);

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
    // Wrtie the version number
    const uint16_t version = 1; // Version number for the file format
    file.write(reinterpret_cast<const char*>(&version), sizeof(uint16_t));

    auto size = voxelManager->getSize();
    std::cout << "Exporting voxel data with dimensions: "
              << size.x << "x" << size.y << "x" << size.z << std::endl;
    uint16_t w = static_cast<uint16_t>(size.x);
    uint16_t h = static_cast<uint16_t>(size.y);
    uint16_t d = static_cast<uint16_t>(size.z);
    // Read voxel data from the texture
    file.write(reinterpret_cast<const char*>(&(w)), sizeof(uint16_t));
    file.write(reinterpret_cast<const char*>(&(h)), sizeof(uint16_t));
    file.write(reinterpret_cast<const char*>(&(d)), sizeof(uint16_t));

    if (file.fail()) {
        errorText = "Failed to write dimensions to file: " + exportPath;
        file.close();
        return 1;
    }

    // Write palette data to the file, in binary, for name, selected color
    // index, and colors
    auto& palette = paletteManager->GetCurrentPalette();
    // Write palette name
    const std::string& paletteName = palette.getName();
    const size_t nameLength = paletteName.size();
    file.write(reinterpret_cast<const char*>(&nameLength), sizeof(size_t));
    file.write(paletteName.c_str(), nameLength);

    if (file.fail()) {
        errorText = "Failed to write palette name to file: " + exportPath;
        file.close();
        return 1;
    }

    // Write selected color index
    const uint16_t selectedColorIndex = palette.getSelectedIndex();
    file.write(reinterpret_cast<const char*>(&selectedColorIndex),
               sizeof(uint16_t));

    if (file.fail()) {
        errorText =
            "Failed to write selected color index to file: " + exportPath;
        file.close();
        return 1;
    }

    // Write colors
    auto& colors = palette.getColors();
    const uint16_t colorCount = colors.size()-1;
    file.write(reinterpret_cast<const char*>(&colorCount), sizeof(uint16_t));
    for (uint16_t i = 0; i < colorCount; i++) {
        file.write(reinterpret_cast<const char*>(&colors[i+1]), sizeof(glm::vec4));
        if (file.fail()) {
            errorText = "Failed to write color to file: " + exportPath;
            file.close();
            return 1;
        }
    }

    // Prepare voxel data for writing
    auto intVoxelData = std::vector<uint8_t>(size.x * size.y * size.z);
    for (size_t i = 0; i < intVoxelData.size(); ++i) {
        // Convert float voxel data to uint8_t (0-255)
        intVoxelData[i] =
            static_cast<uint8_t>(voxelManager->getVoxel()[i] * 255.0f);
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

void Serializer::Init(VoxelManager* voxelManager,
                      PaletteManager* paletteManager) {
    this->voxelManager = voxelManager;
    this->paletteManager = paletteManager;
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
