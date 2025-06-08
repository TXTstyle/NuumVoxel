#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>

class Palette {
  private:
    std::string name;
    std::vector<glm::vec4> colors;
    uint32_t selectedColorIndex = 0;

  public:
    Palette(std::string name, uint32_t size);
    Palette(std::string name, const std::vector<glm::vec4> colors);
    Palette(Palette&&) = default;
    Palette(const Palette&) = default;
    Palette& operator=(Palette&&) = default;
    Palette& operator=(const Palette&) = default;
    ~Palette();

    inline const std::string& getName() const { return name; }
    inline std::vector<glm::vec4>& getColors() { return colors; }
    inline uint32_t getSelectedColor() const { return selectedColorIndex; }
};
