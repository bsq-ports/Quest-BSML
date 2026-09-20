#include "BSML/Tags/TextSegmentedControlTag.hpp"
#include "BSML-Lite/Creation/Misc.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<TextSegmentedControlTag> textSegmentedControlTag({"text-segments"});

    UnityEngine::GameObject* TextSegmentedControlTag::CreateObject(UnityEngine::Transform* parent) const {
        return BSML::Lite::CreateTextSegmentedControl(parent)->get_gameObject();
    }
}
