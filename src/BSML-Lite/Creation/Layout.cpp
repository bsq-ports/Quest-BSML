#include "BSML-Lite/Creation/Layout.hpp"
#include "logging.hpp"

#include "BSML/Components/Backgroundable.hpp"
#include "BSML/Components/ExternalComponents.hpp"
#include "BSML/Components/ScrollView.hpp"
#include "BSML/Components/ScrollViewContent.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/TextAnchor.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/UI/ScrollRect.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "UnityEngine/UI/Mask.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"

#include "HMUI/TextPageScrollView.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/EventSystemListener.hpp"
#include "HMUI/Touchable.hpp"
#include "HMUI/ImageView.hpp"

#include "Helpers/utilities.hpp"

#include "GlobalNamespace/EulaDisplayViewController.hpp"
#include "GlobalNamespace/GameplaySetupViewController.hpp"
#include "GlobalNamespace/ColorsOverrideSettingsPanelController.hpp"
#include "GlobalNamespace/ColorSchemeDropdown.hpp"

#include "VRUIControls/VRGraphicRaycaster.hpp"

#include "Helpers/getters.hpp"
#include "beatsaber-hook/shared/safeptr.hpp"

// Note: BSML-Lite owns layout/scroll-view/modal creation logic directly here.
// It no longer reaches into the corresponding Tags' protected CreateObject
// (previously via `#define protected public`).

namespace BSML::Lite {
    UnityEngine::UI::VerticalLayoutGroup* CreateVerticalLayoutGroup(const TransformWrapper& parent) {
        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLVerticalLayoutGroup");
        gameObject->get_transform()->SetParent(parent, false);
        auto vertical = gameObject->AddComponent<UnityEngine::UI::VerticalLayoutGroup*>();

        auto contentSizeFitter = gameObject->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        contentSizeFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        gameObject->AddComponent<BSML::Backgroundable*>();

        auto rectTransform = vertical->get_rectTransform();
        rectTransform->set_anchorMin({0, 0});
        rectTransform->set_anchorMax({1, 1});
        rectTransform->set_sizeDelta({0, 0});

        gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        return vertical;
    }

    UnityEngine::UI::HorizontalLayoutGroup* CreateHorizontalLayoutGroup(const TransformWrapper& parent) {
        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLHorizontalLayoutGroup");
        gameObject->get_transform()->SetParent(parent, false);
        auto horizontal = gameObject->AddComponent<UnityEngine::UI::HorizontalLayoutGroup*>();

        auto contentSizeFitter = gameObject->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        contentSizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        gameObject->AddComponent<BSML::Backgroundable*>();

        auto rectTransform = horizontal->get_rectTransform();
        rectTransform->set_anchorMin({0, 0});
        rectTransform->set_anchorMax({1, 1});
        rectTransform->set_sizeDelta({0, 0});

        gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        return horizontal;
    }

    UnityEngine::UI::GridLayoutGroup* CreateGridLayoutGroup(const TransformWrapper& parent) {
        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLGridLayoutGroup");
        gameObject->get_transform()->SetParent(parent, false);
        auto grid = gameObject->AddComponent<UnityEngine::UI::GridLayoutGroup*>();

        auto contentSizeFitter = gameObject->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        contentSizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        gameObject->AddComponent<BSML::Backgroundable*>();

        auto rectTransform = grid->get_rectTransform();
        rectTransform->set_anchorMin({0, 0});
        rectTransform->set_anchorMax({1, 1});
        rectTransform->set_sizeDelta({0, 0});

        gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        return grid;
    }

    HMUI::StackLayoutGroup* CreateStackLayoutGroup(const TransformWrapper& parent) {
        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLStackLayoutGroup");
        gameObject->get_transform()->SetParent(parent, false);
        auto stack = gameObject->AddComponent<HMUI::StackLayoutGroup*>();
        gameObject->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        gameObject->AddComponent<BSML::Backgroundable*>();

        auto rectTransform = gameObject->transform.cast<UnityEngine::RectTransform>();
        rectTransform->set_anchorMin({0, 0});
        rectTransform->set_anchorMax({1, 1});
        rectTransform->set_sizeDelta({0, 0});

        gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        return stack;
    }

    HMUI::TextPageScrollView* GetScrollViewTemplate() {
        static safe_ptr<HMUI::TextPageScrollView*> scrollViewTemplate;
        if (!scrollViewTemplate) {
            scrollViewTemplate = UnityEngine::Object::Instantiate(Helpers::GetDiContainer()->Resolve<GlobalNamespace::EulaDisplayViewController*>()->_textPageScrollView);
            scrollViewTemplate->set_name("BSMLScrollViewTemplate");
            scrollViewTemplate->SetText(nullptr);

            auto rectTransform = scrollViewTemplate->transform.cast<UnityEngine::RectTransform>();
            rectTransform->set_anchorMin({0, 0});
            rectTransform->set_anchorMax({1, 1});
            rectTransform->set_sizeDelta({0, 0});
        }
        return scrollViewTemplate.ptr();
    }

    UnityEngine::GameObject* CreateScrollView(const TransformWrapper& parent) {
        HMUI::TextPageScrollView* textScrollView = UnityEngine::Object::Instantiate(GetScrollViewTemplate(), parent);
        auto gameObject = textScrollView->gameObject;
        gameObject->SetActive(false);

        textScrollView->set_name("BSMLScrollView");
        auto pageUpButton = textScrollView->_pageUpButton;
        auto pageDownButton = textScrollView->_pageDownButton;
        auto verticalScrollIndicator = textScrollView->_verticalScrollIndicator;

        UnityEngine::RectTransform* viewport = textScrollView->_viewport;
        Helpers::GetDiContainer()->InstantiateComponent<VRUIControls::VRGraphicRaycaster*>(viewport->get_gameObject());

        UnityEngine::Object::Destroy(textScrollView->_text->get_gameObject());
        UnityEngine::Object::Destroy(textScrollView);
        gameObject->SetActive(false);

        BSML::ScrollView* scrollView = Helpers::GetDiContainer()->InstantiateComponent<BSML::ScrollView*>(gameObject);
        scrollView->_pageUpButton = pageUpButton;
        scrollView->_pageDownButton = pageDownButton;
        scrollView->_verticalScrollIndicator = verticalScrollIndicator;
        scrollView->_viewport = viewport;
        auto scrollTransform = scrollView->transform.cast<UnityEngine::RectTransform>();
        scrollTransform->set_anchorMin(UnityEngine::Vector2(0.0f, 0.0f));
        scrollTransform->set_anchorMax(UnityEngine::Vector2(1.0f, 1.0f));

        viewport->set_anchorMin(UnityEngine::Vector2(0, 0));
        viewport->set_anchorMax(UnityEngine::Vector2(1, 1));

        UnityEngine::GameObject* parentObject = UnityEngine::GameObject::New_ctor();
        parentObject->set_name("BSMLScrollViewContent");
        parentObject->get_transform()->SetParent(viewport, false);

        UnityEngine::UI::ContentSizeFitter* contentSize = parentObject->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        contentSize->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        contentSize->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);

        UnityEngine::UI::VerticalLayoutGroup* verticalLayout = parentObject->AddComponent<UnityEngine::UI::VerticalLayoutGroup*>();
        verticalLayout->set_childForceExpandHeight(false);
        verticalLayout->set_childForceExpandWidth(false);
        verticalLayout->set_childControlHeight(true);
        verticalLayout->set_childControlWidth(true);
        verticalLayout->set_childAlignment(UnityEngine::TextAnchor::UpperCenter);

        auto rectTransform = parentObject->transform.cast<UnityEngine::RectTransform>();
        rectTransform->set_anchorMin(UnityEngine::Vector2(0, 1));
        rectTransform->set_anchorMax(UnityEngine::Vector2(1, 1));
        rectTransform->set_sizeDelta(UnityEngine::Vector2(0, 0));
        rectTransform->set_pivot(UnityEngine::Vector2(0.5f, 1));

        UnityEngine::GameObject* child = UnityEngine::GameObject::New_ctor();
        child->set_name("BSMLScrollViewContentContainer");
        child->get_transform()->SetParent(rectTransform, false);

        parentObject->AddComponent<BSML::ScrollViewContent*>()->scrollView = scrollView;

        UnityEngine::UI::VerticalLayoutGroup* layoutGroup = child->AddComponent<UnityEngine::UI::VerticalLayoutGroup*>();
        layoutGroup->set_childControlHeight(false);
        layoutGroup->set_childForceExpandHeight(false);
        layoutGroup->set_childAlignment(UnityEngine::TextAnchor::LowerCenter);
        layoutGroup->set_spacing(0.5f);

        auto externalComponents = child->AddComponent<BSML::ExternalComponents*>();
        externalComponents->components->Add(scrollView);
        externalComponents->components->Add(scrollView->get_transform());
        externalComponents->components->Add(gameObject->AddComponent<UnityEngine::UI::LayoutElement*>());

        child->transform.cast<UnityEngine::RectTransform>()->set_sizeDelta(UnityEngine::Vector2(0, -1));

        scrollView->_contentRectTransform = parentObject->transform.cast<UnityEngine::RectTransform>();
        gameObject->SetActive(true);
        return child;
    }

    UnityEngine::GameObject* CreateScrollableSettingsContainer(const TransformWrapper& parent) {
        auto go = CreateScrollView(parent);
        auto externalComponents = go->GetComponent<BSML::ExternalComponents*>();
        auto scrollTransform = externalComponents->Get<UnityEngine::RectTransform*>();
        scrollTransform->set_anchorMin(UnityEngine::Vector2(0.0f, 0.0f));
        scrollTransform->set_anchorMax(UnityEngine::Vector2(1.0f, 1.0f));
        scrollTransform->set_anchoredPosition(UnityEngine::Vector2(2.0f, 6.0f));
        scrollTransform->set_sizeDelta(UnityEngine::Vector2(0.0f, -20.0f));
        go->set_name("BSMLScrollableSettingsContainer");
        return go;
    }

    namespace {
        HMUI::ModalView* GetModalViewTemplate() {
            static safe_ptr<HMUI::ModalView*> modalViewTemplate;
            if (!modalViewTemplate) {
                modalViewTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::GameplaySetupViewController*>()->_colorsOverrideSettingsPanelController->_colorSchemeDropDown->_modalView.cast<HMUI::ModalView>();
            }
            return modalViewTemplate.ptr();
        }
    }

    BSML::ModalView* CreateModal(const TransformWrapper& parent, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, std::function<void()> onBlockerClicked, bool dismissOnBlockerClicked) {
        auto modalViewTemplate = GetModalViewTemplate();
        auto copy = Helpers::GetDiContainer()->InstantiatePrefab(modalViewTemplate, parent);
        auto gameObject = copy->get_gameObject();
        gameObject->SetActive(false);
        gameObject->set_name("BSMLModalView");
        // we use our own custom modalView type, this differs from PC BSML but it just makes it easier to set things up
        auto modalView = Helpers::GetDiContainer()->InstantiateComponent<BSML::ModalView*>(gameObject);

        modalView->_presentPanelAnimations = modalViewTemplate->_presentPanelAnimations;
        modalView->_dismissPanelAnimation = modalViewTemplate->_dismissPanelAnimation;

        UnityEngine::Object::DestroyImmediate(gameObject->GetComponent<HMUI::TableView*>());
        UnityEngine::Object::DestroyImmediate(gameObject->GetComponent<UnityEngine::UI::ScrollRect*>());
        UnityEngine::Object::DestroyImmediate(gameObject->GetComponent<HMUI::ScrollView*>());
        UnityEngine::Object::DestroyImmediate(gameObject->GetComponent<HMUI::EventSystemListener*>());

        auto rectTransform = modalView->transform.cast<UnityEngine::RectTransform>();
        int childCount = rectTransform->get_childCount();

        for (int i = 0; i < childCount; i++) {
            auto child = rectTransform->GetChild(i).cast<UnityEngine::RectTransform>();
            DEBUG("child name: {}", child->get_name());

            if (child->get_name() == "BG") {
                DEBUG("twas BG");
                child->set_anchoredPosition({0, 0});
                child->set_sizeDelta({0, 0});
                child->GetComponent<UnityEngine::UI::Image*>()->set_raycastTarget(true);
                child->get_gameObject()->SetActive(true);
            } else {
                DEBUG("Destroyed!");
                UnityEngine::Object::Destroy(child->get_gameObject());
            }
        }

        rectTransform->set_anchorMin({.5f, .5f});
        rectTransform->set_anchorMax({.5f, .5f});
        rectTransform->set_sizeDelta({0, 0});

        auto externalComponents = gameObject->AddComponent<BSML::ExternalComponents*>();
        externalComponents->Add(rectTransform);
        externalComponents->Add(modalView);

        rectTransform->set_anchoredPosition(anchoredPosition);
        rectTransform->set_sizeDelta(sizeDelta);

        modalView->dismissOnBlockerClicked = dismissOnBlockerClicked;
        modalView->onHide = onBlockerClicked;

        return modalView;
    }

    UnityEngine::GameObject* CreateScrollableModalContainer(BSML::ModalView* modal) {
        auto rect = modal->transform.cast<UnityEngine::RectTransform>();
        auto sizeDelta = rect->get_sizeDelta();
        float width = sizeDelta.x;
        float height = sizeDelta.y;

        UnityEngine::Transform* parent = modal->transform;
        auto content = CreateScrollView(parent);

        auto externalComponents = content->GetComponent<BSML::ExternalComponents*>();
        auto scrollTransform = externalComponents->Get<UnityEngine::RectTransform*>();

        scrollTransform->set_anchoredPosition({-2.5f, 0.0f});
        scrollTransform->set_sizeDelta({7.5f, 0.0f});

        auto layout = content->GetComponent<UnityEngine::UI::VerticalLayoutGroup*>();

        layout->set_childControlWidth(true);
        layout->set_childForceExpandWidth(true);

        auto layoutelem = content->AddComponent<UnityEngine::UI::LayoutElement*>();
        layoutelem->set_preferredWidth(width - 10.0f);

        static ConstString name("BSMLScrollableModalContainer");

        scrollTransform->get_gameObject()->set_name(name);
        return content;
    }

    UnityEngine::UI::VerticalLayoutGroup* CreateModifierContainer(const TransformWrapper& parent) {
        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLModifierContainer");
        gameObject->get_transform()->SetParent(parent, false);

        auto vertical = gameObject->AddComponent<UnityEngine::UI::VerticalLayoutGroup*>();
        vertical->set_padding(UnityEngine::RectOffset::New_ctor(3, 3, 2, 2));
        vertical->set_childControlHeight(false);
        vertical->set_childForceExpandHeight(false);

        gameObject->AddComponent<UnityEngine::UI::ContentSizeFitter*>()->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        // I saw that this one was not there on pc, but I want people to be able to use it anyways, it doesn't do anything unless you tell it to anyways
        gameObject->AddComponent<BSML::Backgroundable*>();

        auto rectTransform = vertical->get_rectTransform();
        rectTransform->set_anchoredPosition({0, 3});
        rectTransform->set_anchorMin({0.5f, 0.5f});
        rectTransform->set_anchorMax({0.5f, 0.5f});
        rectTransform->set_sizeDelta({54, 3});

        gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        return vertical;
    }

    UnityEngine::GameObject* CreateScrollableContainer(const TransformWrapper& parent) {
        auto go = UnityEngine::GameObject::New_ctor("BSMLScrollableContainer");
        go->SetActive(false);

        auto transform = go->AddComponent<UnityEngine::RectTransform*>();
        transform->SetParent(parent, false);
        transform->set_localPosition({0, 0, 0});
        transform->set_anchorMin({0, 0});
        transform->set_anchorMax({1, 1});
        transform->set_anchoredPosition({0, 0});
        transform->set_sizeDelta({0, 0});

        auto vpgo = UnityEngine::GameObject::New_ctor("Viewport");
        auto viewport = vpgo->AddComponent<UnityEngine::RectTransform*>();
        viewport->SetParent(transform, false);
        viewport->set_localPosition({0, 0, 0});
        viewport->set_anchorMin({0, 0});
        viewport->set_anchorMax({1, 1});
        viewport->set_anchoredPosition({0, 0});
        viewport->set_sizeDelta({0, 0});

        auto vpMask = vpgo->AddComponent<UnityEngine::UI::Mask*>();
        auto vpImage = vpgo->AddComponent<HMUI::ImageView*>();
        vpMask->set_showMaskGraphic(false);
        vpImage->set_color({1, 1, 1, 1});
        vpImage->set_sprite(Utilities::ImageResources::GetWhitePixel());
        vpImage->set_material(Helpers::GetUINoGlowMat());

        auto contentGo = UnityEngine::GameObject::New_ctor("Content Wrapper");
        auto content = contentGo->AddComponent<UnityEngine::RectTransform*>();
        content->SetParent(viewport, false);
        content->set_localPosition({0, 0, 0});
        content->set_anchorMin({0, 1});
        content->set_anchorMax({1, 1});
        content->set_anchoredPosition({0, 0});
        content->set_sizeDelta({0, 0});
        content->set_pivot({0.5, 1});

        auto contentFitter = contentGo->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        contentFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained);
        contentFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);

        auto layout = contentGo->AddComponent<UnityEngine::UI::VerticalLayoutGroup*>();
        layout->set_childControlHeight(false);
        layout->set_childForceExpandHeight(false);
        layout->set_childForceExpandWidth(false);

        go->AddComponent<HMUI::Touchable*>(); // Required by EventSystemListener
        go->AddComponent<HMUI::EventSystemListener*>(); // Required by ScrollView
        auto scrollView = go->AddComponent<BSML::ScrollableContainer*>();
        scrollView->_contentRectTransform = content;
        scrollView->_viewport = viewport;
        scrollView->_platformHelper = Helpers::GetIVRPlatformHelper();
        scrollView->_xrSystemState = Helpers::GetIXRSystemState();

        auto externalComponents = contentGo->AddComponent<BSML::ExternalComponents*>();
        externalComponents->Add(scrollView);
        externalComponents->Add(transform);
        auto layoutElement = go->AddComponent<UnityEngine::UI::LayoutElement*>();
        layoutElement->set_minWidth(-1);
        layoutElement->set_preferredWidth(-1);
        layoutElement->set_flexibleWidth(0);

        externalComponents->Add(layoutElement);

        go->SetActive(true);
        return contentGo;
    }
}
