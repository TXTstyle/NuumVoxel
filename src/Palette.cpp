#include "Palette.hpp"
#include <imgui.h>

Palette::Palette(std::string name, uint32_t size) {
    this->name = name;
    this->colors.resize(size + 1);
    // Empty space
    colors[0] = {0.0f, 0.0f, 0.0f, 0.0f}; 
    // Fill the rest with a gradient
    for (uint32_t i = 1; i < size+1; ++i) {
        colors[i] = {float(i) / float(size), float(i) / float(size),
                     float(i) / float(size), 1.0f};
    }
}

Palette::~Palette() {}

void Palette::renderWindow() {
    ImGui::Begin(name.c_str());
    for (uint32_t i = 0; i < colors.size(); ++i) {
        ImGui::PushID(i);

        ImGui::PushItemWidth(100);
        ImGui::ColorEdit4("##color", &colors[i][0],
                          ImGuiColorEditFlags_NoInputs |
                              ImGuiColorEditFlags_NoLabel |
                              ImGuiColorEditFlags_NoBorder);
        ImGui::PopID();
        ImGui::PopItemWidth();
    }
    if (ImGui::BeginCombo("##selectedColor", "Color",
                          ImGuiComboFlags_WidthFitPreview)) {
        for (uint32_t i = 0; i < colors.size(); ++i) {
            ImGui::PushID(i);
            if (ImGui::Selectable(std::to_string(i).c_str(),
                                  selectedColorIndex == i)) {
                selectedColorIndex = i;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::Text("Selected Color: %d", selectedColorIndex);
    ImGui::End();
}
