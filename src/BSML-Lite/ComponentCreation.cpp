#include "BSML-Lite/ComponentCreation.hpp"

#include "BGLib/Polyglot/LocalizedTextMeshProUGUI.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/Component.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/UI/LayoutGroup.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/ScriptableObject.hpp"
#include "GlobalNamespace/Signal.hpp"
#include "GlobalNamespace/MenuShockwave.hpp"
#include "BeatSaber/Haptics/HapticFeedbackManager.hpp"
#include "BeatSaber/Haptics/HapticPresetSO.hpp"

namespace BSML::Lite {
    TMPro::TextMeshProUGUI* StripLocalizedTextAndSet(UnityEngine::Transform* root, std::string_view relativePath, StringW text, BSML::ExternalComponents* externalComponents) {
        UnityEngine::Transform* target = root;
        if (!relativePath.empty()) {
            target = root->Find(relativePath);
            if (!target) return nullptr;
        }
        auto textObject = target->get_gameObject();

        if (auto localizer = textObject->GetComponent<BGLib::Polyglot::LocalizedTextMeshProUGUI*>())
            UnityEngine::Object::Destroy(localizer);

        auto textMesh = textObject->GetComponent<TMPro::TextMeshProUGUI*>();
        if (!textMesh) return nullptr;

        textMesh->set_text(text);
        textMesh->set_richText(true);
        if (externalComponents) externalComponents->Add(textMesh);
        return textMesh;
    }

    void ApplyStandardSizingFixups(UnityEngine::GameObject* gameObject, UnityEngine::Transform* strayLayoutElementParent, UnityEngine::Component* searchLayoutGroupRoot, BSML::ExternalComponents* externalComponents) {
        if (strayLayoutElementParent) {
            if (auto stray = strayLayoutElementParent->GetComponent<UnityEngine::UI::LayoutElement*>())
                UnityEngine::Object::Destroy(stray);
        }

        auto sizeFitter = gameObject->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        sizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        sizeFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        externalComponents->Add(sizeFitter);

        if (auto stackLayoutGroup = searchLayoutGroupRoot->GetComponentInChildren<UnityEngine::UI::LayoutGroup*>())
            externalComponents->Add(stackLayoutGroup);

        auto layoutElement = gameObject->GetComponent<UnityEngine::UI::LayoutElement*>();
        if (!layoutElement) layoutElement = gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        externalComponents->Add(layoutElement);
    }

    GlobalNamespace::Signal* GetClickedSignal() {
        static safe_ptr<GlobalNamespace::Signal*> clickedSignal;
        if (!clickedSignal) {
            auto menuShockWave = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::MenuShockwave*>().front_or_default();
            clickedSignal = menuShockWave ? menuShockWave->_buttonClickEvents.back_or_default() : nullptr;
        }
        return clickedSignal.ptr();
    }

    BeatSaber::Haptics::HapticPresetSO* GetClickHapticPreset() {
        static safe_ptr<BeatSaber::Haptics::HapticPresetSO*> hapticPreset;
        if (!hapticPreset) {
            hapticPreset = UnityEngine::ScriptableObject::CreateInstance<BeatSaber::Haptics::HapticPresetSO*>();
            hapticPreset->_duration = 0.02f;
            hapticPreset->_strength = 1.0f;
            hapticPreset->_frequency = 0.2f;
            UnityEngine::Object::DontDestroyOnLoad(hapticPreset.ptr());
        }
        return hapticPreset.ptr();
    }

    BeatSaber::Haptics::HapticFeedbackManager* GetClickHapticFeedbackManager() {
        static safe_ptr<BeatSaber::Haptics::HapticFeedbackManager*> hapticFeedbackManager;
        if (!hapticFeedbackManager) {
            hapticFeedbackManager = UnityEngine::Resources::FindObjectsOfTypeAll<BeatSaber::Haptics::HapticFeedbackManager*>().front();
        }
        return hapticFeedbackManager.ptr();
    }
}
