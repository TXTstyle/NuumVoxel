#include "PaletteManager.hpp"
#include <bgfx/bgfx.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <vector>

PaletteManager::PaletteManager() {}

PaletteManager::~PaletteManager() {}

void PaletteManager::Init() {
    // Initialize with a default palette, i.e; "pico-8"
    AddDefualtPalette();
    paletteSize = 16;

    paletteLayout.begin()
        .add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float)
        .end();
    paletteBuffer =
        bgfx::createDynamicVertexBuffer(paletteSize, paletteLayout, 0);
}

void PaletteManager::Destroy() {
    bgfx::destroy(paletteBuffer);
    palettes.clear();
}

void PaletteManager::RenderWindow(bool* open) {
    if (open != nullptr && !*open) {
        return;
    }
    ImGui::Begin("Palettes", open);
    if (ImGui::BeginCombo("Palettes", "Select Palette",
                          ImGuiComboFlags_WidthFitPreview)) {
        for (uint32_t i = 0; i < palettes.size(); ++i) {
            ImGui::PushID(i);
            if (ImGui::Selectable(
                    palettes[i].getName().c_str(),
                    palettes[currentPaletteIndex].getSelectedIndex() == i)) {
                currentPaletteIndex = i;
                shouldUpdate = true;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("New Palette")) {
        AddPalette(Palette("New Palette", 16));
        currentPaletteIndex = palettes.size() - 1;
        shouldUpdate = true;
    }

    ImGui::Separator();
    ImGui::InputText("Name", &palettes[currentPaletteIndex].getName());
    if (ImGui::Button("Delete")) {
        RemovePalette(currentPaletteIndex);
    }

    auto& colors = GetCurrentPalette().getColors();
    for (uint32_t i = 1; i < colors.size(); i++) {
        ImGui::PushID(i);
        // no inputs, no label, alpha preview half, no tooltip
        if (ImGui::ColorEdit4(std::to_string(i).c_str(), &colors[i][0],
                              ImGuiColorEditFlags_NoInputs |
                                  ImGuiColorEditFlags_NoLabel |
                                  ImGuiColorEditFlags_AlphaPreviewHalf |
                                  ImGuiColorEditFlags_NoTooltip)) {
            shouldUpdate = true;
        }
        // Right-click to select color
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
            palettes[currentPaletteIndex].setSelectedColorIndex(i);
            shouldUpdate = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("X")) {
            palettes[currentPaletteIndex].RemoveColor(i);
            shouldUpdate = true;
        }
        ImGui::PopID();
    }
    if (ImGui::Button("+")) {
        palettes[currentPaletteIndex].AddColor({0.0f, 0.0f, 0.0f, 1.0f});
        shouldUpdate = true;
    }
    // selected color
    ImGui::Text("Selected Color: ");
    ImGui::SameLine();
    const glm::vec4& selectedColor =
        palettes[currentPaletteIndex].getSelectedColor();
    ImGui::ColorButton("Selected Color",
                       ImVec4(selectedColor.r, selectedColor.g, selectedColor.b,
                              selectedColor.a),
                       ImGuiColorEditFlags_NoInputs |
                           ImGuiColorEditFlags_NoLabel |
                           ImGuiColorEditFlags_AlphaPreviewHalf);

    ImGui::End();
}

void PaletteManager::UpdateColorData() {
    if (shouldUpdate == false) {
        bgfx::setBuffer(1, paletteBuffer, bgfx::Access::Read);
        return;
    }
    shouldUpdate = false;

    std::vector<glm::vec4>& currentPalette = GetCurrentPalette().getColors();

    if (!bgfx::isValid(paletteBuffer) || currentPalette.size() != paletteSize) {
        if (bgfx::isValid(paletteBuffer)) {
            bgfx::destroy(paletteBuffer);
        }

        paletteSize = currentPalette.size();
        paletteBuffer = bgfx::createDynamicVertexBuffer(
            paletteSize, paletteLayout, BGFX_BUFFER_COMPUTE_READ);
    }

    bgfx::update(paletteBuffer, 0,
                 bgfx::makeRef(currentPalette.data(),
                               currentPalette.size() * sizeof(glm::vec4)));
    bgfx::setBuffer(1, paletteBuffer, bgfx::Access::Read);
}

void PaletteManager::AddDefualtPalette() {
    std::vector<glm::vec4> colors = {
        {0.000, 0.000, 0.000, 1.0}, {0.114, 0.169, 0.325, 1.0},
        {0.494, 0.145, 0.325, 1.0}, {0.000, 0.529, 0.318, 1.0},
        {0.671, 0.322, 0.212, 1.0}, {0.373, 0.341, 0.310, 1.0},
        {0.761, 0.765, 0.780, 1.0}, {1.000, 0.945, 0.910, 1.0},
        {1.000, 0.000, 0.302, 1.0}, {1.000, 0.639, 0.000, 1.0},
        {1.000, 0.925, 0.153, 1.0}, {0.000, 0.894, 0.212, 1.0},
        {0.161, 0.678, 1.000, 1.0}, {0.514, 0.463, 0.612, 1.0},
        {1.000, 0.467, 0.659, 1.0}, {1.000, 0.800, 0.667, 1.0}};
    palettes.emplace_back("Default", std::move(colors));
    currentPaletteIndex = palettes.size() - 1;
    shouldUpdate = true;
}

size_t PaletteManager::AddPalette(Palette palette) {
    palettes.push_back(std::move(palette));
    shouldUpdate = true;
    return palettes.size() - 1;
}

size_t PaletteManager::AddPalette(std::string name,
                                  std::vector<glm::vec4> colors) {
    palettes.emplace_back(std::move(name), std::move(colors));
    shouldUpdate = true;
    return palettes.size() - 1;
}

void PaletteManager::RemovePalette(size_t index) {
    if (index < palettes.size()) {
        palettes.erase(palettes.begin() + index);
        if (currentPaletteIndex >= palettes.size()) {
            currentPaletteIndex = palettes.size() - 1;
        }
        if (palettes.empty()) {
            AddDefualtPalette();
        }
        shouldUpdate = true;
    }
}

void PaletteManager::SetCurrentPalette(size_t index) {
    if (index < palettes.size() && index != currentPaletteIndex) {
        currentPaletteIndex = index;
        shouldUpdate = true;
    }
}
