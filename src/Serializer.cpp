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

    if (importPath.find(".nuum") == std::string::npos) {
        importPath += ".nuum"; // Append the default file extension
    }

    // Attempt to open the file for reading
    std::ifstream file(importPath, std::ios::binary);
    if (!file.is_open()) {
        errorText = "Failed to open file: " + importPath;
        return 1;
    }
    resultText = "Importing from: " + importPath + "\n";
    // Read the version number
    uint16_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(uint16_t));
    if (file.fail()) {
        errorText = "Failed to read version";
        return 1;
    }
    if (version != 1) {
        errorText = "Unsupported file version: " + std::to_string(version);
        file.close();
        return 1;
    }
    resultText += "Version: " + std::to_string(version) + "\n";

    // Read dimensions from the file
    uint16_t w, h, d;
    file.read(reinterpret_cast<char*>(&d), sizeof(uint16_t));
    file.read(reinterpret_cast<char*>(&h), sizeof(uint16_t));
    file.read(reinterpret_cast<char*>(&w), sizeof(uint16_t));

    bool validDimensions =
        (w > 0 && h > 0 && d > 0) && (w < 65535 && h < 65535 && d < 65535);
    if (file.fail() || !validDimensions) {
        errorText = "Failed to read dimensions";
        return 1;
    }
    resultText += "Dimensions: " + std::to_string(w) + "x" + std::to_string(h) +
                  "x" + std::to_string(d) + "\n";

    // Read palette data from the file
    std::string paletteName;
    size_t nameLength;
    file.read(reinterpret_cast<char*>(&nameLength), sizeof(size_t));
    if (file.fail() || nameLength == 0) {
        errorText =
            "Failed to read palette name length";
        return 1;
    }
    paletteName.resize(nameLength);
    file.read(paletteName.data(), nameLength);
    if (file.fail()) {
        errorText = "Failed to read palette name";
        return 1;
    }
    resultText += "Palette: " + paletteName + "\n";
    uint16_t selectedColorIndex;
    file.read(reinterpret_cast<char*>(&selectedColorIndex), sizeof(uint16_t));
    if (file.fail()) {
        errorText =
            "Failed to read selected color index";
        return 1;
    }
    resultText +=
        "Selected color index: " + std::to_string(selectedColorIndex) + "\n";
    uint16_t colorCount;
    file.read(reinterpret_cast<char*>(&colorCount), sizeof(uint16_t));
    if (file.fail() || colorCount == 0) {
        errorText = "Failed to read color count";
        return 1;
    }
    resultText += "Color count: " + std::to_string(colorCount) + "\n";
    std::vector<glm::vec4> colors(colorCount);
    for (uint16_t i = 0; i < colorCount; i++) {
        file.read(reinterpret_cast<char*>(&colors[i]), sizeof(glm::vec4));
        if (file.fail()) {
            errorText = "Failed to read color " + std::to_string(i);
            return 1;
        }
    }
    resultText += "Colors read successfully.\n";

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
        errorText = "Failed to read voxel data";
        return 1;
    }
    file.close();
    resultText += "Voxel data read successfully.\n";

    voxelManager->newVoxelData(intVoxelData, w, h, d);

    std::cout << "Import log:\n" << resultText << std::endl;

    return 0;
}

bool Serializer::Export() {

    // Check if the export path is valid
    if (exportPath.empty()) {
        errorText = "Export path is empty!";
        return 1;
    }

    if (exportPath.find(".nuum") == std::string::npos) {
        exportPath += ".nuum"; // Append the default file extension
    }

    // Attempt to open the file for writing
    std::ofstream file(exportPath);
    if (!file.is_open()) {
        errorText = "Failed to open file: " + exportPath;
        return 1;
    }
    resultText = "Exporting to: " + exportPath + "\n";
    // Wrtie the version number
    const uint16_t version = 1; // Version number for the file format
    file.write(reinterpret_cast<const char*>(&version), sizeof(uint16_t));
    resultText += "Version: " + std::to_string(version) + "\n";

    auto size = voxelManager->getSize();
    uint16_t w = static_cast<uint16_t>(size.x);
    uint16_t h = static_cast<uint16_t>(size.y);
    uint16_t d = static_cast<uint16_t>(size.z);
    // Read voxel data from the texture
    file.write(reinterpret_cast<const char*>(&(w)), sizeof(uint16_t));
    file.write(reinterpret_cast<const char*>(&(h)), sizeof(uint16_t));
    file.write(reinterpret_cast<const char*>(&(d)), sizeof(uint16_t));

    if (file.fail()) {
        errorText = "Failed to write dimensions";
        file.close();
        return 1;
    }
    resultText += "Dimensions: " + std::to_string(w) + "x" + std::to_string(h) +
                  "x" + std::to_string(d) + "\n";

    // Write palette data to the file, in binary, for name, selected color
    // index, and colors
    auto& palette = paletteManager->GetCurrentPalette();
    // Write palette name
    const std::string& paletteName = palette.getName();
    const size_t nameLength = paletteName.size();
    file.write(reinterpret_cast<const char*>(&nameLength), sizeof(size_t));
    file.write(paletteName.c_str(), nameLength);

    if (file.fail()) {
        errorText = "Failed to write palette name";
        file.close();
        return 1;
    }
    resultText += "Palette: " + paletteName + "\n";

    // Write selected color index
    const uint16_t selectedColorIndex = palette.getSelectedIndex();
    file.write(reinterpret_cast<const char*>(&selectedColorIndex),
               sizeof(uint16_t));

    if (file.fail()) {
        errorText =
            "Failed to write selected color index";
        file.close();
        return 1;
    }
    resultText +=
        "Selected color index: " + std::to_string(selectedColorIndex) + "\n";

    // Write colors
    auto& colors = palette.getColors();
    const uint16_t colorCount = colors.size() - 1;
    file.write(reinterpret_cast<const char*>(&colorCount), sizeof(uint16_t));
    resultText += "Color count: " + std::to_string(colorCount) + "\n";
    for (uint16_t i = 0; i < colorCount; i++) {
        file.write(reinterpret_cast<const char*>(&colors[i + 1]),
                   sizeof(glm::vec4));
        if (file.fail()) {
            errorText = "Failed to write color " + std::to_string(i);
            file.close();
            return 1;
        }
    }
    resultText += "Colors written successfully.\n";

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
        errorText = "Failed to write voxel data";
        file.close();
        return 1;
    }
    resultText += "Voxel data written successfully.";
    file.close();

    std::cout << "Export log:\n" << resultText << std::endl;

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
            currentModalTitle = "Import successful!";
        } else {
            currentModalTitle = "Import failed!";
        }
        showModal = true;
    }

    // Input field for Export Path
    ImGui::InputText("Export path", &exportPath);
    if (ImGui::Button("Export")) {
        if (!Export()) {
            currentModalTitle = "Export successful!";
        } else {
            currentModalTitle = "Export failed!";
        }
        showModal = true;
    }

    // Generic modal popup
    if (showModal) {
        ImGui::OpenPopup(currentModalTitle.c_str());
    }

    if (ImGui::BeginPopupModal(currentModalTitle.c_str(), nullptr,
                               ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
        // Scrollable text area for result
        ImGui::Text("Log:");
        if (resultText.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s",
                               errorText.c_str());
        } else {
            ImGui::BeginChild("ResultText", ImVec2(220, 150), true,
                              ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextUnformatted(resultText.c_str());
            if (!errorText.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s",
                                   errorText.c_str());
            }
            ImGui::EndChild();
        }
        if (ImGui::Button("OK")) {
            showModal = false;
            ImGui::CloseCurrentPopup();
            errorText.clear();
            resultText.clear();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}
