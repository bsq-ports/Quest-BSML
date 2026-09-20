#include "BSML-Lite/Creation/Misc.hpp"
#include "BSML-Lite/Creation/Layout.hpp"

#include "Helpers/getters.hpp"
#include "Helpers/creation.hpp"
#include "Helpers/delegates.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/UI/CanvasScaler.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/AdditionalCanvasShaderChannels.hpp"
#include "System/Action_2.hpp"
#include "VRUIControls/VRGraphicRaycaster.hpp"
#include "beatsaber-hook/shared/listw.hpp"
#include "beatsaber-hook/shared/safeptr.hpp"

#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "GlobalNamespace/BeatmapDifficultySegmentedControlController.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSegmentedControlController.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/LoadingControl.hpp"
#include "GlobalNamespace/PlatformLeaderboardViewController.hpp"
#include "GlobalNamespace/LeaderboardTableView.hpp"
#include "GlobalNamespace/LeaderboardTableCell.hpp"
#include "GlobalNamespace/PlayerStatisticsViewController.hpp"
#include "Zenject/DiContainer.hpp"
#include "HMUI/VerticalScrollIndicator.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/IconSegmentedControl.hpp"
#include "HMUI/TextPageScrollView.hpp"
#include "TMPro/TextMeshProUGUI.hpp"

#include "BSML/Components/Backgroundable.hpp"
#include "BSML/Components/ExternalComponents.hpp"
#include "BSML/Components/TextPageScrollViewRefresher.hpp"
#include "BSML/Components/ScrollIndicator.hpp"
#include "BSML/Components/TabSelector.hpp"
#include "BSML/Components/Tab.hpp"

// Note: BSML-Lite owns text-segmented-control creation logic directly here. It
// no longer reaches into BSML::TextSegmentedControlTag's protected CreateObject
// (previously via `#define protected public`).

namespace BSML::Lite {
    namespace {
        HMUI::TextSegmentedControl* GetTextSegmentedControlTemplate() {
            static safe_ptr<HMUI::TextSegmentedControl*> textSegmentedControlTemplate;
            if (!textSegmentedControlTemplate) {
                textSegmentedControlTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::StandardLevelDetailViewController*>()->_standardLevelDetailView->_beatmapDifficultySegmentedControlController->GetComponent<HMUI::TextSegmentedControl*>();
            }
            return textSegmentedControlTemplate.ptr();
        }
    }

    HMUI::HoverHint* AddHoverHint(const GameObjectWrapper& parent, StringW text) {
        return BSML::Helpers::AddHoverHint(parent, text);
    }

    BSML::FloatingScreen* CreateFloatingScreen(UnityEngine::Vector2 screenSize, UnityEngine::Vector3 position, UnityEngine::Vector3 rotation, float curvatureRadius, bool hasBackground, bool createHandle, BSML::Side handleSide) {
        auto screen = BSML::FloatingScreen::CreateFloatingScreen(screenSize, createHandle, position, UnityEngine::Quaternion::Euler(rotation), curvatureRadius, hasBackground);
        screen->set_HandleSide(handleSide);

        return screen;
    }

    BSML::ProgressBar* CreateProgressBar(UnityEngine::Vector3 position, UnityEngine::Vector3 rotation, UnityEngine::Vector3 scale, StringW headerText, StringW subText1, StringW subText2) {
        return BSML::ProgressBar::CreateProgressBar(position, scale, rotation, headerText, subText1, subText2);
    }

    HMUI::TextSegmentedControl* CreateTextSegmentedControl(const TransformWrapper& parent, UnityEngine::Vector2 anchoredPosition, std::optional<UnityEngine::Vector2> sizeDelta, std::span<std::string_view> values, std::function<void(int)> onCellWithIdxClicked) {
        DEBUG("Creating TextSegmentedControl");

        auto templateControl = GetTextSegmentedControlTemplate();
        auto go = Helpers::GetDiContainer()->InstantiatePrefab(templateControl, parent);
        auto textSegmentedControl = go->GetComponent<HMUI::TextSegmentedControl*>();
        textSegmentedControl->_dataSource = nullptr;

        go->set_name("BSMLTextSegmentedControl");

        auto rect = go->transform.cast<UnityEngine::RectTransform>();
        int childCount = rect->get_childCount();
        for (int i = 1; i <= childCount; i++) {
            UnityEngine::Object::DestroyImmediate(rect->GetChild(childCount - i)->get_gameObject());
        }

        UnityEngine::Object::Destroy(go->GetComponent<GlobalNamespace::BeatmapDifficultySegmentedControlController*>());
        go->SetActive(true);

        rect->set_anchoredPosition(anchoredPosition);
        if (sizeDelta) rect->set_sizeDelta(*sizeDelta);

        auto texts = ListW<StringW>::New();
        texts->EnsureCapacity(values.size());
        for (const auto& text : values) texts->Add(text);
        textSegmentedControl->SetTexts(*texts, nullptr);

        if (onCellWithIdxClicked) {
            textSegmentedControl->add_didSelectCellEvent(
                custom_types::MakeDelegate<System::Action_2<UnityW<HMUI::SegmentedControl>, int>*>(
                    std::function<void(HMUI::SegmentedControl*, int)>( [onCellWithIdxClicked](HMUI::SegmentedControl* _, int idx) { onCellWithIdxClicked(idx); } )
                )
            );
        }

        return textSegmentedControl;
    }

    static inline UnityEngine::AdditionalCanvasShaderChannels operator |(UnityEngine::AdditionalCanvasShaderChannels a, UnityEngine::AdditionalCanvasShaderChannels b) {
        return UnityEngine::AdditionalCanvasShaderChannels(a.value__ | b.value__);
    }

    UnityEngine::GameObject* CreateCanvas() {
        static ConstString name("BSMLCanvas");
        auto go = UnityEngine::GameObject::New_ctor(name);
        go->set_layer(5);
        auto cv = go->AddComponent<UnityEngine::Canvas*>();
        cv->set_additionalShaderChannels(UnityEngine::AdditionalCanvasShaderChannels::TexCoord1 | UnityEngine::AdditionalCanvasShaderChannels::TexCoord2);
        cv->set_sortingOrder(4);

        auto scaler = go->AddComponent<UnityEngine::UI::CanvasScaler*>();
        scaler->set_scaleFactor(1.0f);
        scaler->set_dynamicPixelsPerUnit(3.44f);
        scaler->set_referencePixelsPerUnit(10.0f);

        auto* physicsRaycaster = Helpers::GetPhysicsRaycasterWithCache();
        if(physicsRaycaster)
            go->AddComponent<VRUIControls::VRGraphicRaycaster*>()->_physicsRaycaster = physicsRaycaster;

        auto rectTransform = go->GetComponent<UnityEngine::RectTransform*>();
        float scale = 1.5f * 0.02f; //Wrapper->ScreenSystem: 1.5 Wrapper->ScreenSystem->ScreenContainer: 0.02
        rectTransform->set_localScale(UnityEngine::Vector3(scale, scale, scale));
        return go;
    }

    UnityEngine::GameObject* CreateLoadingIndicator(const TransformWrapper& parent) {
        static safe_ptr<UnityEngine::GameObject*> loadingTemplate;
        if (!loadingTemplate) {
            loadingTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::LevelCollectionNavigationController*>()->_loadingControl->_loadingContainer->transform->Find("LoadingIndicator")->get_gameObject();
        }
        if (!loadingTemplate) {
            ERROR("No loading template found!");
            return nullptr;
        }

        auto gameObject = UnityEngine::Object::Instantiate(loadingTemplate.ptr(), parent, false);
        gameObject->set_name("BSMLLoadingIndicator");

        gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        return gameObject;
    }

    BSML::ScrollIndicator* CreateScrollIndicator(const TransformWrapper& parent) {
        static safe_ptr<HMUI::VerticalScrollIndicator*> scrollIndicatorTemplate;
        if (!scrollIndicatorTemplate) {
            scrollIndicatorTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::LevelCollectionNavigationController*>()->_levelCollectionViewController->_levelCollectionTableView->_tableView->get_scrollView()->_verticalScrollIndicator;
        }

        DEBUG("making ScrollIndicator");
        auto gameObj = UnityEngine::Object::Instantiate(scrollIndicatorTemplate->get_gameObject(), parent, false);
        DEBUG("instantiated {}", gameObj->get_name());

        gameObj->SetActive(false);
        gameObj->set_name("BSMLVerticalScrollIndicator");

        auto transform = gameObj->GetComponent<UnityEngine::RectTransform*>();
        transform->SetParent(parent, false);

        UnityEngine::Object::DestroyImmediate(gameObj->GetComponent<HMUI::VerticalScrollIndicator*>());

        auto indicator = gameObj->AddComponent<BSML::ScrollIndicator*>();
        indicator->set_Handle(transform->GetChild(0)->GetComponent<UnityEngine::RectTransform*>());

        gameObj->SetActive(true);

        return indicator;
    }

    namespace {
        HMUI::IconSegmentedControl* StripIconSegmentedControlTemplate(HMUI::IconSegmentedControl* templateControl, const TransformWrapper& parent, std::string_view name) {
            auto gameObject = Helpers::GetDiContainer()->InstantiatePrefab(templateControl, parent);
            gameObject->SetActive(false);
            gameObject->set_name(name);

            auto iconSegmentedControl = gameObject->GetComponent<HMUI::IconSegmentedControl*>();
            iconSegmentedControl->_dataSource = nullptr;

            auto transform = gameObject->transform.cast<UnityEngine::RectTransform>();
            transform->set_anchoredPosition({0, 0});
            int childCount = transform->get_childCount();
            for (int i = 1; i <= childCount; i++) {
                UnityEngine::Object::DestroyImmediate(transform->GetChild(childCount - i)->get_gameObject());
            }
            return iconSegmentedControl;
        }
    }

    HMUI::IconSegmentedControl* CreateIconSegmentedControl(const TransformWrapper& parent) {
        static safe_ptr<HMUI::IconSegmentedControl*> iconSegmentedControlTemplate;
        if (!iconSegmentedControlTemplate) {
            iconSegmentedControlTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::StandardLevelDetailViewController*>()->_standardLevelDetailView->_beatmapCharacteristicSegmentedControlController->GetComponent<HMUI::IconSegmentedControl*>();
        }

        auto iconSegmentedControl = StripIconSegmentedControlTemplate(iconSegmentedControlTemplate.ptr(), parent, "BSMLIconSegmentedControl");
        UnityEngine::Object::Destroy(iconSegmentedControl->GetComponent<GlobalNamespace::BeatmapCharacteristicSegmentedControlController*>());
        iconSegmentedControl->get_gameObject()->SetActive(true);
        return iconSegmentedControl;
    }

    HMUI::IconSegmentedControl* CreateVerticalIconSegmentedControl(const TransformWrapper& parent) {
        static safe_ptr<HMUI::IconSegmentedControl*> verticalIconSegmentedControlTemplate;
        if (!verticalIconSegmentedControlTemplate) {
            verticalIconSegmentedControlTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::PlatformLeaderboardViewController*>()->_scopeSegmentedControl;
        }

        auto verticalIconSegmentedControlTemplateVal = verticalIconSegmentedControlTemplate.ptr();
        auto gameObject = Helpers::GetDiContainer()->InstantiatePrefab(verticalIconSegmentedControlTemplateVal, parent);
        auto verticalIconSegmentedControl = gameObject->GetComponent<HMUI::IconSegmentedControl*>();
        verticalIconSegmentedControl->_dataSource = nullptr;

        gameObject->set_name("BSMLVerticalIconSegmentedControl");

        auto transform = gameObject->transform.cast<UnityEngine::RectTransform>();
        transform->set_anchorMin({0.5f, 0.5f});
        transform->set_anchorMax({0.5f, 0.5f});
        transform->set_anchoredPosition({0, 0});
        transform->set_pivot({0.5f, 0.5f});

        int childCount = transform->get_childCount();
        for (int i = 1; i <= childCount; i++) {
            UnityEngine::Object::DestroyImmediate(transform->GetChild(childCount - i)->get_gameObject());
        }

        gameObject->SetActive(true);
        return verticalIconSegmentedControl;
    }

    GlobalNamespace::LeaderboardTableView* CreateLeaderboard(const TransformWrapper& parent) {
        static safe_ptr<GlobalNamespace::LeaderboardTableView*> leaderboardTemplate;
        if (!leaderboardTemplate) {
            leaderboardTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::PlatformLeaderboardViewController*>()->_leaderboardTableView;
        }

        auto table = UnityEngine::Object::Instantiate(leaderboardTemplate.ptr(), parent, false);
        table->set_name("BSMLLeaderboard");
        table->_cellPrefab->_scoreText->set_enableWordWrapping(false);
        table->GetComponent<VRUIControls::VRGraphicRaycaster*>()->_physicsRaycaster = Helpers::GetPhysicsRaycasterWithCache();
        for (auto tableCell : table->GetComponentsInChildren<GlobalNamespace::LeaderboardTableCell*>())
            UnityEngine::Object::Destroy(tableCell->get_gameObject());

        auto loadingControl = table->GetComponentInChildren<GlobalNamespace::LoadingControl*>();
        if (loadingControl) loadingControl->Hide();

        return table;
    }

    UnityEngine::GameObject* CreateTabSelector(const TransformWrapper& parent) {
        static safe_ptr<HMUI::TextSegmentedControl*> tabSelectorTagTemplate;
        if (!tabSelectorTagTemplate) {
            tabSelectorTagTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::PlayerStatisticsViewController*>()->_statsScopeSegmentedControl;
        }
        if (!tabSelectorTagTemplate) {
            ERROR("No tabSelectorTagTemplate found!");
            return nullptr;
        }

        auto tabTemplate = tabSelectorTagTemplate.ptr();
        auto diContainer = tabTemplate->_container;
        auto textSegmentedControl = diContainer->InstantiatePrefabForComponent<HMUI::TextSegmentedControl*>(tabTemplate, parent);
        auto gameObject = textSegmentedControl->get_gameObject();

        gameObject->set_name("BSMLTabSelector");

        auto transform = gameObject->transform.cast<UnityEngine::RectTransform>();
        transform->set_anchoredPosition({0, 0});
        int childCount = transform->get_childCount();
        for (int i = 1; i <= childCount; i++) {
            UnityEngine::Object::DestroyImmediate(transform->GetChild(childCount - i)->get_gameObject());
        }

        gameObject->AddComponent<BSML::TabSelector*>()->textSegmentedControl = textSegmentedControl;
        gameObject->SetActive(true);

        return gameObject;
    }

    BSML::Tab* CreateTab(const TransformWrapper& parent) {
        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLTab");
        gameObject->get_transform()->SetParent(parent, false);
        gameObject->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        gameObject->AddComponent<BSML::Backgroundable*>();

        auto rectTransform = gameObject->transform.cast<UnityEngine::RectTransform>();
        rectTransform->set_anchorMin({0, 0});
        rectTransform->set_anchorMax({1, 1});
        rectTransform->set_sizeDelta({0, 0});

        return gameObject->AddComponent<BSML::Tab*>();
    }

    HMUI::TextPageScrollView* CreateTextPageScrollView(const TransformWrapper& parent) {
        HMUI::TextPageScrollView* scrollView = Helpers::GetDiContainer()->InstantiatePrefabForComponent<HMUI::TextPageScrollView*>(GetScrollViewTemplate(), parent);

        scrollView->set_name("BSMLTextScrollPageView");
        scrollView->set_enabled(true);

        TMPro::TextMeshProUGUI* textMesh = scrollView->_text;
        textMesh->set_text("Default Text");

        textMesh->get_gameObject()->AddComponent<BSML::TextPageScrollViewRefresher*>()->scrollView = scrollView;

        auto externalComponents = scrollView->get_gameObject()->AddComponent<BSML::ExternalComponents*>();
        externalComponents->Add(textMesh);

        return scrollView;
    }
}
