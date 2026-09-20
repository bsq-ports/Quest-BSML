#include "BSML/Tags/Settings/ToggleSettingTag.hpp"
#include "BSML-Lite/Creation/Settings.hpp"
#include "BSML/Components/Settings/ToggleSetting.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<ToggleSettingTag> toggleSettingTagParser({"toggle-setting", "bool-setting", "checkbox-setting", "checkbox"});

    UnityEngine::GameObject* ToggleSettingTag::CreateObject(UnityEngine::Transform* parent) const {
        return BSML::Lite::CreateToggle(parent)->get_gameObject();
    }
}
