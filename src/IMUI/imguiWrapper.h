#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::imguiWrapper
    @brief imgui wrapper class for Oryol
*/
#include "Core/Types.h"
#include "Core/Containers/StaticArray.h"
#include "Gfx/Gfx.h"
#include "imgui.h"
#include "IMUI/IMUISetup.h"

namespace Oryol {
namespace _priv {

class imguiWrapper {
public:
    /// setup the wrapper
    void Setup(const IMUISetup& setup);
    /// discard the wrapper
    void Discard();
    /// return true if wrapper is valid
    bool IsValid() const;
    /// call before issuing ImGui commands
    void NewFrame(float frameDurationInSeconds);
    /// grab a new ImTextureID
    ImTextureID AllocImage();
    /// free a ImTextureID
    void FreeImage(ImTextureID img);
    /// associate an ImTextureID with an Oryol texture
    void BindImage(ImTextureID img, Id texId);

    /// setup font texture
    void setupFontTexture(const IMUISetup& setup);
    /// setup draw state
    void setupMeshAndDrawState();
    /// setup dummy 'white' texture
    void setupWhiteTexture();
    /// imgui's draw callback
    static void imguiRenderDrawLists(ImDrawData* draw_data);

    static const int MaxNumVertices = 64 * 1024;
    static const int MaxNumIndices = 128 * 1024;
    static const int MaxNumFonts = 4;

    static imguiWrapper* self;

    bool isValid = false;
    bool touchActive = false;
    ResourceLabel resLabel;
    DrawState drawState;
    StaticArray<ImFont*, MaxNumFonts> fonts;
    Id whiteTexture;
    Id fontTexture;
    static const int MaxImages = 256;
    StaticArray<Id, MaxImages> images;
    Array<intptr_t> freeImageSlots;
    ImDrawVert vertexData[MaxNumVertices];
    ImDrawIdx indexData[MaxNumIndices];
};

} // namespace _priv
} // namespace Oryol
