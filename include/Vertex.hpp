#pragma once

#include <glm/glm.hpp>

struct ScreenVertex {
    glm::vec3 pos;
    glm::vec2 texCoord;

    inline bool operator==(const ScreenVertex& other) const {
        return pos == other.pos && texCoord == other.texCoord;
    }
};
