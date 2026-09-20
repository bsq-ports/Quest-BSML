#include "BSML/Tags/VerticalTag.hpp"
#include "BSML-Lite/Creation/Layout.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<VerticalTag> verticalTagParser({"vertical"});
    UnityEngine::GameObject* VerticalTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating Vertical");
        return BSML::Lite::CreateVerticalLayoutGroup(parent)->get_gameObject();
    }
}
