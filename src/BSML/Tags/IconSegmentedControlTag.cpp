#include "BSML/Tags/IconSegmentedControlTag.hpp"
#include "BSML-Lite/Creation/Misc.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<IconSegmentedControlTag> iconSegmentedControlTag({"icon-segments"});

    UnityEngine::GameObject* IconSegmentedControlTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating IconSegmentedControl");
        return BSML::Lite::CreateIconSegmentedControl(parent)->get_gameObject();
    }
}
