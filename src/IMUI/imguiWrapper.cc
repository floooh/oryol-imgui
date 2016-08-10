//------------------------------------------------------------------------------
//  imguiWrapper.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "imguiWrapper.h"
#include "Core/Assertion.h"
#include "Input/Input.h"
#include "IMUIShaders.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace Oryol {
namespace _priv {


imguiWrapper* imguiWrapper::self = nullptr;

//------------------------------------------------------------------------------
void
imguiWrapper::Setup(const IMUISetup& setup) {
    o_assert_dbg(!this->IsValid());
    self = this;
    this->fonts.Fill(nullptr);

    this->freeImageSlots.Reserve(MaxImages);
    for (int i = MaxImages-1; i>=0; i--) {
        this->freeImageSlots.Add(i);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_Tab] = Key::Tab; 
    io.KeyMap[ImGuiKey_LeftArrow] = Key::Left;
    io.KeyMap[ImGuiKey_RightArrow] = Key::Right;
    io.KeyMap[ImGuiKey_UpArrow] = Key::Up;
    io.KeyMap[ImGuiKey_DownArrow] = Key::Down;
    io.KeyMap[ImGuiKey_Home] = Key::Home;
    io.KeyMap[ImGuiKey_End] = Key::End;
    io.KeyMap[ImGuiKey_Delete] = Key::Delete;
    io.KeyMap[ImGuiKey_Backspace] = Key::BackSpace;
    io.KeyMap[ImGuiKey_Enter] = Key::Enter;
    io.KeyMap[ImGuiKey_Escape] = Key::Escape;
    io.KeyMap[ImGuiKey_A] = Key::A;
    io.KeyMap[ImGuiKey_C] = Key::C;
    io.KeyMap[ImGuiKey_V] = Key::V;
    io.KeyMap[ImGuiKey_X] = Key::X;
    io.KeyMap[ImGuiKey_Y] = Key::Y;
    io.KeyMap[ImGuiKey_Z] = Key::Z;

    io.RenderDrawListsFn = imguiRenderDrawLists;

    #if ORYOL_RASPBERRYPI
    io.MouseDrawCursor = true;
    #endif

    // create gfx resources
    this->resLabel = Gfx::PushResourceLabel();
    this->setupMeshAndDrawState();
    this->setupWhiteTexture();
    this->setupFontTexture(setup);
    Gfx::PopResourceLabel();

    this->isValid = true;
}

//------------------------------------------------------------------------------
void
imguiWrapper::Discard() {
    o_assert_dbg(this->IsValid());
    ImGui::GetIO().Fonts->TexID = 0;
    ImGui::Shutdown();
    Gfx::DestroyResources(this->resLabel);
    this->isValid = false;
    self = nullptr;
}

//------------------------------------------------------------------------------
bool
imguiWrapper::IsValid() const {
    return this->isValid;
}

//------------------------------------------------------------------------------
void
imguiWrapper::setupWhiteTexture() {
    // this is the dummy texture for user-provided images that have
    // not been bound yet (because they might still be async-loading)
    const int w = 4;
    const int h = 4;
    uint32 pixels[w * h];
    Memory::Fill(pixels, sizeof(pixels), 0xFF);
    auto texSetup = TextureSetup::FromPixelData(w, h, 1, TextureType::Texture2D, PixelFormat::RGBA8);
    texSetup.Sampler.WrapU = TextureWrapMode::Repeat;
    texSetup.Sampler.WrapV = TextureWrapMode::Repeat;
    texSetup.Sampler.MinFilter = TextureFilterMode::Nearest;
    texSetup.Sampler.MagFilter = TextureFilterMode::Nearest;
    texSetup.ImageData.Sizes[0][0] = sizeof(pixels);
    this->whiteTexture = Gfx::CreateResource(texSetup, pixels, sizeof(pixels));
}

//------------------------------------------------------------------------------
void
imguiWrapper::setupFontTexture(const IMUISetup& setup) {
    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->AddFontDefault();
    for (int i = 0; i < setup.numFonts; i++) {
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        const auto& desc = setup.fonts[i];
        this->fonts[i] = io.Fonts->AddFontFromMemoryTTF(desc.ttf_data, desc.ttf_size, desc.font_height, &fontConfig);
        o_assert_dbg(this->fonts[i]);
    }

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    const int imgSize = width * height * sizeof(uint32_t);

    auto texSetup = TextureSetup::FromPixelData(width, height, 1, TextureType::Texture2D, PixelFormat::RGBA8);
    texSetup.Sampler.WrapU = TextureWrapMode::ClampToEdge;
    texSetup.Sampler.WrapV = TextureWrapMode::ClampToEdge;
    texSetup.Sampler.MinFilter = TextureFilterMode::Nearest;
    texSetup.Sampler.MagFilter = TextureFilterMode::Nearest;
    texSetup.ImageData.Sizes[0][0] = imgSize;
    this->fontTexture = Gfx::CreateResource(texSetup, pixels, imgSize);
    this->drawState.FSTexture[IMUITextures::Texture] = this->fontTexture;

    io.Fonts->TexID = this->AllocImage();
    this->BindImage(io.Fonts->TexID, this->fontTexture);
}

//------------------------------------------------------------------------------
void
imguiWrapper::setupMeshAndDrawState() {
    MeshSetup meshSetup = MeshSetup::Empty(MaxNumVertices, Usage::Stream, IndexType::Index16, MaxNumIndices, Usage::Stream);
    meshSetup.Layout
        .Add(VertexAttr::Position, VertexFormat::Float2)
        .Add(VertexAttr::TexCoord0, VertexFormat::Float2)
        .Add(VertexAttr::Color0, VertexFormat::UByte4N);
    o_assert_dbg(meshSetup.Layout.ByteSize() == sizeof(ImDrawVert));
    this->drawState.Mesh[0] = Gfx::CreateResource(meshSetup);
    o_assert(this->drawState.Mesh[0].IsValid());
    o_assert(Gfx::QueryResourceInfo(this->drawState.Mesh[0]).State == ResourceState::Valid);

    Id shd = Gfx::CreateResource(IMUIShader::Setup());
    auto ps = PipelineSetup::FromLayoutAndShader(meshSetup.Layout, shd);
    ps.DepthStencilState.DepthWriteEnabled = false;
    ps.DepthStencilState.DepthCmpFunc = CompareFunc::Always;
    ps.BlendState.BlendEnabled = true;
    ps.BlendState.SrcFactorRGB = BlendFactor::SrcAlpha;
    ps.BlendState.DstFactorRGB = BlendFactor::OneMinusSrcAlpha;
    ps.BlendState.ColorFormat = Gfx::DisplayAttrs().ColorPixelFormat;
    ps.BlendState.DepthFormat = Gfx::DisplayAttrs().DepthPixelFormat;
    ps.BlendState.ColorWriteMask = PixelChannel::RGB;
    ps.RasterizerState.ScissorTestEnabled = true;
    ps.RasterizerState.CullFaceEnabled = false;
    ps.RasterizerState.SampleCount = Gfx::DisplayAttrs().SampleCount;
    this->drawState.Pipeline = Gfx::CreateResource(ps);
}

//------------------------------------------------------------------------------
ImTextureID
imguiWrapper::AllocImage() {
    o_assert_dbg(!this->freeImageSlots.Empty());
    return ImTextureID(this->freeImageSlots.PopBack());
}

//------------------------------------------------------------------------------
void
imguiWrapper::FreeImage(ImTextureID img) {
    intptr_t slot = (intptr_t) img;
    o_assert_dbg(this->images[slot].IsValid());
    this->images[slot].Invalidate();
}

//------------------------------------------------------------------------------
void
imguiWrapper::BindImage(ImTextureID img, Id texId) {
    intptr_t slot = (intptr_t) img;
    o_assert_dbg(!this->images[slot].IsValid());
    this->images[slot] = texId;
}

//------------------------------------------------------------------------------
void
imguiWrapper::NewFrame(float frameDurationInSeconds) {

    ImGuiIO& io = ImGui::GetIO();
    DisplayAttrs dispAttrs = Gfx::RenderTargetAttrs();
    o_assert_dbg((dispAttrs.FramebufferWidth > 0) && (dispAttrs.FramebufferHeight > 0));
    io.DisplaySize = ImVec2((float)dispAttrs.FramebufferWidth, (float)dispAttrs.FramebufferHeight);
    io.DeltaTime = frameDurationInSeconds;

    // transfer input
    if (Input::IsValid()) {

        if (Input::MouseAttached()) {
            io.MousePos.x = Input::MousePosition().x;
            io.MousePos.y = Input::MousePosition().y;
            io.MouseWheel = Input::MouseScroll().y;
            for (int btn = 0; btn < 3; btn++) {
                io.MouseDown[btn] = Input::MouseButtonDown((MouseButton::Code)btn)||Input::MouseButtonPressed((MouseButton::Code)btn);
            }
        }
        if (Input::TouchpadAttached()) {
            const glm::vec2& touchPos = Input::TouchPosition(0);
            if ((touchPos.x > 0.0f) && (touchPos.y > 0.0f)) {
                io.MousePos.x = touchPos.x;
                io.MousePos.y = touchPos.y;
                io.MouseDown[0] = Input::TouchTapped() || Input::TouchPanning();
            }
        }

        if (Input::KeyboardAttached()) {
            const wchar_t* text = Input::Text();
            while (wchar_t c = *text++) {
                io.AddInputCharacter((unsigned short)c);
            }
            io.KeyCtrl  = Input::KeyPressed(Key::LeftControl) || Input::KeyPressed(Key::RightControl);
            io.KeyShift = Input::KeyPressed(Key::LeftShift) || Input::KeyPressed(Key::RightShift);
            io.KeyAlt   = Input::KeyPressed(Key::LeftAlt) || Input::KeyPressed(Key::RightAlt);

            const static Key::Code keys[] = {
                Key::Tab, Key::Left, Key::Right, Key::Up, Key::Down, Key::Home, Key::End,
                Key::Delete, Key::BackSpace, Key::Enter, Key::Escape,
                Key::A, Key::C, Key::V, Key::X, Key::Y, Key::Z
            };
            for (auto key : keys) {
                io.KeysDown[key] = Input::KeyDown(key)|Input::KeyPressed(key);;
            }
        }
    }
    ImGui::NewFrame();
}

//------------------------------------------------------------------------------
void
imguiWrapper::imguiRenderDrawLists(ImDrawData* draw_data) {
    o_assert_dbg(self);
    o_assert_dbg(draw_data);

    if (draw_data->CmdListsCount == 0) {
        return;
    }

    // copy over vertices into single vertex buffer
    int numVertices = 0;
    int numIndices = 0;
    int numCmdLists;
    for (numCmdLists = 0; numCmdLists < draw_data->CmdListsCount; numCmdLists++) {

        const ImDrawList* cmd_list = draw_data->CmdLists[numCmdLists];
        const int cmdListNumVertices = cmd_list->VtxBuffer.size();
        const int cmdListNumIndices  = cmd_list->IdxBuffer.size();

        // overflow check
        if (((numVertices + cmdListNumVertices) > imguiWrapper::MaxNumVertices) ||
            ((numIndices + cmdListNumIndices) > imguiWrapper::MaxNumIndices)) {
            break;
        }
        // copy vertices
        Memory::Copy((const void*)&cmd_list->VtxBuffer.front(),
                     &(self->vertexData[numVertices]),
                     cmdListNumVertices * sizeof(ImDrawVert));

        // need to copy indices one by one and add the current base vertex index :/
        const ImDrawIdx* srcIndexPtr = &cmd_list->IdxBuffer.front();
        const uint16_t baseVertexIndex = numVertices;
        for (int i = 0; i < cmdListNumIndices; i++) {
            self->indexData[numIndices++] = srcIndexPtr[i] + baseVertexIndex;
        }
        numVertices += cmdListNumVertices;
    }

    // draw command lists
    IMUIShader::VSParams vsParams;
    const float width  = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    vsParams.Ortho = glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
    const int vertexDataSize = numVertices * sizeof(ImDrawVert);
    const int indexDataSize = numIndices * sizeof(ImDrawIdx);

    Gfx::UpdateVertices(self->drawState.Mesh[0], self->vertexData, vertexDataSize);
    Gfx::UpdateIndices(self->drawState.Mesh[0], self->indexData, indexDataSize);
    Id curTexture;
    int elmOffset = 0;
    for (int cmdListIndex = 0; cmdListIndex < numCmdLists; cmdListIndex++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[cmdListIndex];
        const ImDrawCmd* pcmd_end = cmd_list->CmdBuffer.end();
        for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != pcmd_end; pcmd++) {
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else {
                const Id& newTexture = self->images[intptr_t(pcmd->TextureId)];
                if (curTexture != newTexture) {
                    if (newTexture.IsValid()) {
                        self->drawState.FSTexture[IMUITextures::Texture] = newTexture;
                    }
                    else {
                        self->drawState.FSTexture[IMUITextures::Texture] = self->whiteTexture;
                    }
                    curTexture = newTexture;
                    Gfx::ApplyDrawState(self->drawState);
                    Gfx::ApplyUniformBlock(vsParams);
                }
                Gfx::ApplyScissorRect((int)pcmd->ClipRect.x,
                                      (int)(height - pcmd->ClipRect.w),
                                      (int)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                                      (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                Gfx::Draw(PrimitiveGroup(elmOffset, pcmd->ElemCount));
            }
            elmOffset += pcmd->ElemCount;
        }
    }
    Gfx::ApplyScissorRect(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
}

} // namespace _priv
} // namespace Oryol
