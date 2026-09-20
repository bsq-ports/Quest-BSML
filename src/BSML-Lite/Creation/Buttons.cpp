#include "BSML-Lite/Creation/Buttons.hpp"
#include "BSML-Lite/ComponentCreation.hpp"
#include "logging.hpp"
#include "custom-types/shared/delegate.hpp"

#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/LayoutGroup.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/TextureWrapMode.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
#include "HMUI/ButtonSpriteSwap.hpp"
#include "HMUI/HoverHint.hpp"
#include "HMUI/ImageView.hpp"
#include "HMUI/ScrollView.hpp"

#include "Helpers/getters.hpp"
#include "Helpers/utilities.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "GlobalNamespace/PracticeViewController.hpp"
#include "GlobalNamespace/PlayerOptionsViewController.hpp"
#include "GlobalNamespace/PlayerSettingsPanelController.hpp"
#include "GlobalNamespace/LocalizedHoverHint.hpp"

#include "BSML/Components/ExternalComponents.hpp"
#include "BSML/Components/ButtonIconImage.hpp"
#include "BSML/Components/PageButton.hpp"
#include "beatsaber-hook/shared/safeptr.hpp"

// Note: BSML-Lite owns button-prefab resolution and creation directly here.
// It no longer reaches into BSML::ButtonTag/PrimaryButtonTag's protected
// CreateObject (previously via `#define protected public`).

namespace BSML::Lite {
    namespace {
        UnityEngine::UI::Button* GetPracticeButtonPrefab() {
            static safe_ptr<UnityEngine::UI::Button*> prefab;
            if (!prefab)
                prefab = Helpers::GetDiContainer()->Resolve<GlobalNamespace::StandardLevelDetailViewController*>()->_standardLevelDetailView->get_practiceButton();
            return prefab.ptr();
        }

        UnityEngine::UI::Button* GetPlayButtonPrefab() {
            static safe_ptr<UnityEngine::UI::Button*> prefab;
            if (!prefab)
                prefab = Helpers::GetDiContainer()->Resolve<GlobalNamespace::PracticeViewController*>()->_playButton;
            return prefab.ptr();
        }

        UnityEngine::UI::Button* ResolveButtonPrefab(std::string_view buttonTemplate) {
            if (buttonTemplate == DEFAULT_BUTTONTEMPLATE) return GetPracticeButtonPrefab();
            if (buttonTemplate == "PlayButton") return GetPlayButtonPrefab();

            static std::unordered_map<std::string, safe_ptr<UnityEngine::UI::Button*>> buttonCopyMap;
            auto& buttonCopy = buttonCopyMap[std::string(buttonTemplate)];
            if (!buttonCopy) {
                buttonCopy = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::UI::Button*>()
                    .back_or_default([&buttonTemplate](auto* x) { return x->get_name() == buttonTemplate; });
            }
            if (!buttonCopy) {
                ERROR("Could not find button template {}, returning nullptr", buttonTemplate);
                return nullptr;
            }
            return buttonCopy.ptr();
        }
    }

    UnityEngine::UI::Button* CreateUIButton(const TransformWrapper& parent, StringW buttonText, const ButtonOptions& options) {
        auto prefab = ResolveButtonPrefab(options.buttonTemplate);
        if (!prefab) return nullptr;

        auto instance = InstantiatePrefab(prefab, parent, "BSMLButton");
        UnityEngine::UI::Button* button = instance.component.ptr();
        UnityEngine::GameObject* gameObject = instance.gameObject.ptr();
        UnityEngine::RectTransform* rect = instance.rectTransform.ptr();
        BSML::ExternalComponents* externalComponents = instance.externalComponents.ptr();
        button->set_interactable(true);

        StripLocalizedTextAndSet(button->get_transform(), "Content/Text", buttonText, externalComponents);
        ApplyStandardSizingFixups(gameObject, rect->Find("Content"), button, externalComponents);

        rect->set_anchoredPosition(options.anchoredPosition);
        rect->set_sizeDelta(options.sizeDelta);
        if (auto layoutElement = button->GetComponent<UnityEngine::UI::LayoutElement*>()) {
            if (options.sizeDelta.x != 0) layoutElement->preferredWidth = options.sizeDelta.x;
            if (options.sizeDelta.y != 0) layoutElement->preferredHeight = options.sizeDelta.y;
        }

        button->set_onClick(UnityEngine::UI::Button::ButtonClickedEvent::New_ctor());
        if (options.onClick)
            button->get_onClick()->AddListener(custom_types::MakeDelegate<UnityEngine::Events::UnityAction*>(options.onClick));
        return button;
    }

    void SetButtonText(UnityEngine::UI::Button* button, StringW text) {
        if (!button) {
            ERROR("Can't set button text on nullptr button");
            return;
        }

        auto textMesh = button->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
        if (textMesh)
            textMesh->set_text(text);
    }

    void SetButtonTextSize(UnityEngine::UI::Button* button, float fontSize) {
        auto textMesh = button->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
        if (textMesh) textMesh->set_fontSize(fontSize);
    }

    void ToggleButtonWordWrapping(UnityEngine::UI::Button* button, bool enableWordWrapping) {
        auto textMesh = button->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
        if (textMesh) textMesh->set_enableWordWrapping(enableWordWrapping);
    }

    void SetButtonIcon(UnityEngine::UI::Button* button, UnityEngine::Sprite* icon) {
        auto iconImage = button->GetComponentsInChildren<UnityEngine::UI::Image*>().front_or_default([](auto x){ return x->get_name() == "Icon"; });
        if (iconImage) iconImage->set_sprite(icon);
    }

    void SetButtonBackground(UnityEngine::UI::Button* button, UnityEngine::Sprite* background) {
        auto iconImage = button->GetComponentsInChildren<UnityEngine::UI::Image*>().front_or_default([](auto x){ return x->get_name() == "Background"; });
        if (iconImage) iconImage->set_sprite(background);
    }

    void SetButtonSprites(UnityEngine::UI::Button* button, UnityEngine::Sprite* inactive, UnityEngine::Sprite* active) {
        // make sure the textures are set to clamp
        inactive->get_texture()->set_wrapMode(UnityEngine::TextureWrapMode::Clamp);
        active->get_texture()->set_wrapMode(UnityEngine::TextureWrapMode::Clamp);

        auto spriteSwap = button->GetComponent<HMUI::ButtonSpriteSwap*>();

        // setting the sprites
        spriteSwap->_highlightStateSprite = active;
        spriteSwap->_pressedStateSprite = active;

        spriteSwap->_disabledStateSprite = inactive;
        spriteSwap->_normalStateSprite = inactive;
    }

    UnityEngine::GameObject* CreateIconButton(const TransformWrapper& parent) {
        auto button = UnityEngine::Object::Instantiate(GetPracticeButtonPrefab(), parent, false);
        button->set_name("BSMLIconButton");
        button->set_interactable(true);
        auto transform = button->transform.cast<UnityEngine::RectTransform>();
        auto gameObject = button->get_gameObject();
        gameObject->SetActive(false);

        UnityEngine::Object::Destroy(button->GetComponent<HMUI::HoverHint*>());
        UnityEngine::Object::Destroy(button->GetComponent<GlobalNamespace::LocalizedHoverHint*>());

        auto externalComponents = gameObject->AddComponent<BSML::ExternalComponents*>();
        externalComponents->Add(button);
        externalComponents->Add(transform);

        auto contentTransform = transform->Find("Content");
        UnityEngine::Object::Destroy(contentTransform->GetComponent<UnityEngine::UI::LayoutElement*>());

        UnityEngine::Object::Destroy(contentTransform->Find("Text")->get_gameObject());

        auto iconImage = UnityEngine::GameObject::New_ctor("Icon")->AddComponent<HMUI::ImageView*>();
        iconImage->set_material(Helpers::GetUINoGlowMat());
        auto iconRectTransform = iconImage->get_rectTransform();
        iconRectTransform->SetParent(contentTransform, false);
        iconRectTransform->set_anchoredPosition({0, 0});
        iconRectTransform->set_sizeDelta({20, 20});
        iconRectTransform->set_anchorMin({0.5f, 0.f});
        iconRectTransform->set_anchorMax({0.5f, 0.5f});
        iconImage->set_preserveAspect(true);
        iconImage->set_sprite(Utilities::FindSpriteCached("EditIcon"));

        auto btnIcon = gameObject->AddComponent<BSML::ButtonIconImage*>();
        btnIcon->button = UnityW<UnityEngine::UI::Button>(button).cast<HMUI::NoTransitionsButton>();
        btnIcon->image = iconImage;
        btnIcon->underline = transform->Find("Underline")->get_gameObject();
        btnIcon->SetSkew(transform->Find("BG")->GetComponent<HMUI::ImageView*>()->get_skew());
        externalComponents->Add(btnIcon);

        auto buttonSizeFitter = gameObject->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        buttonSizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        buttonSizeFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        externalComponents->Add(buttonSizeFitter);

        auto stackLayoutGroup = button->GetComponentInChildren<UnityEngine::UI::LayoutGroup*>();
        if (stackLayoutGroup) externalComponents->Add(stackLayoutGroup);

        auto layoutElement = gameObject->GetComponent<UnityEngine::UI::LayoutElement*>();
        if (!layoutElement) layoutElement = gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        externalComponents->Add(layoutElement);

        gameObject->SetActive(true);
        return gameObject;
    }

    UnityEngine::GameObject* CreatePageButton(const TransformWrapper& parent) {
        static safe_ptr<UnityEngine::UI::Button*> pageButtonTemplate;
        if (!pageButtonTemplate)
            pageButtonTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::PlayerOptionsViewController*>()->_playerSettingsPanelController->GetComponent<HMUI::ScrollView*>()->_pageUpButton;

        auto button = UnityEngine::Object::Instantiate(pageButtonTemplate.ptr(), parent, false);
        auto gameObject = button->get_gameObject();
        gameObject->SetActive(false);
        gameObject->set_name("BSMLPageButton");
        button->set_interactable(true);

        auto transform = button->transform.cast<UnityEngine::RectTransform>();
        auto pageButton = gameObject->AddComponent<BSML::PageButton*>();

        auto externalComponents = gameObject->AddComponent<BSML::ExternalComponents*>();
        externalComponents->Add(button);
        externalComponents->Add(transform);
        externalComponents->Add(pageButton);

        auto btnIcon = gameObject->AddComponent<BSML::ButtonIconImage*>();
        btnIcon->button = UnityW<UnityEngine::UI::Button>(button).cast<HMUI::NoTransitionsButton>();
        btnIcon->image = gameObject->GetComponentsInChildren<UnityEngine::UI::Image*>(true).front_or_default([](auto x) { return x->get_name() == "Icon"; });
        externalComponents->Add(btnIcon);

        auto buttonSizeFitter = gameObject->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        buttonSizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        buttonSizeFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        externalComponents->Add(buttonSizeFitter);

        auto layoutElement = gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        layoutElement->set_preferredWidth(-1);
        layoutElement->set_preferredHeight(-1);
        layoutElement->set_flexibleHeight(0);
        layoutElement->set_flexibleWidth(0);

        auto buttonTransform = transform->GetChild(0).cast<UnityEngine::RectTransform>();
        buttonTransform->set_anchorMin({0, 0});
        buttonTransform->set_anchorMax({1, 1});
        buttonTransform->set_sizeDelta({0, 0});
        transform->set_pivot({0.5f, 0.5f});
        externalComponents->Add(layoutElement);

        gameObject->SetActive(true);
        return gameObject;
    }
}
