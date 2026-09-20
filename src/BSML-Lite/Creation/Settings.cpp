#include "BSML-Lite/Creation/Settings.hpp"

#include "BSML/Components/ExternalComponents.hpp"
#include "BSML/Components/Settings/IncDecSetting.hpp"
#include "BSML/Components/Settings/SliderSettingBase.hpp"
#include "BSML/Components/ModalView.hpp"
#include "BSML.hpp"

#include "custom-types/shared/delegate.hpp"
#include "Helpers/delegates.hpp"
#include "Helpers/getters.hpp"
#include "Helpers/utilities.hpp"
#include "logging.hpp"

#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/CanvasGroup.hpp"
#include "UnityEngine/Events/UnityAction_1.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/Toggle.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "UnityEngine/UI/ColorBlock.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "HMUI/ImageView.hpp"
#include "HMUI/AnimatedSwitchView.hpp"
#include "HMUI/SimpleTextDropdown.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/CustomFormatRangeValuesSlider.hpp"
#include "HMUI/TableView.hpp"
#include "BGLib/Polyglot/LocalizedTextMeshProUGUI.hpp"

#include "GlobalNamespace/GameplayModifierToggle.hpp"
#include "GlobalNamespace/GameplayModifiersPanelController.hpp"
#include "GlobalNamespace/GameplaySetupViewController.hpp"
#include "GlobalNamespace/SwitchSettingsController.hpp"
#include "GlobalNamespace/EnvironmentOverrideSettingsPanelController.hpp"
#include "GlobalNamespace/FormattedFloatListSettingsController.hpp"
#include "GlobalNamespace/ColorsOverrideSettingsPanelController.hpp"
#include "GlobalNamespace/ColorSchemeDropdown.hpp"
#include "GlobalNamespace/ColorSchemeView.hpp"
#include "GlobalNamespace/ColorSchemeTableCell.hpp"
#include "GlobalNamespace/EditColorSchemeController.hpp"
#include "GlobalNamespace/MainSettingsMenuViewController.hpp"
#include "GlobalNamespace/SettingsSubMenuInfo.hpp"
#include "BeatSaber/GameSettings/ControllerProfilesSettingsViewController.hpp"

#include "beatsaber-hook/shared/safeptr.hpp"

// Note: BSML-Lite owns settings-widget creation logic directly here. It no
// longer reaches into the corresponding Tags' protected CreateObject
// (previously via `#define protected public`/`#define private public`).

namespace BSML::Lite {
    namespace {
        HMUI::InputFieldView* GetFieldViewPrefabTemplate() {
            static safe_ptr<HMUI::InputFieldView*> fieldViewPrefab;
            if (!fieldViewPrefab) {
                fieldViewPrefab = UnityEngine::Resources::FindObjectsOfTypeAll<HMUI::InputFieldView*>().front_or_default([](auto x) { return x->get_name() == "GuestNameInputField"; });
            }
            return fieldViewPrefab.ptr();
        }

        GlobalNamespace::GameplayModifierToggle* GetGameplayModifierToggleTemplate() {
            static safe_ptr<GlobalNamespace::GameplayModifierToggle*> gameplayModifierToggleTemplate;
            if (!gameplayModifierToggleTemplate)
                gameplayModifierToggleTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::GameplaySetupViewController*>()->_gameplayModifiersPanelController->GetComponentsInChildren<GlobalNamespace::GameplayModifierToggle*>(true).front_or_default([](GlobalNamespace::GameplayModifierToggle* x) { return x->get_name() == "InstaFail"; });
            return gameplayModifierToggleTemplate.ptr();
        }

        GlobalNamespace::FormattedFloatListSettingsController* GetIncDecValueControllerTemplate() {
            static safe_ptr<GlobalNamespace::FormattedFloatListSettingsController*> incdecValueControllerTemplate;
            if (!incdecValueControllerTemplate) {
                incdecValueControllerTemplate = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::FormattedFloatListSettingsController*>().front([](auto x) { return x->get_name() == "VRRenderingScale"; });
            }
            return incdecValueControllerTemplate.ptr();
        }

        UnityEngine::UI::LayoutElement* GetControllersTransformTemplate() {
            static safe_ptr<UnityEngine::UI::LayoutElement*> controllersTransformTemplate;
            if (!controllersTransformTemplate) {
                controllersTransformTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::MainSettingsMenuViewController*>()->
                    _settingsSubMenuInfos.front([](auto x) {
                        return i2c::try_cast<BeatSaber::GameSettings::ControllerProfilesSettingsViewController*>(x->viewController.ptr());
                    })->viewController->transform->Find("Content/MainContent/Sliders/PositionX")->GetComponent<UnityEngine::UI::LayoutElement*>();
                if (!controllersTransformTemplate) ERROR("No controllersTransformTemplate found!");
            }
            return controllersTransformTemplate.ptr();
        }

        UnityEngine::GameObject* GetToggleTemplate() {
            static safe_ptr<UnityEngine::GameObject*> toggleTemplate;
            if (!toggleTemplate) {
                auto foundToggle = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::UI::Toggle*>().front_or_default([](auto x) {
                    if (!x) return false;
                    auto parent = x->get_transform()->get_parent();
                    if (!parent) return false;
                    return parent->get_gameObject()->get_name() == "Fullscreen";
                });
                toggleTemplate = foundToggle ? foundToggle->get_transform()->get_parent()->get_gameObject() : nullptr;
            }
            return toggleTemplate.ptr();
        }

        UnityEngine::GameObject* GetDropdownTemplate() {
            static safe_ptr<UnityEngine::GameObject*> dropdownTemplate;
            if (!dropdownTemplate) {
                dropdownTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::GameplaySetupViewController*>()->_environmentOverrideSettingsPanelController->_elementsGO->transform->Find("NormalLevels")->get_gameObject();
                if (!dropdownTemplate) ERROR("No dropdown template found!");
            }
            return dropdownTemplate.ptr();
        }

        GlobalNamespace::FormattedFloatListSettingsController* GetColorSettingRowTemplate() {
            static safe_ptr<GlobalNamespace::FormattedFloatListSettingsController*> baseSettings;
            if (!baseSettings) {
                baseSettings = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::FormattedFloatListSettingsController*>().front_or_default([](auto x) { return x->get_name() == "VRRenderingScale"; });
            }
            return baseSettings.ptr();
        }

        UnityEngine::UI::Image* GetColorSettingColorImageTemplate() {
            static safe_ptr<UnityEngine::UI::Image*> colorImage;
            if (!colorImage) {
                colorImage = Helpers::GetDiContainer()->Resolve<GlobalNamespace::GameplaySetupViewController*>()->_colorsOverrideSettingsPanelController->_colorSchemeDropDown->_cellPrefab->_colorSchemeView->_saberAColorImage;
            }
            return colorImage.ptr();
        }

        GlobalNamespace::RGBPanelController* GetRgbTemplate() {
            static safe_ptr<GlobalNamespace::RGBPanelController*> rgbTemplate;
            if (!rgbTemplate)
                rgbTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::GameplaySetupViewController*>()->GetComponentInChildren<GlobalNamespace::EditColorSchemeController*>(true)->GetComponentInChildren<GlobalNamespace::RGBPanelController*>(true);
            return rgbTemplate.ptr();
        }

        GlobalNamespace::HSVPanelController* GetHsvTemplate() {
            static safe_ptr<GlobalNamespace::HSVPanelController*> hsvTemplate;
            if (!hsvTemplate)
                hsvTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::GameplaySetupViewController*>()->GetComponentInChildren<GlobalNamespace::EditColorSchemeController*>(true)->GetComponentInChildren<GlobalNamespace::HSVPanelController*>(true);
            return hsvTemplate.ptr();
        }

        UnityEngine::UI::Image* GetModalCurrentColorTemplate() {
            static safe_ptr<UnityEngine::UI::Image*> currentColorTemplate;
            if (!currentColorTemplate) {
                currentColorTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::GameplaySetupViewController*>()->_colorsOverrideSettingsPanelController->_colorSchemeDropDown->_cellPrefab->_colorSchemeView->_saberAColorImage;
            }
            return currentColorTemplate.ptr();
        }

        const std::string kColorPickerButtonsXML =
            "<horizontal anchor-pos-y='-28' spacing='2' horizontal-fit='PreferredSize'>"
            "<button text='Cancel' on-click='CancelPressed' pref-width='34' pref-height='10'/>"
            "<action-button text='OK' on-click='DonePressed' pref-width='34' pref-height='10'/>"
            "</horizontal>";
    }

    HMUI::InputFieldView* CreateStringSetting(const TransformWrapper& parent, StringW settingsName, StringW currentValue, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector3 keyboardPositionOffset, std::function<void(StringW)> onValueChange) {
        auto go = UnityEngine::Object::Instantiate(GetFieldViewPrefabTemplate()->get_gameObject(), parent, false);
        static ConstString name("BSMLTextField");
        go->set_name(name);
        auto layoutElement = go->AddComponent<UnityEngine::UI::LayoutElement*>();
        layoutElement->set_preferredWidth(90.0f);
        layoutElement->set_preferredHeight(8.0f);
        auto transform = go->transform.cast<UnityEngine::RectTransform>();
        transform->set_anchoredPosition({0, 0});

        auto fieldView = go->GetComponent<HMUI::InputFieldView*>();
        fieldView->_useGlobalKeyboard = true;
        fieldView->_textLengthLimit = 128;
        fieldView->_keyboardPositionOffset = keyboardPositionOffset;

        fieldView->Awake();

        UnityEngine::Object::Destroy(fieldView->_placeholderText->GetComponent<BGLib::Polyglot::LocalizedTextMeshProUGUI*>());
        auto text = fieldView->_placeholderText->GetComponent<TMPro::TextMeshProUGUI*>();
        auto externalComponents = go->AddComponent<BSML::ExternalComponents*>();
        externalComponents->Add(text);

        transform->set_anchoredPosition(anchoredPosition);

        fieldView->onValueChanged = HMUI::InputFieldView::InputFieldChanged::New_ctor();
        if (onValueChange) {
            fieldView->onValueChanged->AddListener(
                custom_types::MakeDelegate<UnityEngine::Events::UnityAction_1<UnityW<HMUI::InputFieldView>>*>(
                    std::function<void(HMUI::InputFieldView*)>(
                        [onValueChange](auto fieldView) {
                            onValueChange(fieldView->get_text());
                        }
                    )
                )
            );
        }

        fieldView->set_text(currentValue);
        text->set_text(settingsName);

        return fieldView;
    }

    UnityEngine::UI::Toggle* CreateModifierButton(const TransformWrapper& parent, StringW buttonText, bool currentValue, UnityEngine::Sprite* iconSprite, std::function<void(bool)> onClick, std::optional<UnityEngine::Vector2> anchoredPosition) {
        DEBUG("Creating Modifier");

        auto gameplayModifierToggleTemplate = GetGameplayModifierToggleTemplate();
        auto baseModifier = UnityEngine::Object::Instantiate(gameplayModifierToggleTemplate, parent, false);
        baseModifier->set_name("BSMLModifier");

        auto gameObject = baseModifier->get_gameObject();
        gameObject->SetActive(false);
        auto transform = gameObject->transform.cast<UnityEngine::RectTransform>();
        UnityEngine::Object::Destroy(baseModifier);
        UnityEngine::Object::Destroy(gameObject->GetComponent<HMUI::HoverTextSetter*>());
        UnityEngine::Object::Destroy(transform->Find("Multiplier")->get_gameObject());

        auto externalComponents = gameObject->AddComponent<BSML::ExternalComponents*>();
        auto text = transform->Find("Name")->GetComponent<TMPro::TextMeshProUGUI*>();
        text->set_text(buttonText);
        externalComponents->Add(text);
        externalComponents->Add(transform->Find("Icon")->GetComponent<UnityEngine::UI::Image*>());

        auto toggle = gameObject->AddComponent<UnityEngine::UI::Toggle*>();
        auto toggleSetting = gameObject->AddComponent<BSML::ToggleSetting*>();
        toggleSetting->toggle = toggle;
        toggleSetting->text = text;

        if (iconSprite && iconSprite->m_CachedPtr.m_value) {
            auto img = externalComponents->Get<HMUI::ImageView*>();
            img->set_sprite(iconSprite);
        }

        if (anchoredPosition) transform->set_anchoredPosition(*anchoredPosition);
        if (onClick) {
            toggle->onValueChanged = UnityEngine::UI::Toggle::ToggleEvent::New_ctor();
            toggle->onValueChanged->AddListener(
                custom_types::MakeDelegate<UnityEngine::Events::UnityAction_1<bool>*>(onClick)
            );
        }

        toggle->set_isOn(currentValue);

        gameObject->SetActive(true);
        return toggle;
    }

    // Shared by IncrementSettingTag/ListSettingTag (via IncDecSettingTagBase) and the
    // direct-C++ CreateIncrementSetting() facade below. `type` must be (or derive
    // from) BSML::IncDecSetting — matches the runtime-typed AddComponent the Tags
    // always used (the concrete leaf type isn't knowable at this shared call site).
    BSML::IncDecSetting* CreateIncDecSettingBase(System::Type* type, const TransformWrapper& parent) {
        DEBUG("Creating IncDecSetting");

        auto baseSetting = UnityEngine::Object::Instantiate(GetIncDecValueControllerTemplate(), parent, false);
        auto gameObject = baseSetting->get_gameObject();
        UnityEngine::Object::Destroy(baseSetting);
        gameObject->SetActive(false);

        auto transform = gameObject->transform.cast<UnityEngine::RectTransform>();
        static ConstString name{"BSMLIncDecSetting"};
        gameObject->set_name(name);

        BSML::IncDecSetting* setting = gameObject->AddComponent(type).cast<BSML::IncDecSetting>();
        auto firstChild = transform->GetChild(1)->get_gameObject();

        setting->text = firstChild->GetComponentsInChildren<TMPro::TextMeshProUGUI*>().front();
        setting->text->set_richText(true);
        setting->text->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);

        auto buttons = firstChild->GetComponentsInChildren<UnityEngine::UI::Button*>();
        setting->decButton = buttons.front();
        setting->incButton = buttons.back();
        firstChild->transform.cast<UnityEngine::RectTransform>()->set_sizeDelta({40, 0});

        auto nameText = transform->Find("NameText")->get_gameObject();
        UnityEngine::Object::Destroy(nameText->GetComponent<BGLib::Polyglot::LocalizedTextMeshProUGUI*>());

        TMPro::TextMeshProUGUI* text = nameText->GetComponent<TMPro::TextMeshProUGUI*>();
        text->set_richText(true);
        text->set_text("BSMLIncDecSetting");

        auto layoutElement = gameObject->GetComponent<UnityEngine::UI::LayoutElement*>();
        layoutElement->set_preferredWidth(90);

        auto externalComponents = gameObject->AddComponent<BSML::ExternalComponents*>();
        externalComponents->Add(text);

        gameObject->SetActive(true);

        return setting;
    }

    BSML::IncrementSetting* CreateIncrementSetting(const TransformWrapper& parent, StringW label, int decimals, float increment, float currentValue, bool hasMin, bool hasMax, float minValue, float maxValue, UnityEngine::Vector2 anchoredPosition, std::function<void(float)> onValueChange) {
        auto incrementSetting = static_cast<BSML::IncrementSetting*>(CreateIncDecSettingBase(i2c::cs_type_of<BSML::IncrementSetting*>(), parent));
        auto go = incrementSetting->get_gameObject();
        auto externalComponents = go->GetComponent<BSML::ExternalComponents*>();

        incrementSetting->digits = decimals;
        incrementSetting->isInt = std::abs(increment - 1.0f) < 0.00001f;
        incrementSetting->increments = increment;

        auto rect = go->transform.cast<UnityEngine::RectTransform>();
        rect->set_anchoredPosition(anchoredPosition);

        if (hasMin) incrementSetting->minValue = minValue;
        if (hasMax) incrementSetting->maxValue = maxValue;
        auto text = externalComponents->Get<TMPro::TextMeshProUGUI*>();
        text->set_text(label);

        incrementSetting->BaseSetup();
        incrementSetting->Setup();

        incrementSetting->set_Value(currentValue);

        incrementSetting->onChange = onValueChange;

        incrementSetting->set_Value(currentValue);

        return incrementSetting;
    }

    // Shared by SliderSettingTag/ListSliderSettingTag (via GenericSliderSettingTagBase)
    // and the direct-C++ CreateSliderSetting() facade below. `type` must be (or
    // derive from) BSML::SliderSettingBase.
    BSML::SliderSettingBase* CreateGenericSliderSettingBase(System::Type* type, const TransformWrapper& parent) {
        DEBUG("Creating SliderSettingBase");
        auto baseSetting = UnityEngine::Object::Instantiate(GetControllersTransformTemplate(), parent, false);
        auto gameObject = baseSetting->get_gameObject();
        gameObject->set_name("BSMLSliderSetting");

        auto rectTransform = baseSetting->transform.cast<UnityEngine::RectTransform>();
        rectTransform->set_anchoredPosition({0, 0});
        UnityEngine::Object::Destroy(rectTransform->Find("SliderLeft")->gameObject);
        UnityEngine::Object::Destroy(baseSetting->GetComponent<UnityEngine::CanvasGroup*>());

        auto sliderSetting = gameObject->AddComponent(type).cast<BSML::SliderSettingBase>();
        auto slider = gameObject->GetComponentInChildren<HMUI::CustomFormatRangeValuesSlider*>();
        sliderSetting->slider = slider;

        // colors to not be red
        auto& colorBlock = slider->___m_Colors;
        colorBlock.set_normalColor({0, 0, 0, 0.5});
        colorBlock.set_highlightedColor({1, 1, 1, 0.2});
        colorBlock.set_pressedColor({1, 1, 1, 0.2});
        colorBlock.set_selectedColor({1, 1, 1, 0.2});
        colorBlock.set_disabledColor({0.8, 0.8, 0.8, 0.5});

        slider->set_name("BSMLSlider");
        slider->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_enableWordWrapping(false);
        slider->_enableDragging = true;
        auto sliderRect = slider->transform.cast<UnityEngine::RectTransform>();
        sliderRect->set_anchorMin({1, 0});
        sliderRect->set_anchorMax({1, 1});
        sliderRect->set_sizeDelta({52, 0});
        sliderRect->set_pivot({1, 0.5f});
        sliderRect->set_anchoredPosition({0, 0});

        auto nameText = gameObject->get_transform()->Find("Title")->get_gameObject();
        UnityEngine::Object::Destroy(nameText->GetComponent<BGLib::Polyglot::LocalizedTextMeshProUGUI*>());

        auto titleTransform = nameText->get_transform().cast<UnityEngine::RectTransform>();
        titleTransform->set_anchorMin({0, 0});
        titleTransform->set_anchorMax({0, 0});
        titleTransform->set_offsetMin({0, 0});
        titleTransform->set_offsetMax({-52, 0});

        TMPro::TextMeshProUGUI* text = nameText->GetComponent<TMPro::TextMeshProUGUI*>();
        text->set_richText(true);
        text->set_text("BSMLSlider");
        text->get_rectTransform()->set_anchorMax({1, 1});
        text->set_alignment(::TMPro::TextAlignmentOptions::CaplineLeft);
        text->set_enableWordWrapping(false);
        text->set_overflowMode(::TMPro::TextOverflowModes::Overflow);

        baseSetting->set_preferredWidth(90.0f);

        auto externalComponents = gameObject->AddComponent<BSML::ExternalComponents*>();
        externalComponents->Add(text);

        return sliderSetting;
    }

    BSML::SliderSetting* CreateSliderSetting(const TransformWrapper& parent, StringW label, float increment, float currentValue, float minValue, float maxValue, float applyValueTime, UnityEngine::Vector2 anchoredPosition, std::function<void(float)> onValueChange) {
        return CreateSliderSetting(parent, label, increment, currentValue, minValue, maxValue, applyValueTime, false, anchoredPosition, onValueChange);
    }

    BSML::SliderSetting* CreateSliderSetting(const TransformWrapper& parent, StringW label, float increment, float currentValue, float minValue, float maxValue, float applyValueTime, bool showButtons, UnityEngine::Vector2 anchoredPosition, std::function<void(float)> onValueChange) {
        auto sliderSetting = static_cast<BSML::SliderSetting*>(CreateGenericSliderSettingBase(i2c::cs_type_of<BSML::SliderSetting*>(), parent));
        auto go = sliderSetting->get_gameObject();
        auto externalComponents = go->GetComponent<BSML::ExternalComponents*>();

        sliderSetting->digits = 2;
        sliderSetting->isInt = std::abs(increment - 1.0f) < 0.00001f;
        sliderSetting->increments = increment;

        auto rect = go->transform.cast<UnityEngine::RectTransform>();
        rect->set_anchoredPosition(anchoredPosition);

        sliderSetting->slider->set_minValue(minValue);
        sliderSetting->slider->set_maxValue(maxValue);
        auto text = externalComponents->Get<TMPro::TextMeshProUGUI*>();
        text->set_text(label);

        sliderSetting->showButtons = showButtons;
        sliderSetting->BaseSetup();
        sliderSetting->Setup();

        sliderSetting->set_Value(currentValue);

        sliderSetting->onChange = onValueChange;

        sliderSetting->set_Value(currentValue);

        return sliderSetting;
    }

    BSML::DropdownListSetting* CreateDropdown(const TransformWrapper& parent, StringW label, StringW currentValue, std::span<std::string_view> values, std::function<void(StringW)> onValueChange) {
        DEBUG("Creating DropdownListSetting");

        auto gameObject = Helpers::GetDiContainer()->InstantiatePrefab(GetDropdownTemplate(), parent);
        auto transform = gameObject->transform.cast<UnityEngine::RectTransform>();
        auto externalComponents = gameObject->AddComponent<BSML::ExternalComponents*>();
        externalComponents->Add(transform);

        gameObject->set_name("BSMLDropdownList");
        auto dropdown = gameObject->GetComponentInChildren<HMUI::SimpleTextDropdown*>(true);
        dropdown->gameObject->SetActive(false);
        gameObject->SetActive(false);

        dropdown->set_name("Dropdown");

        auto labelObject = transform->Find("Label")->get_gameObject();
        UnityEngine::Object::Destroy(labelObject->GetComponent<BGLib::Polyglot::LocalizedTextMeshProUGUI*>());

        auto textMesh = labelObject->GetComponent<HMUI::CurvedTextMeshPro*>();
        textMesh->set_text(label);
        externalComponents->Add(textMesh);

        auto layoutElement = gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        layoutElement->set_preferredHeight(8.0f);
        layoutElement->set_preferredWidth(90.0f);
        externalComponents->Add(layoutElement);

        auto dropdownListSetting = dropdown->get_gameObject()->AddComponent<BSML::DropdownListSetting*>();
        dropdownListSetting->dropdown = dropdown;
        externalComponents->Add(dropdownListSetting);

        dropdown->_tableView->_preallocatedCells = ArrayW<HMUI::TableView::CellsGroup*>::New();
        dropdown->_tableView->_visibleCells->Clear();

        auto cont = dropdown->get_transform()->Find("DropdownTableView/Viewport/Content");
        int childCount = cont ? cont->get_childCount() : 0;
        for (int i = childCount - 1; i >= 0; i--) {
            cont->GetChild(i)->get_gameObject()->SetActive(false);
        }

        dropdown->gameObject->SetActive(true);
        gameObject->SetActive(true);

        // Only finalize (Setup()/select an index) when real values were actually
        // given — with none, there's nothing meaningful to set up yet (matches what
        // the bare XML-driven Tag needs: the TypeHandler supplies real values and
        // calls Setup() itself later, once XML attributes are parsed).
        if (!values.empty()) {
            auto valuesList = ListW<System::Object*>::New();
            valuesList->EnsureCapacity(values.size());
            for (auto v : values) valuesList->Add(static_cast<System::Object*>(StringW(v).convert()));
            dropdownListSetting->values = valuesList;

            dropdownListSetting->Setup();

            auto itr = std::find_if(values.begin(), values.end(), [currentValue](const auto& x) { return x == currentValue; });
            int idx = itr != values.end() ? itr - values.begin() : 0;

            dropdownListSetting->index = idx;
            dropdownListSetting->dropdown->SelectCellWithIdx(idx);
            dropdownListSetting->UpdateState();
        }

        dropdownListSetting->onChange = onValueChange;

        return dropdownListSetting;
    }

    BSML::ModalColorPicker* CreateColorPickerModal(const TransformWrapper& parent, StringW name, UnityEngine::Color defaultColor, std::function<void(UnityEngine::Color)> onDone, std::function<void()> onCancel, std::function<void(UnityEngine::Color)> onChange) {
        DEBUG("Creating Modal Color Picker");

        UnityEngine::GameObject* gameObject = CreateModal(parent, {0, 0}, {135, 70}, nullptr)->get_gameObject();
        auto externalComponents = gameObject->GetComponent<BSML::ExternalComponents*>();
        gameObject->set_name(name);

        auto colorPicker = gameObject->AddComponent<BSML::ModalColorPicker*>();
        colorPicker->modalView = externalComponents->Get<BSML::ModalView*>();
        // Center independently of a setting row's position in scrolled content.
        colorPicker->modalView->moveToCenter = true;

        auto delegate = MakeSystemAction(std::function<void(UnityEngine::Color, GlobalNamespace::ColorChangeUIEventType)>(
            std::bind(&BSML::ModalColorPicker::OnChange, colorPicker, std::placeholders::_1, std::placeholders::_2)
        ));
        auto rgbController = UnityEngine::Object::Instantiate(GetRgbTemplate(), gameObject->get_transform(), false);
        rgbController->set_name("BSMLRGBPanel");
        auto rgbTransform = rgbController->transform.cast<UnityEngine::RectTransform>();
        rgbTransform->set_anchoredPosition({0, 3});
        rgbTransform->set_anchorMin({0, .25f});
        rgbTransform->set_anchorMax({0, .25f});
        colorPicker->rgbPanel = rgbController;
        rgbController->add_colorDidChangeEvent(delegate);

        auto hsvController = UnityEngine::Object::Instantiate(GetHsvTemplate(), gameObject->get_transform(), false);
        hsvController->set_name("BSMLHSVPanel");
        auto hsvTransform = hsvController->transform.cast<UnityEngine::RectTransform>();
        hsvTransform->set_anchoredPosition({0, 3});
        hsvTransform->set_anchorMin({0.6f, 0.15f});
        hsvTransform->set_anchorMax({0.6f, 0.15f});
        hsvController->add_colorDidChangeEvent(delegate);
        colorPicker->hsvPanel = hsvController;

        auto colorImage = UnityEngine::Object::Instantiate(GetModalCurrentColorTemplate(), gameObject->get_transform(), false);
        colorImage->set_name("BSMLCurrentColor");
        auto colorTransform = colorImage->transform.cast<UnityEngine::RectTransform>();
        colorTransform->set_anchoredPosition({0, 0});
        colorTransform->set_anchorMin({0.53f, 0.53f});
        colorTransform->set_anchorMax({0.53f, 0.53f});
        colorTransform->set_sizeDelta({6, 6});
        colorPicker->colorImage = colorImage;

        BSML::parse_and_construct(kColorPickerButtonsXML, gameObject->get_transform(), colorPicker);

        externalComponents->Add(colorPicker);

        colorPicker->onChange = onChange;
        colorPicker->done = onDone;
        colorPicker->cancel = onCancel;
        colorPicker->currentColor = defaultColor;

        return colorPicker;
    }

    BSML::ColorSetting* CreateColorPicker(const TransformWrapper& parent, StringW label, UnityEngine::Color defaultColor, std::function<void(UnityEngine::Color)> onDone, std::function<void()> onCancel, std::function<void(UnityEngine::Color)> onChange) {
        DEBUG("Creating ColorSetting");

        auto baseSetting = UnityEngine::Object::Instantiate(GetColorSettingRowTemplate(), parent, false);
        auto gameObject = baseSetting->get_gameObject();
        auto externalComponents = gameObject->AddComponent<BSML::ExternalComponents*>();
        gameObject->SetActive(false);
        gameObject->set_name("BSMLColorSetting");
        externalComponents->Add(gameObject->get_transform());

        UnityEngine::Object::Destroy(baseSetting);
        auto colorSetting = gameObject->AddComponent<BSML::ColorSetting*>();
        externalComponents->Add(colorSetting);

        auto valuePick = gameObject->get_transform()->Find("ValuePicker")->get_gameObject();
        valuePick->transform.cast<UnityEngine::RectTransform>()->set_sizeDelta({13, 0});

        auto buttons = valuePick->GetComponentsInChildren<UnityEngine::UI::Button*>();
        auto decButton = buttons.front_or_default();
        decButton->set_enabled(false);
        decButton->set_interactable(true);
        UnityEngine::Object::Destroy(decButton->get_transform()->Find("Icon")->get_gameObject());
        UnityEngine::Object::Destroy(valuePick->GetComponentsInChildren<TMPro::TextMeshProUGUI*>().front_or_default()->get_gameObject());
        colorSetting->editButton = buttons.back_or_default();

        auto nameText = gameObject->get_transform()->Find("NameText")->get_gameObject();
        UnityEngine::Object::Destroy(nameText->GetComponent<BGLib::Polyglot::LocalizedTextMeshProUGUI*>());

        auto text = nameText->GetComponent<TMPro::TextMeshProUGUI*>();
        text->set_text(label);
        externalComponents->Add(text);

        auto layoutElement = gameObject->GetComponent<UnityEngine::UI::LayoutElement*>();
        layoutElement->set_preferredWidth(90.0f);
        externalComponents->Add(layoutElement);

        colorSetting->colorImage = UnityEngine::Object::Instantiate(GetColorSettingColorImageTemplate(), valuePick->get_transform(), false);
        colorSetting->colorImage->set_name("BSMLCurrentColor");
        auto colorImageTransform = colorSetting->colorImage->transform.cast<UnityEngine::RectTransform>();
        colorImageTransform->set_anchoredPosition({0, 0});
        colorImageTransform->set_sizeDelta({5, 5});
        colorImageTransform->set_anchorMin({0.2f, 0.5f});
        colorImageTransform->set_anchorMax({0.2f, 0.5f});
        externalComponents->Add(colorSetting->colorImage);

        auto icon = colorSetting->editButton->get_transform()->Find("Icon")->GetComponent<UnityEngine::UI::Image*>();
        icon->set_name("EditIcon");
        icon->set_sprite(Utilities::FindSpriteCached("EditIcon"));
        icon->get_rectTransform()->set_sizeDelta({4, 4});
        colorSetting->editButton->set_interactable(true);

        colorSetting->editButton->transform.cast<UnityEngine::RectTransform>()->set_anchorMin({0, 0});

        colorSetting->modalColorPicker = CreateColorPickerModal(gameObject->get_transform(), "BSMLModalColorPicker", {}, nullptr, nullptr, nullptr);

        gameObject->SetActive(true);

        colorSetting->Setup();

        colorSetting->modalColorPicker->onChange = onChange;
        colorSetting->modalColorPicker->done = onDone;
        colorSetting->modalColorPicker->cancel = onCancel;

        colorSetting->set_currentColor(defaultColor);

        return colorSetting;
    }

    BSML::ToggleSetting* CreateToggle(const TransformWrapper& parent, StringW label, bool currentValue, std::optional<UnityEngine::Vector2> anchoredPosition, std::function<void(bool)> onToggle) {
        DEBUG("Creating ToggleSetting");

        auto go = UnityEngine::Object::Instantiate(GetToggleTemplate(), parent, false);
        go->SetActive(false);
        auto transform = go->transform.cast<UnityEngine::RectTransform>();

        auto nameText = transform->Find("NameText")->get_gameObject();
        auto switchView = transform->Find("SwitchView")->get_gameObject();
        UnityEngine::Object::Destroy(go->GetComponent<GlobalNamespace::SwitchSettingsController*>());

        go->set_name("BSMLToggle");
        auto toggleSetting = go->AddComponent<BSML::ToggleSetting*>();
        HMUI::AnimatedSwitchView* animatedSwitchView = switchView->GetComponent<HMUI::AnimatedSwitchView*>();

        UnityEngine::Object::Destroy(nameText->GetComponent<BGLib::Polyglot::LocalizedTextMeshProUGUI*>());
        toggleSetting->toggle = switchView->GetComponent<UnityEngine::UI::Toggle*>();
        toggleSetting->toggle->onValueChanged = UnityEngine::UI::Toggle::ToggleEvent::New_ctor();
        toggleSetting->toggle->set_interactable(true);

        toggleSetting->text = nameText->GetComponent<TMPro::TextMeshProUGUI*>();
        toggleSetting->text->set_text(label);
        toggleSetting->text->set_richText(true);
        toggleSetting->text->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);

        auto layoutElement = go->GetComponent<UnityEngine::UI::LayoutElement*>();
        layoutElement->set_preferredWidth(90.0f);
        go->SetActive(true);

        auto externalComponents = go->AddComponent<BSML::ExternalComponents*>();
        externalComponents->Add(toggleSetting->text);

        if (anchoredPosition) transform->set_anchoredPosition(*anchoredPosition);

        toggleSetting->set_Value(currentValue);

        if (onToggle) {
            toggleSetting->toggle->onValueChanged = UnityEngine::UI::Toggle::ToggleEvent::New_ctor();
            toggleSetting->toggle->onValueChanged->AddListener(
                custom_types::MakeDelegate<UnityEngine::Events::UnityAction_1<bool>*>(onToggle)
            );
        }

        return toggleSetting;
    }
}
