#include "BSML/Tags/Settings/DropdownListSettingTag.hpp"
#include "BSML-Lite/Creation/Settings.hpp"
#include "BSML/Components/Settings/DropdownListSetting.hpp"
#include "BSML/Components/ExternalComponents.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<DropdownListSettingTag> dropdownListSettingTagParser({"dropdown-list-setting"});

    UnityEngine::GameObject* DropdownListSettingTag::CreateObject(UnityEngine::Transform* parent) const {
        // CreateDropdown returns the DropdownListSetting component, which lives on a child
        // GameObject (the dropdown itself), not the wrapper the Tag needs to return for
        // correct XML composition — GetComponentInParent finds the wrapper's ExternalComponents.
        auto dropdownSetting = BSML::Lite::CreateDropdown(parent);
        return dropdownSetting->GetComponentInParent<BSML::ExternalComponents*>()->get_gameObject();
    }
}
