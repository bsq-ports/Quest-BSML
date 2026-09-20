#include "BSML/Tags/HorizontalTag.hpp"
#include "BSML-Lite/Creation/Layout.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<HorizontalTag> horizontalTagParser({"horizontal"});
    UnityEngine::GameObject* HorizontalTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating Horizontal");
        return BSML::Lite::CreateHorizontalLayoutGroup(parent)->get_gameObject();
    }
}
