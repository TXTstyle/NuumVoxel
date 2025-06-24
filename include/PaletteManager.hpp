#pragma once

#include "Palette.hpp"
#include <bgfx/bgfx.h>
#include <vector>

class PaletteManager {
private:
    std::vector<Palette> palettes;
    uint32_t currentPaletteIndex;
    int32_t paletteSize;
    bool shouldUpdate = true;

    bgfx::DynamicVertexBufferHandle paletteBuffer;
    bgfx::VertexLayout paletteLayout;

    void AddDefualtPalette();

public:
    PaletteManager();
    PaletteManager(const PaletteManager&) = delete;
    PaletteManager& operator=(const PaletteManager&) = delete;
    ~PaletteManager();

    void RenderWindow(bool* open);
    void UpdateColorData();

    void Init();
    void Destroy();
    size_t AddPalette(Palette palette);
    size_t AddPalette(std::string name, std::vector<glm::vec4> colors);
    void RemovePalette(size_t index);
    void SetCurrentPalette(size_t index);

    inline Palette& GetCurrentPalette() {
        return palettes[currentPaletteIndex];
    }
    inline void ClearPalettes() {
        palettes.clear();
        currentPaletteIndex = 0;
        paletteSize = 0;
    }
};
