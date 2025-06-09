#pragma once

#include "Palette.hpp"
#include <bgfx/bgfx.h>
#include <vector>

class PaletteManager {
private:
    std::vector<Palette> palettes;
    uint32_t currentPaletteIndex;
    uint32_t paletteSize;

    bgfx::DynamicVertexBufferHandle paletteBuffer;
    bgfx::VertexLayout paletteLayout;

public:
    PaletteManager();
    PaletteManager(const PaletteManager&) = delete;
    PaletteManager& operator=(const PaletteManager&) = delete;
    ~PaletteManager();

    void RenderWindow(bool* open);
    void UpdateColorData();

    void Init();
    void Destroy();
    void AddPalette(const Palette& palette);
    void AddPalette(const std::string& name, const std::vector<glm::vec3> colors);
    void RemovePalette(u_int32_t index);
    void SetCurrentPalette(u_int32_t index);

};
