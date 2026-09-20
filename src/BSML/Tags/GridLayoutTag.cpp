#include "BSML/Tags/GridLayoutTag.hpp"
#include "BSML-Lite/Creation/Layout.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<GridLayoutTag> gridLayoutTagParser({"grid"});
    UnityEngine::GameObject* GridLayoutTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating GridLayout");
        return BSML::Lite::CreateGridLayoutGroup(parent)->get_gameObject();
    }
}
