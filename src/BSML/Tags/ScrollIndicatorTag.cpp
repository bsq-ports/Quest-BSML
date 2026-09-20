#include "BSML/Tags/ScrollIndicatorTag.hpp"
#include "BSML-Lite/Creation/Misc.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<ScrollIndicatorTag> scrollIndicatorTagParser({"scroll-indicator", "vertical-scroll-indicator"});

    UnityEngine::GameObject* ScrollIndicatorTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("making ScrollIndicator");
        return BSML::Lite::CreateScrollIndicator(parent)->get_gameObject();
    }
}
