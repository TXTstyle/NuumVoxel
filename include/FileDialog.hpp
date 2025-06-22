#pragma once

#include <SDL.h>
#include "nfd.hpp"
#include "nfd_sdl2.h"

class FileDialog {
  private:
    nfdwindowhandle_t nativeWindow = {};

  public:
    FileDialog();
    FileDialog(FileDialog&&) = default;
    FileDialog(const FileDialog&) = default;
    FileDialog& operator=(FileDialog&&) = default;
    FileDialog& operator=(const FileDialog&) = default;
    ~FileDialog();

    inline void Init(SDL_Window* window) {
        NFD::Init();
        if (window != nullptr)
            NFD_GetNativeWindowFromSDLWindow(window, &nativeWindow);
    }
    inline void Destroy() { NFD::Quit(); }

    int OpenFileDialog(std::string& outPath,
                       const nfdu8filteritem_t* filter = nullptr,
                       const size_t filterCount = 0);
    int SaveFileDialog(std::string& outPath, const std::string& defaultName = "Untitled.nuum",
                       const nfdu8filteritem_t* filter = nullptr,
                       const size_t filterCount = 0);
};
