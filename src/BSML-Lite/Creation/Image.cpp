#include "BSML-Lite/Creation/Image.hpp"
#include "BSML-Lite/ComponentCreation.hpp"
#include "Helpers/getters.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/SpriteMeshType.hpp"
#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/TextureFormat.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/Vector4.hpp"
#include "cppcodec/base64_rfc4648.hpp"
#include <fstream>

// Note: BSML-Lite owns image-component creation logic directly here. It no
// longer reaches into BSML::{Image,RawImage,ClickableImage}Tag's protected
// CreateObject (previously via `#define protected public`).

namespace BSML::Lite {
    HMUI::ImageView* CreateImage(const TransformWrapper& parent, UnityEngine::Sprite* sprite, const ImageOptions& options) {
        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLImage");
        auto image = gameObject->AddComponent<HMUI::ImageView*>();
        image->set_material(Helpers::GetUINoGlowMat());
        image->set_sprite(sprite);

        auto rectTransform = image->get_rectTransform();
        rectTransform->SetParent(parent, false);
        rectTransform->set_anchoredPosition(options.anchoredPosition);
        rectTransform->set_sizeDelta(options.sizeDelta);

        gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        return image;
    }

    BSML::ClickableImage* CreateClickableImage(const TransformWrapper& parent, UnityEngine::Sprite* sprite, const ClickableImageOptions& options) {
        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLClickableImage");
        auto image = gameObject->AddComponent<BSML::ClickableImage*>();
        image->set_material(Helpers::GetUINoGlowMat());
        image->set_sprite(sprite);

        auto rectTransform = image->get_rectTransform();
        rectTransform->SetParent(parent, false);
        rectTransform->set_anchoredPosition(options.anchoredPosition);
        rectTransform->set_sizeDelta(options.sizeDelta);

        image->buttonClickedSignal = GetClickedSignal();
        image->hapticFeedbackPresetSO = GetClickHapticPreset();
        image->hapticFeedbackManager = GetClickHapticFeedbackManager();

        gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();

        if (options.onClick) image->onClick += {options.onClick};
        return image;
    }

    UnityEngine::UI::RawImage* CreateRawImage(const TransformWrapper& parent, UnityEngine::Texture* texture, const RawImageOptions& options) {
        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLRawImage");
        auto image = gameObject->AddComponent<UnityEngine::UI::RawImage*>();
        image->set_material(Helpers::GetUINoGlowMat());
        image->set_texture(texture);

        auto rectTransform = image->get_rectTransform();
        rectTransform->SetParent(parent, false);
        rectTransform->set_anchoredPosition(options.anchoredPosition);
        rectTransform->set_sizeDelta(options.sizeDelta);

        gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        return image;
    }

    UnityEngine::Sprite* FileToSprite(const std::string_view& filePath) {
        std::ifstream instream(filePath.data(), std::ios::in | std::ios::binary | std::ios::ate);
        il2cpp_array_size_t size = instream.tellg();
        instream.seekg(0, instream.beg);
        ArrayW<uint8_t> bytes(size);
        instream.read(reinterpret_cast<char*>(bytes->_values), size);
        return ArrayToSprite(bytes);
    }

    UnityEngine::Sprite* TextureToSprite(UnityEngine::Texture2D* tex) {
        return (tex && tex->m_CachedPtr.m_value) ? UnityEngine::Sprite::Create(tex, UnityEngine::Rect(0.0f, 0.0f, (float)tex->get_width(), (float)tex->get_height()), UnityEngine::Vector2(0.5f, 0.5f), 100.0f, 1u, UnityEngine::SpriteMeshType::FullRect, UnityEngine::Vector4(0.0f, 0.0f, 0.0f, 0.0f), false) : nullptr;
    }

    UnityEngine::Sprite* Base64ToSprite(const std::string_view& base64Str) {
        using base64 = cppcodec::base64_rfc4648;

        ArrayW<uint8_t> bytes(base64::decoded_max_size(base64Str.size()));
        base64::decode(bytes.begin(), bytes.size(), base64Str.data(), base64Str.size());
        return ArrayToSprite(bytes);
    }

    UnityEngine::Sprite* VectorToSprite(const std::vector<uint8_t>& bytes) {
        ArrayW<uint8_t> arr(il2cpp_array_size_t(bytes.size()));
        std::memcpy(arr->_values, bytes.data(), bytes.size());
        return ArrayToSprite(arr);
    }

    UnityEngine::Sprite* ArrayToSprite(ArrayW<uint8_t> bytes) {
        UnityEngine::Texture2D* texture = UnityEngine::Texture2D::New_ctor(1, 1, UnityEngine::TextureFormat::RGBA32, false, false);
        if (UnityEngine::ImageConversion::LoadImage(texture, bytes, false)) {
            return TextureToSprite(texture);
        }
        UnityEngine::Object::DestroyImmediate(texture);
        return nullptr;
    }
}
