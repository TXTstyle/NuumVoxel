#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>

class Palette {
  private:
    std::string name;
    std::vector<glm::vec4> colors;
    uint16_t selectedColorIndex = 0;

  public:
    Palette(std::string name, uint16_t size);
    Palette(std::string name, std::vector<glm::vec4> colors);
    Palette(Palette&&) = default;
    Palette(const Palette&) = default;
    Palette& operator=(Palette&&) = default;
    Palette& operator=(const Palette&) = default;
    ~Palette();

    inline uint16_t AddColor(const glm::vec4& color) {
        colors.push_back(color);
        return colors.size() - 1;
    }

    inline void RemoveColor(uint16_t index) {
        if (index < colors.size()) {
            colors.erase(colors.begin() + index);
            if (selectedColorIndex >= colors.size()) {
                selectedColorIndex = colors.size() - 1;
            }
        }
    }

    inline void setSelectedColorIndex(uint16_t index) {
        if (index < colors.size()) {
            selectedColorIndex = index;
        }
    }
    inline std::string& getName() { return name; }
    inline const std::string& getName() const { return name; }
    inline std::vector<glm::vec4>& getColors() { return colors; }
    inline const glm::vec4& getSelectedColor() const {
        return colors[selectedColorIndex];
    }
    inline const uint16_t getSelectedIndex() const {
        return selectedColorIndex;
    }
};
