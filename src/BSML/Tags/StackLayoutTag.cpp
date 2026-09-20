#include "BSML/Tags/StackLayoutTag.hpp"
#include "BSML-Lite/Creation/Layout.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<StackLayoutTag> stackLayoutTagParser({"stack"});

    UnityEngine::GameObject* StackLayoutTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating StackLayoutTag");
        return BSML::Lite::CreateStackLayoutGroup(parent)->get_gameObject();
    }
}
