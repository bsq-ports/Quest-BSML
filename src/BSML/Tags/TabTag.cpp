#include "BSML/Tags/TabTag.hpp"
#include "BSML-Lite/Creation/Misc.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<TabTag> tabSelectorTagParser({"tab"});

    UnityEngine::GameObject* TabTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating Tab");
        return BSML::Lite::CreateTab(parent)->get_gameObject();
    }
}
