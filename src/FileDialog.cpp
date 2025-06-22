#include "FileDialog.hpp"
#include "nfd.hpp"
#include <iostream>

FileDialog::FileDialog() {}

FileDialog::~FileDialog() {}

int FileDialog::OpenFileDialog(std::string& outPath, const nfdu8filteritem_t* filter,
                               const size_t filterCount) {
    nfdu8char_t* outPathCStr = nullptr;
    nfdresult_t result =
        NFD::OpenDialog(outPathCStr, filter, filterCount, ".", nativeWindow);
    if (result == NFD_CANCEL) {
        outPath = "";
        return 2;
    } else if (result == NFD_ERROR) {
        return 1;
    }
    outPath = std::string(outPathCStr);
    NFD::FreePath(outPathCStr);
    return 0;
}

int FileDialog::SaveFileDialog(std::string& outPath, const std::string& defaultName, const nfdu8filteritem_t* filter,
                               const size_t filterCount) {
    nfdu8char_t* outPathCStr = nullptr;
    nfdresult_t result = NFD::SaveDialog(outPathCStr, filter, filterCount, ".",
                                         defaultName.c_str(), nativeWindow);
    if (result == NFD_CANCEL) {
        outPath = "";
        return 2;
    } else if (result == NFD_ERROR) {
        return 1;
    }
    outPath = std::string(outPathCStr);
    NFD::FreePath(outPathCStr);
    return 0;
}
