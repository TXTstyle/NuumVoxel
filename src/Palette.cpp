#include "Palette.hpp"
#include <imgui.h>

Palette::Palette(std::string name, uint16_t size) {
    this->name = name;
    this->colors.resize(size + 1);
    // Empty space
    colors[0] = {0.0f, 0.0f, 0.0f, 0.0f};
    // Fill the rest with a gradient
    for (uint32_t i = 1; i < size + 1; ++i) {
        colors[i] = {float(i) / float(size), float(i) / float(size),
                     float(i) / float(size), 1.0f};
    }
}

Palette::Palette(std::string name, std::vector<glm::vec4> colors) {
    this->name = name;
    this->colors = colors;
    if (this->colors.empty()) {
        this->colors.resize(1, {0.0f, 0.0f, 0.0f, 0.0f}); // Empty space
    } else {
        this->colors.insert(this->colors.begin(),
                            {0.0f, 0.0f, 0.0f, 0.0f}); // Empty space
    }
}

Palette::~Palette() {}
