#include "BSML-Lite/Creation/Text.hpp"
#include "BSML-Lite/ComponentCreation.hpp"
#include "BSML/Components/TextGradientUpdater.hpp"
#include "Helpers/getters.hpp"

#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Color.hpp"

// Note: BSML-Lite owns text-component creation logic directly here. It no
// longer reaches into BSML::TextTag/ClickableTextTag's protected CreateObject
// (previously via `#define protected public`).

namespace BSML::Lite {
    HMUI::CurvedTextMeshPro* CreateText(const TransformWrapper& parent, StringW text, const TextOptions& options) {
        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLText");
        gameObject->get_transform()->SetParent(parent, false);

        auto textMesh = gameObject->AddComponent<HMUI::CurvedTextMeshPro*>();
        textMesh->set_font(Helpers::GetMainTextFont());
        textMesh->set_fontSharedMaterial(Helpers::GetMainUIFontMaterial());
        textMesh->set_color({1.0f, 1.0f, 1.0f, 1.0f});
        textMesh->set_text(text);
        textMesh->set_fontStyle(options.fontStyle);
        textMesh->set_fontSize(options.fontSize);

        auto rectTransform = textMesh->get_rectTransform();
        rectTransform->set_anchorMin({0.5f, 0.5f});
        rectTransform->set_anchorMax({0.5f, 0.5f});
        rectTransform->set_anchoredPosition(options.anchoredPosition);
        rectTransform->set_sizeDelta(options.sizeDelta);

        gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();

        return textMesh;
    }

    BSML::ClickableText* CreateClickableText(const TransformWrapper& parent, StringW text, const ClickableTextOptions& options) {
        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLClickableText");
        gameObject->SetActive(false);
        gameObject->get_transform()->SetParent(parent, false);

        auto textMesh = gameObject->AddComponent<BSML::ClickableText*>();
        textMesh->set_font(Helpers::GetMainTextFont());
        textMesh->set_fontSharedMaterial(Helpers::GetMainUIFontMaterial());
        textMesh->set_color({1.0f, 1.0f, 1.0f, 1.0f});
        textMesh->set_richText(true);
        textMesh->set_text(text);
        textMesh->set_fontStyle(options.fontStyle);
        textMesh->set_fontSize(options.fontSize);

        auto rectTransform = textMesh->get_rectTransform();
        rectTransform->set_anchorMin({0.5f, 0.5f});
        rectTransform->set_anchorMax({0.5f, 0.5f});
        rectTransform->set_anchoredPosition(options.anchoredPosition);
        rectTransform->set_sizeDelta(options.sizeDelta);

        gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();

        textMesh->buttonClickedSignal = GetClickedSignal();
        textMesh->hapticFeedbackPresetSO = GetClickHapticPreset();
        textMesh->hapticFeedbackManager = GetClickHapticFeedbackManager();

        if (options.onClick) textMesh->onClick += {options.onClick};

        gameObject->SetActive(true);
        return textMesh;
    }

    BSML::TextGradientUpdater* CreateGradientText(const TransformWrapper& parent, StringW text) {
        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLText");
        gameObject->get_transform()->SetParent(parent, false);

        auto textMesh = gameObject->AddComponent<HMUI::CurvedTextMeshPro*>();
        textMesh->set_font(Helpers::GetMainTextFont());
        textMesh->set_fontSharedMaterial(Helpers::GetMainUIFontMaterial());
        textMesh->set_fontSize(4);
        textMesh->set_color({1.0f, 1.0f, 1.0f, 1.0f});
        textMesh->set_text(text);

        auto rectTransform = textMesh->get_rectTransform();
        rectTransform->set_anchorMin({0.5f, 0.5f});
        rectTransform->set_anchorMax({0.5f, 0.5f});

        return gameObject->AddComponent<BSML::TextGradientUpdater*>();
    }
}
