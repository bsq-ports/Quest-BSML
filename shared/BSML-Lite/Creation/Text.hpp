#pragma once

#include "../../_config.h"
#include "../TransformWrapper.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "TMPro/TextRenderFlags.hpp"
#include "TMPro/FontStyles.hpp"
#include "UnityEngine/Transform.hpp"
#include "../../BSML/Components/ClickableText.hpp"
#include "../../BSML/Components/TextGradientUpdater.hpp"

namespace BSML::Lite {
    /// @brief Options for CreateText.
    struct BSML_EXPORT TextOptions {
        /// @brief the style of the font
        TMPro::FontStyles fontStyle = TMPro::FontStyles::Italic;
        /// @brief size of the font
        float fontSize = 4;
        /// @brief position of the anchor relative to the parent
        UnityEngine::Vector2 anchoredPosition = {0, 0};
        /// @brief how much smaller this thing is relative to the parent
        UnityEngine::Vector2 sizeDelta = {0, 0};
    };

    /// @brief Creates text which is parented to the passed parent
    /// @param parent parent transform to parent text to
    /// @param text the text to display
    /// @param options creation options (font style, font size, position, size)
    /// @return The created text
    BSML_EXPORT HMUI::CurvedTextMeshPro* CreateText(const TransformWrapper& parent, StringW text, const TextOptions& options = {});

    /// @brief Options for CreateClickableText.
    struct BSML_EXPORT ClickableTextOptions {
        /// @brief the style of the font
        TMPro::FontStyles fontStyle = TMPro::FontStyles::Italic;
        /// @brief size of the font
        float fontSize = 4;
        /// @brief position of the anchor relative to the parent
        UnityEngine::Vector2 anchoredPosition = {0, 0};
        /// @brief how much smaller this thing is relative to the parent
        UnityEngine::Vector2 sizeDelta = {0, 0};
        /// @brief what to run when it's clicked
        std::function<void()> onClick = nullptr;
    };

    /// @brief Creates Clickable text which is parented to the passed parent
    /// @param parent parent transform to parent text to
    /// @param text the text to display
    /// @param options creation options (font style, font size, position, size, click handler)
    /// @return The created text
    BSML_EXPORT BSML::ClickableText* CreateClickableText(const TransformWrapper& parent, StringW text, const ClickableTextOptions& options = {});

    /// @brief Creates Clickable text with a click handler (convenience overload for the common case)
    /// @param parent parent transform to parent text to
    /// @param text the text to display
    /// @param onClick what to run when it's clicked
    /// @return The created text
    static inline BSML::ClickableText* CreateClickableText(const TransformWrapper& parent, StringW text, std::function<void()> onClick) {
        return CreateClickableText(parent, text, ClickableTextOptions{.onClick = std::move(onClick)});
    }

    /// @brief Creates text with a scrolling gradient applied to it
    /// @param parent parent transform to parent text to
    /// @param text the text to display
    /// @return the created gradient updater (its `text` field is the underlying HMUI::CurvedTextMeshPro)
    BSML_EXPORT BSML::TextGradientUpdater* CreateGradientText(const TransformWrapper& parent, StringW text = "BSMLText");
}
