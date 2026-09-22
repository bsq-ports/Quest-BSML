#include "IndexedAnimation.hpp"
#include "UnityEngine/Application.hpp"
#include "UnityEngine/AssetBundle.hpp"
#include "UnityEngine/Shader.hpp"
#include "UnityEngine/Vector4.hpp"
#include "UnityEngine/SystemInfo.hpp"
#include "UnityEngine/Graphics.hpp"
#include "UnityEngine/GL.hpp"
#include "UnityEngine/QualitySettings.hpp"
#include "UnityEngine/ColorSpace.hpp"
#include "UnityEngine/FilterMode.hpp"
#include "UnityEngine/TextureWrapMode.hpp"
#include "UnityEngine/TextureFormat.hpp"
#include "UnityEngine/Rendering/CopyTextureSupport.hpp"
#include "UnityEngine/Experimental/Rendering/GraphicsFormat.hpp"
#include "System/IntPtr.hpp"
#include "logging.hpp"
#include <fstream>

namespace BSML::detail {
    namespace {
        constexpr uint8_t shaderBundle[] = {
            #embed "../../../assets/shaders/bsml-indexed-gif.bundle"
        };
        safe_ptr<UnityEngine::Shader*, true> expansionShader;
        bool attemptedShaderLoad = false;

        UnityEngine::Shader* GetShader() {
            if (expansionShader) return expansionShader.ptr();
            if (attemptedShaderLoad) return nullptr;
            attemptedShaderLoad = true;
            // LoadFromMemory is stripped from the game. Extract the embedded
            // bundle into the application's cache for the available file API.
            std::string path = UnityEngine::Application::get_temporaryCachePath();
            path += "/bsml-indexed-gif.bundle";
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            file.write(reinterpret_cast<const char*>(shaderBundle), sizeof(shaderBundle));
            file.close();
            if (!file) { ERROR("Could not write indexed GIF shader bundle"); return nullptr; }
            auto loadedBundle = UnityEngine::AssetBundle::LoadFromFile(path);
            safe_ptr<UnityEngine::AssetBundle*, true> bundle = loadedBundle ? loadedBundle.ptr() : nullptr;
            if (!bundle) { ERROR("Could not load indexed GIF shader bundle"); return nullptr; }
            auto loadedShader = bundle->LoadAsset("assets/indexedgifexpand.shader");
            expansionShader = loadedShader ? static_cast<UnityEngine::Shader*>(loadedShader.ptr()) : nullptr;
            bundle->Unload(false);
            if (expansionShader) expansionShader->set_hideFlags(UnityEngine::HideFlags::HideAndDontSave);
            if (!expansionShader) { ERROR("Indexed GIF shader is missing from bundle"); return nullptr; }
            INFO("Loaded indexed GIF expansion shader");
            return expansionShader.ptr();
        }

        void Configure(UnityEngine::Texture* texture, UnityEngine::FilterMode filter) {
            texture->set_hideFlags(UnityEngine::HideFlags::HideAndDontSave);
            texture->set_filterMode(filter);
            texture->set_wrapMode(UnityEngine::TextureWrapMode::Clamp);
        }

        template<class T> void Destroy(safe_ptr<T*, true>& object) {
            if (object) UnityEngine::Object::DestroyImmediate(object.ptr());
        }
    }

    bool IndexedAnimation::Supported() {
        using namespace UnityEngine;
        auto copy = static_cast<int>(SystemInfo::get_copyTextureSupport());
        constexpr int needed = 1 | 4 | 16; // Basic, DifferentTypes, RTToTexture
        if ((copy & needed) != needed || !SystemInfo::SupportsTextureFormat(TextureFormat::R8)) return false;
        auto shader = GetShader();
        return shader && shader->get_isSupported();
    }

    std::unique_ptr<IndexedAnimation> IndexedAnimation::Create(const std::vector<IndexedFrame>& frames,
        IndexedAtlasLayout layout, UnityEngine::Texture2D* output) {
        using namespace UnityEngine;
        if (!Supported() || frames.empty() || !layout.columns || !output) return nullptr;
        auto self = std::make_unique<IndexedAnimation>();
        self->layout = layout;
        self->width = output->get_width();
        self->height = output->get_height();
        self->frameCount = frames.size();
        self->output = output;
        std::vector<uint8_t> pixels(size_t(layout.width) * layout.height);
        std::vector<uint8_t> palettes(frames.size() * 256 * 4);
        for (size_t f = 0; f < frames.size(); ++f) {
            int x = (f % layout.columns) * self->width, y = (f / layout.columns) * self->height;
            for (int row = 0; row < self->height; ++row)
                std::copy_n(frames[f].indices.data() + size_t(row) * self->width, self->width,
                    pixels.data() + size_t(y + row) * layout.width + x);
            std::copy(frames[f].palette.begin(), frames[f].palette.end(), palettes.begin() + f * 1024);
        }
        self->indices = Texture2D::New_ctor(layout.width, layout.height, TextureFormat::R8, false, true);
        Configure(self->indices.ptr(), FilterMode::Point);
        self->indices->LoadRawTextureData(System::IntPtr(pixels.data()), pixels.size());
        self->indices->Apply(false, true);
        // sRGB palette sampling and an sRGB target match ordinary RGBA sprites.
        self->palettes = Texture2D::New_ctor(256, frames.size(), TextureFormat::RGBA32, false, false);
        Configure(self->palettes.ptr(), FilterMode::Point);
        self->palettes->LoadRawTextureData(System::IntPtr(palettes.data()), palettes.size());
        self->palettes->Apply(false, true);
        self->target = RenderTexture::New_ctor(self->width, self->height, 0, output->get_graphicsFormat());
        Configure(self->target.ptr(), FilterMode::Point);
        self->target->set_useMipMap(false);
        self->target->set_autoGenerateMips(false);
        if (!self->target->Create() || self->target->get_graphicsFormat() != output->get_graphicsFormat()) return nullptr;
        self->material = Material::New_ctor(GetShader());
        self->material->set_hideFlags(HideFlags::HideAndDontSave);
        self->material->SetTexture("_PaletteTex", self->palettes.ptr());
        if (!self->Expand(0)) return nullptr;
        INFO("Indexed GIF ready: {}x{}, {} frames, {}x{} R8 atlas", self->width, self->height, self->frameCount, layout.width, layout.height);
        return self;
    }

    bool IndexedAnimation::Expand(int frame) {
        using namespace UnityEngine;
        if (!target || !output || !material || frame < 0 || frame >= frameCount) return false;
        if (target->IsCreated() && expandedFrame == frame) return true;
        if (!target->IsCreated() && !target->Create()) return false;
        static int frameRectId = Shader::PropertyToID("_FrameRect");
        static int paletteRowId = Shader::PropertyToID("_PaletteRow");
        material->SetVector(frameRectId, {
            float((frame % layout.columns) * width) / layout.width,
            float((frame / layout.columns) * height) / layout.height,
            float(width) / layout.width, float(height) / layout.height});
        material->SetFloat(paletteRowId, (frame + 0.5f) / frameCount);
        // Blit changes global render state. Preserve it for the game's renderer.
        struct RenderState {
            RenderTexture* previous = RenderTexture::get_active().unsafe_ptr();
            bool srgb = GL::get_sRGBWrite();
            ~RenderState() { RenderTexture::set_active(previous); GL::set_sRGBWrite(srgb); }
        } restore;
        GL::set_sRGBWrite(QualitySettings::get_activeColorSpace() == ColorSpace::Linear);
        Graphics::Blit(indices.ptr(), target.ptr(), material.ptr(), 0);
        Graphics::CopyTexture(target.ptr(), 0, 0, output.ptr(), 0, 0);
        expandedFrame = frame;
        return true;
    }

    IndexedAnimation::~IndexedAnimation() {
        Destroy(material);
        if (target) target->Release();
        Destroy(target);
        Destroy(palettes);
        Destroy(indices);
    }
}
