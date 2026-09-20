#include "BSML/Tags/ModalColorPickerTag.hpp"
#include "BSML-Lite/Creation/Settings.hpp"
#include "BSML/Components/ModalColorPicker.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<ModalColorPickerTag> modalColorPickerTagParser({"modal-color-picker"});

    UnityEngine::GameObject* ModalColorPickerTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating Modal Color Picker");
        return BSML::Lite::CreateColorPickerModal(parent, "BSMLModalColorPicker", {}, nullptr, nullptr, nullptr)->get_gameObject();
    }
}
