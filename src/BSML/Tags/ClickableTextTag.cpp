#include "BSML/Tags/ClickableTextTag.hpp"
#include "BSML/Components/ClickableText.hpp"
#include "Helpers/getters.hpp"
#include "Helpers/creation.hpp"
#include "logging.hpp"

#include "HMUI/Touchable.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/Interactable.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"

#include "GlobalNamespace/Signal.hpp"
#include "GlobalNamespace/MenuShockwave.hpp"

#include "beatsaber-hook/shared/safeptr.hpp"

using namespace UnityEngine;

namespace BSML {
    static BSMLNodeParser<ClickableTextTag> clickableTextTagParser({"clickable-text"});

    GlobalNamespace::Signal* get_textClickedSignal() {
        static safe_ptr<GlobalNamespace::Signal*> textClickedSignal;
        if (!textClickedSignal) {
            auto menuShockWave = Resources::FindObjectsOfTypeAll<GlobalNamespace::MenuShockwave*>().front_or_default();
            textClickedSignal = menuShockWave ? menuShockWave->_buttonClickEvents.back_or_default() : nullptr;
        }
        return textClickedSignal.ptr();
    }

    UnityEngine::GameObject* ClickableTextTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating Clickable text");
        auto gameObject = GameObject::New_ctor("BSMLClickableText");
        gameObject->SetActive(false);
        gameObject->get_transform()->SetParent(parent, false);

        auto textMesh = gameObject->AddComponent<ClickableText*>();
        // Let VRInputModule provide native hover haptics, as for game controls.
        gameObject->AddComponent<HMUI::Interactable*>();
        textMesh->set_font(Helpers::GetMainTextFont());
        textMesh->set_fontSharedMaterial(Helpers::GetMainUIFontMaterial());
        textMesh->set_text("BSMLClickableText");
        textMesh->set_fontSize(4);
        textMesh->set_color({1.0f, 1.0f, 1.0f, 1.0f});
        textMesh->set_richText(true);

        auto rectTransform = textMesh->get_rectTransform();
        rectTransform->set_anchorMin({0.5f, 0.5f});
        rectTransform->set_anchorMax({0.5f, 0.5f});
        rectTransform->set_sizeDelta({90, 8});

        gameObject->AddComponent<UI::LayoutElement*>();

        textMesh->buttonClickedSignal = get_textClickedSignal();

        gameObject->SetActive(true);
        return gameObject;
    }
}
