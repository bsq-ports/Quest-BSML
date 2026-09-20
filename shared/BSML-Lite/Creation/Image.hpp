#pragma once

#include "../../_config.h"

#include "../TransformWrapper.hpp"
#include "HMUI/ImageView.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/UI/RawImage.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "../../BSML/Components/ClickableImage.hpp"

namespace BSML::Lite {
    /// @brief Options for CreateImage.
    struct BSML_EXPORT ImageOptions {
        /// @brief position of the anchor relative to the parent
        UnityEngine::Vector2 anchoredPosition = {0, 0};
        /// @brief how much smaller this thing is relative to the parent
        UnityEngine::Vector2 sizeDelta = {0, 0};
    };

    /// @brief Creates an image, parented to the passed parent
    BSML_EXPORT HMUI::ImageView* CreateImage(const TransformWrapper& parent, UnityEngine::Sprite* sprite, const ImageOptions& options = {});

    /// @brief Options for CreateClickableImage.
    struct BSML_EXPORT ClickableImageOptions {
        /// @brief position of the anchor relative to the parent
        UnityEngine::Vector2 anchoredPosition = {0, 0};
        /// @brief how much smaller this thing is relative to the parent
        UnityEngine::Vector2 sizeDelta = {0, 0};
        /// @brief what to run when it's clicked
        std::function<void()> onClick = nullptr;
    };

    /// @brief Creates a clickable image, parented to the passed parent
    BSML_EXPORT BSML::ClickableImage* CreateClickableImage(const TransformWrapper& parent, UnityEngine::Sprite* sprite, const ClickableImageOptions& options = {});

    /// @brief Creates a clickable image with a click handler (convenience overload for the common case)
    static inline BSML::ClickableImage* CreateClickableImage(const TransformWrapper& parent, UnityEngine::Sprite* sprite, std::function<void()> onClick) {
        return CreateClickableImage(parent, sprite, ClickableImageOptions{.onClick = std::move(onClick)});
    }

    /// @brief Options for CreateRawImage.
    struct BSML_EXPORT RawImageOptions {
        /// @brief position of the anchor relative to the parent
        UnityEngine::Vector2 anchoredPosition = {0, 0};
        /// @brief how much smaller this thing is relative to the parent
        UnityEngine::Vector2 sizeDelta = {0, 0};
    };

    /// @brief Creates a raw image (backed by a Texture rather than a Sprite), parented to the passed parent
    BSML_EXPORT UnityEngine::UI::RawImage* CreateRawImage(const TransformWrapper& parent, UnityEngine::Texture* texture, const RawImageOptions& options = {});

    /// @brief loads a sprite from a file path
    /// @param filePath path
    /// @return created sprite
    BSML_EXPORT UnityEngine::Sprite* FileToSprite(const std::string_view& filePath);

    /// @brief make a sprite from a texture
    /// @param tex texture to use
    /// @return created sprite
    BSML_EXPORT UnityEngine::Sprite* TextureToSprite(UnityEngine::Texture2D* tex);

    /// @brief make a sprite from a base64 encoded image
    /// @param base64Str the encoded string
    /// @return created sprite
    BSML_EXPORT UnityEngine::Sprite* Base64ToSprite(const std::string_view& base64Str);

    /// @brief Get a sprite from a vector of bytes
    /// @param bytes bytes to load from
    /// @return created sprite
    BSML_EXPORT UnityEngine::Sprite* VectorToSprite(const std::vector<uint8_t>& bytes);

    /// @brief Get a sprite from a C# array of bytes
    /// @param bytes bytes to load from
    /// @return created sprite
    BSML_EXPORT UnityEngine::Sprite* ArrayToSprite(ArrayW<uint8_t> bytes);
}
