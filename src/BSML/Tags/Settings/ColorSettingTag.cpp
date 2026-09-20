#include "BSML/Tags/Settings/ColorSettingTag.hpp"
#include "BSML-Lite/Creation/Settings.hpp"
#include "BSML/Components/Settings/ColorSetting.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<ColorSettingTag> colorSettingTagParser({"color-setting"});

    UnityEngine::GameObject* ColorSettingTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating ColorSetting");
        return BSML::Lite::CreateColorPicker(parent)->get_gameObject();
    }
}
