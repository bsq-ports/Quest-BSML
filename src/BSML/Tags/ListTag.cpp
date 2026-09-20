#include "BSML/Tags/ListTag.hpp"
#include "BSML-Lite/Creation/Lists.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<ListTag> listTagParser({"list", "list2"});

    UnityEngine::GameObject* ListTag::CreateObject(UnityEngine::Transform* parent) const {
        return BSML::Lite::CreateList(parent, std::nullopt, std::nullopt, nullptr, false)->get_gameObject();
    }
}
