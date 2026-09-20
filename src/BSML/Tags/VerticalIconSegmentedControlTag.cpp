#include "BSML/Tags/VerticalIconSegmentedControlTag.hpp"
#include "BSML-Lite/Creation/Misc.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<VerticalIconSegmentedControlTag> vericalIconSegmentedControlTag({"vertical-icon-segments"});

    UnityEngine::GameObject* VerticalIconSegmentedControlTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating VerticalIconSegmentedControl");
        return BSML::Lite::CreateVerticalIconSegmentedControl(parent)->get_gameObject();
    }
}
