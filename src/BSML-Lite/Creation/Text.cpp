#include "BSML-Lite/Creation/Text.hpp"
#include "Helpers/getters.hpp"

#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Color.hpp"

#include "GlobalNamespace/Signal.hpp"
#include "GlobalNamespace/MenuShockwave.hpp"
#include "BeatSaber/Haptics/HapticFeedbackManager.hpp"
#include "BeatSaber/Haptics/HapticPresetSO.hpp"

#include "beatsaber-hook/shared/safeptr.hpp"

// Note: BSML-Lite owns text-component creation logic directly here. It no
// longer reaches into BSML::TextTag/ClickableTextTag's protected CreateObject
// (previously via `#define protected public`).

namespace BSML::Lite {
    namespace {
        GlobalNamespace::Signal* GetTextClickedSignal() {
            static safe_ptr<GlobalNamespace::Signal*> textClickedSignal;
            if (!textClickedSignal) {
                auto menuShockWave = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::MenuShockwave*>().front_or_default();
                textClickedSignal = menuShockWave ? menuShockWave->_buttonClickEvents.back_or_default() : nullptr;
            }
            return textClickedSignal.ptr();
        }

        BeatSaber::Haptics::HapticPresetSO* GetTextHapticPreset() {
            static safe_ptr<BeatSaber::Haptics::HapticPresetSO*> textHapticPreset;
            if (!textHapticPreset) {
                textHapticPreset = UnityEngine::ScriptableObject::CreateInstance<BeatSaber::Haptics::HapticPresetSO*>();
                textHapticPreset->_duration = 0.02f;
                textHapticPreset->_strength = 1.0f;
                textHapticPreset->_frequency = 0.2f;
                UnityEngine::Object::DontDestroyOnLoad(textHapticPreset.ptr());
            }
            return textHapticPreset.ptr();
        }

        BeatSaber::Haptics::HapticFeedbackManager* GetTextHapticFeedbackManager() {
            static safe_ptr<BeatSaber::Haptics::HapticFeedbackManager*> textHapticFeedbackManager;
            if (!textHapticFeedbackManager) {
                textHapticFeedbackManager = UnityEngine::Resources::FindObjectsOfTypeAll<BeatSaber::Haptics::HapticFeedbackManager*>().front();
            }
            return textHapticFeedbackManager.ptr();
        }
    }

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

        textMesh->buttonClickedSignal = GetTextClickedSignal();
        textMesh->hapticFeedbackPresetSO = GetTextHapticPreset();
        textMesh->hapticFeedbackManager = GetTextHapticFeedbackManager();

        if (options.onClick) textMesh->onClick += {options.onClick};

        gameObject->SetActive(true);
        return textMesh;
    }
}
