#include "BSML/Tags/ClickableImageTag.hpp"
#include "Helpers/getters.hpp"
#include "Helpers/utilities.hpp"
#include "logging.hpp"

#include "BSML/Components/ClickableImage.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/Vector2.hpp"
#include "HMUI/ImageView.hpp"

#include "GlobalNamespace/Signal.hpp"
#include "GlobalNamespace/MenuShockwave.hpp"
#include "GlobalNamespace/HapticFeedbackManager.hpp"
#include "Libraries/HM/HMLib/VR/HapticPresetSO.hpp"

#include "beatsaber-hook/shared/safeptr.hpp"

using namespace UnityEngine;
using HapticPresetSO = Libraries::HM::HMLib::VR::HapticPresetSO;

namespace BSML {
    static BSMLNodeParser<ClickableImageTag> imageTagParser({"clickable-image", "clickable-img"});

    GlobalNamespace::Signal* get_imageClickedSignal() {
        static safe_ptr<GlobalNamespace::Signal*> imageClickedSignal;
        if (!imageClickedSignal) {
            auto menuShockWave = Resources::FindObjectsOfTypeAll<GlobalNamespace::MenuShockwave*>().front_or_default();
            imageClickedSignal = menuShockWave ? menuShockWave->_buttonClickEvents.back_or_default() : nullptr;
        }
        return imageClickedSignal.ptr();
    }

    HapticPresetSO* get_imageHapticPreset() {
        static safe_ptr<HapticPresetSO*> imageHapticPreset;
        if (!imageHapticPreset) {
            imageHapticPreset = UnityEngine::ScriptableObject::CreateInstance<HapticPresetSO*>();
            imageHapticPreset->_duration = 0.02f;
            imageHapticPreset->_strength = 1.0f;
            imageHapticPreset->_frequency = 0.2f;
            Object::DontDestroyOnLoad(imageHapticPreset.ptr());
        }
        return imageHapticPreset.ptr();
    }

    GlobalNamespace::HapticFeedbackManager* get_imageHapticFeedbackManager() {
        static safe_ptr<GlobalNamespace::HapticFeedbackManager*> imageHapticFeedbackManager;
        if (!imageHapticFeedbackManager) {
            imageHapticFeedbackManager = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::HapticFeedbackManager*>().front();
        }
        return imageHapticFeedbackManager.ptr();
    }

    UnityEngine::GameObject* ClickableImageTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating Image");
        auto gameObject = GameObject::New_ctor("BSMLClickableImage");
        auto image = gameObject->AddComponent<ClickableImage*>();
        image->set_material(Helpers::GetUINoGlowMat());
        auto rectTransform = image->get_rectTransform();
        rectTransform->set_sizeDelta({20, 20});
        rectTransform->SetParent(parent, false);
        // TODO: maybe use a default placeholder sprite instead? maybe a BSML image
        image->set_sprite(Utilities::ImageResources::GetBlankSprite());

        image->buttonClickedSignal = get_imageClickedSignal();
        image->hapticFeedbackPresetSO = get_imageHapticPreset();
        image->hapticFeedbackManager = get_imageHapticFeedbackManager();

        gameObject->AddComponent<UI::LayoutElement*>();
        return gameObject;
    }
}
