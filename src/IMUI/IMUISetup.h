#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::IMUISetup
    @brief Dear Imgui wrapper runtime config options
*/
#include "Core/String/String.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Assertion.h"

namespace Oryol {

class IMUISetup {
public:
    /// set optional IniFilename where ImGui will save its settings (default is that ini files are disabled)
    String IniFilename;
    /// set optional LogFilename, same rules as IniFilename (but also see ImGui::LogToFile())
    String LogFilename;
    /// add TTF font from static in-memory data
    void AddFontFromMemory(void* ttf_data, int ttf_size, float font_height);
    void AddFontFromMemory(void* ttf_data, int ttf_size, float font_height, ImFontConfig* config);
    void AddFontFromMemory(void* ttf_data, int ttf_size, float font_height, ImFontConfig* config, const ImWchar* ranges, bool compressed);

    struct fontDesc {
        void* ttf_data = nullptr;
        int ttf_size = 0;
        float font_height = 13;
        const ImWchar* glyph_ranges;
        ImFontConfig* font_config;
        bool compressed = false;
	};
    static const int MaxFonts = 4;
    int numFonts = 0;
    StaticArray<fontDesc, MaxFonts> fonts;
};

//------------------------------------------------------------------------------
inline void
IMUISetup::AddFontFromMemory(void* ttf_data, const int ttf_size, const float font_height) {
    ImFontConfig config;
    AddFontFromMemory(ttf_data, ttf_size, font_height, &config);
}

inline void
IMUISetup::AddFontFromMemory(void* ttf_data, const int ttf_size, const float font_height, ImFontConfig* config) {
    AddFontFromMemory(ttf_data, ttf_size, font_height, config, ImGui::GetIO().Fonts->GetGlyphRangesDefault(), false);
}

inline void
IMUISetup::AddFontFromMemory(void* ttf_data, const int ttf_size, const float font_height, ImFontConfig* config, const ImWchar* ranges, bool compressed = false) {
    o_assert_dbg(ttf_data && (ttf_size > 0));
    fontDesc desc;
    desc.ttf_data = ttf_data;
    desc.ttf_size = ttf_size;
    desc.font_height = font_height;
    desc.glyph_ranges = ranges;
    desc.font_config = config;
    desc.compressed = compressed;

    this->fonts[this->numFonts++] = desc;
}


} // namespace Oryol
