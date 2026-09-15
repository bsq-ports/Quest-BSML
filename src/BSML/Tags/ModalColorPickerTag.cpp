#include "BSML/Tags/ModalColorPickerTag.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "GlobalNamespace/ColorSchemeView.hpp"
#include "GlobalNamespace/ColorSchemeTableCell.hpp"
#include "GlobalNamespace/ColorSchemeDropdown.hpp"
#include "GlobalNamespace/ColorsOverrideSettingsPanelController.hpp"
#include "GlobalNamespace/EditColorSchemeController.hpp"
#include "GlobalNamespace/GameplaySetupViewController.hpp"
#include "Helpers/getters.hpp"
#include "BSML/Components/ModalColorPicker.hpp"
#include "BSML/Components/ExternalComponents.hpp"
#include "Helpers/delegates.hpp"
#include "logging.hpp"
#include "BSML.hpp"

#include "UnityEngine/Object.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Vector2.hpp"

#include "HMUI/ImageView.hpp"
#include "BSML/Components/ModalColorPicker.hpp"

using namespace HMUI;
using namespace UnityEngine;
using namespace UnityEngine::UI;

namespace BSML {
    static BSMLNodeParser<ModalColorPickerTag> modalColorPickerTagParser({"modal-color-picker"});
    std::string buttonXML {
        "\
        <horizontal anchor-pos-y='-28' spacing='2' horizontal-fit='PreferredSize'>\
            <button text='Cancel' on-click='CancelPressed' pref-width='34' pref-height='10'/>\
            <action-button text='OK' on-click='DonePressed' pref-width='34' pref-height='10'/>\
        </horizontal>\
        "
    };

    GlobalNamespace::RGBPanelController* get_rgbTemplate() {
        static safe_ptr<GlobalNamespace::RGBPanelController*> rgbTemplate;
        if (!rgbTemplate)
            rgbTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::GameplaySetupViewController*>()->GetComponentInChildren<GlobalNamespace::EditColorSchemeController*>(true)->GetComponentInChildren<GlobalNamespace::RGBPanelController*>(true);
        return rgbTemplate.ptr();
    }
    GlobalNamespace::HSVPanelController* get_hsvTemplate() {
        static safe_ptr<GlobalNamespace::HSVPanelController*> hsvTemplate;
        if (!hsvTemplate)
            hsvTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::GameplaySetupViewController*>()->GetComponentInChildren<GlobalNamespace::EditColorSchemeController*>(true)->GetComponentInChildren<GlobalNamespace::HSVPanelController*>(true);
        return hsvTemplate.ptr();
    }
    UnityEngine::UI::Image* get_currentColorTemplate() {
        static safe_ptr<UnityEngine::UI::Image*> currentColorTemplate;
        if (!currentColorTemplate) {
            currentColorTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::GameplaySetupViewController*>()->_colorsOverrideSettingsPanelController->_colorSchemeDropDown->_cellPrefab->_colorSchemeView->_saberAColorImage;
        }
        return currentColorTemplate.ptr();
    }

    UnityEngine::GameObject* ModalColorPickerTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating Modal Color Picker");

        GameObject* gameObject = Base::CreateObject(parent);
        auto externalComponents = gameObject->GetComponent<ExternalComponents*>();
        auto windowTransform = gameObject->transform.cast<RectTransform>();
        gameObject->set_name("BSMLModalColorPicker");
        windowTransform->set_sizeDelta({135, 70});

        auto colorPicker = gameObject->AddComponent<ModalColorPicker*>();
        colorPicker->modalView = externalComponents->Get<ModalView*>();
        // Center independently of a setting row's position in scrolled content.
        // The shared factory also covers Lite; markup can still override this.
        colorPicker->modalView->moveToCenter = true;

        auto onChangeInfo = i2c::functions::class_get_method_from_name(colorPicker->klass, "OnChange", 2);
        auto delegate = MakeSystemAction<UnityEngine::Color, GlobalNamespace::ColorChangeUIEventType>(colorPicker, onChangeInfo);
        auto rgbController = Object::Instantiate(get_rgbTemplate(), gameObject->get_transform(), false);
        rgbController->set_name("BSMLRGBPanel");
        auto rgbTransform = rgbController->transform.cast<RectTransform>();
        rgbTransform->set_anchoredPosition({0, 3});
        rgbTransform->set_anchorMin({0, .25f});
        rgbTransform->set_anchorMax({0, .25f});
        colorPicker->rgbPanel = rgbController;
        rgbController->add_colorDidChangeEvent(delegate);

        auto hsvController = Object::Instantiate(get_hsvTemplate(), gameObject->get_transform(), false);
        hsvController->set_name("BSMLHSVPanel");
        auto hsvTransform = hsvController->transform.cast<RectTransform>();
        hsvTransform->set_anchoredPosition({0, 3});
        hsvTransform->set_anchorMin({0.6f, 0.15f});
        hsvTransform->set_anchorMax({0.6f, 0.15f});
        hsvController->add_colorDidChangeEvent(delegate);
        colorPicker->hsvPanel = hsvController;

        auto colorImage = Object::Instantiate(get_currentColorTemplate(), gameObject->get_transform(), false);
        colorImage->set_name("BSMLCurrentColor");
        auto colorTransform = colorImage->transform.cast<RectTransform>();
        colorTransform->set_anchoredPosition({0, 0});
        colorTransform->set_anchorMin({0.53f, 0.53f});
        colorTransform->set_anchorMax({0.53f, 0.53f});
        colorTransform->set_sizeDelta({6, 6});
        colorPicker->colorImage = colorImage;

        BSML::parse_and_construct(buttonXML, gameObject->get_transform(), colorPicker);

        externalComponents->Add(colorPicker);
        return gameObject;
    }
}
