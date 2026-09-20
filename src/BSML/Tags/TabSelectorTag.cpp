#include "BSML/Tags/TabSelectorTag.hpp"
#include "BSML-Lite/Creation/Misc.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<TabSelectorTag> tabSelectorTagParser({"tab-select", "tab-selector"});

    UnityEngine::GameObject* TabSelectorTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating TabSelector");
        return BSML::Lite::CreateTabSelector(parent);
    }
}
