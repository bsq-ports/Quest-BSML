#include "BSML-Lite/Creation/Lists.hpp"
#include "BSML-Lite/Creation/Layout.hpp"
#include "BSML-Lite/Creation/Buttons.hpp"
#include "BSML-Lite/Creation/Image.hpp"
#include "assets.hpp"

#include "Helpers/getters.hpp"
#include "Helpers/utilities.hpp"
#include "Helpers/delegates.hpp"
#include "logging.hpp"
#include <span>

#include "BSML/Components/TableView.hpp"
#include "BSML/Components/CustomCellListTableData.hpp"
#include "System/Action_2.hpp"

#include "HMUI/ScrollView.hpp"
#include "HMUI/Touchable.hpp"
#include "HMUI/EventSystemListener.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/ScrollRect.hpp"
#include "UnityEngine/UI/RectMask2D.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/RenderMode.hpp"
#include "UnityEngine/AdditionalCanvasShaderChannels.hpp"
#include "VRUIControls/VRGraphicRaycaster.hpp"

#include "GlobalNamespace/NoteJumpStartBeatOffsetDropdown.hpp"
#include "GlobalNamespace/PlayerSettingsPanelController.hpp"
#include "GlobalNamespace/GameplaySetupViewController.hpp"
#include "HMUI/SimpleTextDropdown.hpp"

#include "beatsaber-hook/shared/safeptr.hpp"

// Note: BSML-Lite owns list creation logic directly here. It no longer
// reaches into BSML::ListTag's protected CreateObject (previously via
// `#define protected public`).

namespace BSML::Lite {
    namespace {
        UnityEngine::Canvas* GetListCanvasTemplate() {
            static safe_ptr<UnityEngine::Canvas*> listCanvasTemplate;
            if (!listCanvasTemplate) {
                listCanvasTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::GameplaySetupViewController*>()->_playerSettingsPanelController->_noteJumpStartBeatOffsetDropdown->_simpleTextDropdown->_tableView->GetComponent<UnityEngine::Canvas*>();
            }
            return listCanvasTemplate.ptr();
        }
    }

    BSML::CustomListTableData* CreateList(const TransformWrapper& parent, std::optional<UnityEngine::Vector2> anchoredPosition, std::optional<UnityEngine::Vector2> sizeDelta, std::function<void(int)> onCellWithIdxClicked, bool activate) {
        DEBUG("Creating List");
        auto container = UnityEngine::GameObject::New_ctor("BSMLListContainer")->AddComponent<UnityEngine::RectTransform*>();
        auto containerGameObject = container->get_gameObject();
        containerGameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        containerGameObject->get_transform()->SetParent(parent, false);

        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLList");
        gameObject->get_transform()->SetParent(containerGameObject->get_transform(), false);
        gameObject->SetActive(false);

        auto listCanvasTemplate = GetListCanvasTemplate();
        auto scrollRect = gameObject->AddComponent<UnityEngine::UI::ScrollRect*>();
        auto canvas = gameObject->AddComponent<UnityEngine::Canvas*>();
        // using this method causes the list to have it's cells be squished:
        //Utilities::AddComponent(gameObject, listCanvasTemplate);
        // therefore we just copy what we need:
        canvas->set_additionalShaderChannels(listCanvasTemplate->get_additionalShaderChannels());
        canvas->set_overrideSorting(listCanvasTemplate->get_overrideSorting());
        canvas->set_pixelPerfect(listCanvasTemplate->get_pixelPerfect());
        canvas->set_referencePixelsPerUnit(listCanvasTemplate->get_referencePixelsPerUnit());
        canvas->set_renderMode(listCanvasTemplate->get_renderMode());
        canvas->set_scaleFactor(listCanvasTemplate->get_scaleFactor());
        canvas->set_sortingLayerID(listCanvasTemplate->get_sortingLayerID());
        canvas->set_sortingOrder(listCanvasTemplate->get_sortingOrder());
        canvas->set_worldCamera(listCanvasTemplate->get_worldCamera());

        gameObject->AddComponent<VRUIControls::VRGraphicRaycaster*>()->_physicsRaycaster = Helpers::GetPhysicsRaycasterWithCache();
        gameObject->AddComponent<HMUI::Touchable*>();
        gameObject->AddComponent<HMUI::EventSystemListener*>();

        auto scrollView = Helpers::GetDiContainer()->InstantiateComponent<HMUI::ScrollView*>(gameObject);

        HMUI::TableView* tableView = gameObject->AddComponent<BSML::TableView*>();
        auto tableData = container->get_gameObject()->AddComponent<BSML::CustomListTableData*>();
        tableData->tableView = tableView;

        tableView->_preallocatedCells = ArrayW<HMUI::TableView::CellsGroup*>(il2cpp_array_size_t(0));
        tableView->_isInitialized = false;
        tableView->_scrollView = scrollView;

        auto viewPort = UnityEngine::GameObject::New_ctor("ViewPort")->AddComponent<UnityEngine::RectTransform*>();
        viewPort->SetParent(gameObject->get_transform(), false);
        viewPort->get_gameObject()->AddComponent<UnityEngine::UI::RectMask2D*>();
        scrollRect->set_viewport(viewPort);

        auto content = UnityEngine::GameObject::New_ctor("Content")->AddComponent<UnityEngine::RectTransform*>();
        content->SetParent(viewPort, false);

        scrollView->_contentRectTransform = content;
        scrollView->_viewport = viewPort;

        viewPort->set_anchorMin({0, 0});
        viewPort->set_anchorMax({1, 1});
        viewPort->set_anchoredPosition({0, 0});
        viewPort->set_sizeDelta({0, 0});

        auto tableViewRectTransform = tableView->transform.cast<UnityEngine::RectTransform>();
        tableViewRectTransform->set_anchorMin({0, 0});
        tableViewRectTransform->set_anchorMax({1, 1});
        tableViewRectTransform->set_anchoredPosition({0, 0});
        tableViewRectTransform->set_sizeDelta({0, 0});

        tableView->SetDataSource(tableData->i_IDataSource(), false);

        tableData->tableView->scrollView->_platformHelper = BSML::Helpers::GetIVRPlatformHelper();
        tableData->tableView->scrollView->_xrSystemState = BSML::Helpers::GetIXRSystemState();

        auto rect = containerGameObject->transform.cast<UnityEngine::RectTransform>();
        if (anchoredPosition) rect->set_anchoredPosition(*anchoredPosition);
        if (sizeDelta) {
            rect->set_sizeDelta(*sizeDelta);

            auto layoutElement = containerGameObject->GetComponent<UnityEngine::UI::LayoutElement*>();
            if (layoutElement) {
                layoutElement->set_preferredHeight(sizeDelta->y);
                layoutElement->set_flexibleHeight(sizeDelta->y);
                layoutElement->set_minHeight(sizeDelta->y);
                layoutElement->set_preferredWidth(sizeDelta->x);
                layoutElement->set_flexibleWidth(sizeDelta->x);
                layoutElement->set_minWidth(sizeDelta->x);
            }
        }

        if (onCellWithIdxClicked) {
            tableView->add_didSelectCellWithIdxEvent(
                custom_types::MakeDelegate<System::Action_2<UnityW<HMUI::TableView>, int>*>(
                    std::function<void(HMUI::TableView*, int)>(
                        [onCellWithIdxClicked](HMUI::TableView* _, int idx){ onCellWithIdxClicked(idx); }
                    )
                )
            );
        }

        // the inner list GameObject starts inactive (see gameObject->SetActive(false) above)
        if (activate) tableView->gameObject->SetActive(true);

        return tableData;
    }

    UnityEngine::Sprite* get_carat_down() {
        static safe_ptr<UnityEngine::Sprite*> carat_down;
        if (!carat_down) {
            auto sprite = BSML::Utilities::LoadSpriteRaw(ArrayW<uint8_t>(Assets::Images::CaratDown));
            UnityEngine::Object::DontDestroyOnLoad(sprite);
            carat_down = sprite;
        }
        return carat_down.ptr();
    }

    UnityEngine::Sprite* get_carat_up() {
        static safe_ptr<UnityEngine::Sprite*> carat_up;
        if (!carat_up) {
            auto sprite = BSML::Utilities::LoadSpriteRaw(ArrayW<uint8_t>(Assets::Images::CaratUp));
            UnityEngine::Object::DontDestroyOnLoad(sprite);
            carat_up = sprite;
        }
        return carat_up.ptr();
    }

    BSML::CustomListTableData* CreateScrollableList(const TransformWrapper& parent, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, std::function<void(int)> onCellWithIdxClicked) {
        auto vertical = CreateVerticalLayoutGroup(parent);
        auto layout = vertical->gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        auto rect = vertical->rectTransform;

        rect->anchoredPosition = anchoredPosition;
        rect->sizeDelta = sizeDelta;

        vertical->childForceExpandHeight = false;
        vertical->childControlHeight = false;
        vertical->childScaleHeight = false;

        layout->preferredHeight = sizeDelta.y;
        layout->preferredWidth = sizeDelta.x;

        auto list = CreateList(rect, UnityEngine::Vector2{0, 0}, UnityEngine::Vector2{sizeDelta.x, sizeDelta.y - 16}, onCellWithIdxClicked);
        auto pageUp = CreateClickableImage(vertical, get_carat_up(), [scrollView = list->tableView->scrollView.unsafe_ptr()](){
            if (scrollView && scrollView->m_CachedPtr.m_value) scrollView->PageUpButtonPressed();
        });
        pageUp->preserveAspect = true;
        pageUp->highlightColor = {1.0, 1.0, 1.0, 0.5};
        pageUp->transform->SetAsFirstSibling();
        auto pageDown = CreateClickableImage(vertical, get_carat_down(), [scrollView = list->tableView->scrollView.unsafe_ptr()](){
            if (scrollView && scrollView->m_CachedPtr.m_value) scrollView->PageDownButtonPressed();
        });
        pageDown->preserveAspect = true;
        pageDown->highlightColor = {1.0, 1.0, 1.0, 0.5};

        pageUp->rectTransform->sizeDelta = {8.0f, 8.0f};
        pageDown->rectTransform->sizeDelta = {8.0f, 8.0f};

        pageUp->gameObject->SetActive(true);
        pageDown->gameObject->SetActive(true);

        return list;
    }

    HMUI::TableView::IDataSource* CreateCustomSourceList(System::Type* type, const TransformWrapper& parent, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, std::function<void(int)> onCellWithIdxClicked) {
        auto list = CreateList(parent, anchoredPosition, sizeDelta, onCellWithIdxClicked);

        auto tableView = list->tableView;
        auto go = list->get_gameObject();
        UnityEngine::Object::DestroyImmediate(list);

        auto dataSource = go->AddComponent(type);
        auto iDataSource = i2c::cast<HMUI::TableView::IDataSource*>(dataSource.ptr());

        auto finfo = i2c::functions::class_get_field_from_name(dataSource->klass, "tableView");
        if (finfo) // there is a field named tableView
            i2c::set_field(dataSource, finfo, tableView);
        tableView->SetDataSource(iDataSource, false);

        return iDataSource;
    }

    HMUI::TableView::IDataSource* CreateScrollableCustomSourceList(System::Type* type, const TransformWrapper& parent, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, std::function<void(int)> onCellWithIdxClicked) {
        auto list = CreateScrollableList(parent, anchoredPosition, sizeDelta, onCellWithIdxClicked);

        auto tableView = list->tableView;
        auto go = list->gameObject;

        UnityEngine::Object::DestroyImmediate(list);

        auto dataSource = go->AddComponent(type);
        auto iDataSource = reinterpret_cast<HMUI::TableView::IDataSource*>(dataSource.unsafe_ptr());
        auto finfo = i2c::functions::class_get_field_from_name(dataSource->klass, "tableView");
        if (finfo) // there is a field named tableView
            i2c::set_field(dataSource, finfo, tableView);
        tableView->SetDataSource(iDataSource, false);

        return iDataSource;
    }

    UnityEngine::GameObject* CreateCustomList(const TransformWrapper& parent, std::string_view bsmlString) {
        DEBUG("Creating Custom List");
        auto container = UnityEngine::GameObject::New_ctor("BSMLCustomListContainer")->AddComponent<UnityEngine::RectTransform*>();
        auto containerGameObject = container->get_gameObject();
        containerGameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        containerGameObject->get_transform()->SetParent(parent, false);

        auto gameObject = UnityEngine::GameObject::New_ctor("BSMLCustomList");
        gameObject->get_transform()->SetParent(containerGameObject->get_transform(), false);
        gameObject->SetActive(false);

        auto listCanvasTemplate = GetListCanvasTemplate();
        auto scrollRect = gameObject->AddComponent<UnityEngine::UI::ScrollRect*>();
        auto canvas = gameObject->AddComponent<UnityEngine::Canvas*>();
        canvas->set_additionalShaderChannels(listCanvasTemplate->get_additionalShaderChannels());
        canvas->set_overrideSorting(listCanvasTemplate->get_overrideSorting());
        canvas->set_pixelPerfect(listCanvasTemplate->get_pixelPerfect());
        canvas->set_referencePixelsPerUnit(listCanvasTemplate->get_referencePixelsPerUnit());
        canvas->set_renderMode(listCanvasTemplate->get_renderMode());
        canvas->set_scaleFactor(listCanvasTemplate->get_scaleFactor());
        canvas->set_sortingLayerID(listCanvasTemplate->get_sortingLayerID());
        canvas->set_sortingOrder(listCanvasTemplate->get_sortingOrder());
        canvas->set_worldCamera(listCanvasTemplate->get_worldCamera());

        gameObject->AddComponent<VRUIControls::VRGraphicRaycaster*>()->_physicsRaycaster = Helpers::GetPhysicsRaycasterWithCache();
        gameObject->AddComponent<HMUI::Touchable*>();
        gameObject->AddComponent<HMUI::EventSystemListener*>();

        auto scrollView = Helpers::GetDiContainer()->InstantiateComponent<HMUI::ScrollView*>(gameObject);

        HMUI::TableView* tableView = gameObject->AddComponent<BSML::TableView*>();
        auto tableData = containerGameObject->AddComponent<BSML::CustomCellListTableData*>();
        tableData->tableView = tableView;
        tableData->bsmlString = std::string(bsmlString);

        tableView->_preallocatedCells = ArrayW<HMUI::TableView::CellsGroup*>(il2cpp_array_size_t(0));
        tableView->_isInitialized = false;
        tableView->_scrollView = scrollView;

        auto viewPort = UnityEngine::GameObject::New_ctor("ViewPort")->AddComponent<UnityEngine::RectTransform*>();
        viewPort->SetParent(gameObject->get_transform(), false);
        viewPort->get_gameObject()->AddComponent<UnityEngine::UI::RectMask2D*>();
        scrollRect->set_viewport(viewPort);

        auto content = UnityEngine::GameObject::New_ctor("Content")->AddComponent<UnityEngine::RectTransform*>();
        content->SetParent(viewPort, false);

        scrollView->_contentRectTransform = content;
        scrollView->_viewport = viewPort;

        viewPort->set_anchorMin({0, 0});
        viewPort->set_anchorMax({1, 1});
        viewPort->set_anchoredPosition({0, 0});
        viewPort->set_sizeDelta({0, 0});

        auto tableViewRectTransform = tableView->transform.cast<UnityEngine::RectTransform>();
        tableViewRectTransform->set_anchorMin({0, 0});
        tableViewRectTransform->set_anchorMax({1, 1});
        tableViewRectTransform->set_anchoredPosition({0, 0});
        tableViewRectTransform->set_sizeDelta({0, 0});

        tableView->SetDataSource(tableData->i_IDataSource(), false);
        return containerGameObject;
    }
}
