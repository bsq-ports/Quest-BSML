#include "BSML/Tags/ScrollableContainerTag.hpp"
#include "BSML-Lite/Creation/Layout.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<ScrollableContainerTag> scrollableContainerTagParser({"scrollable-container"});

    UnityEngine::GameObject* ScrollableContainerTag::CreateObject(UnityEngine::Transform* parent) const {
        return BSML::Lite::CreateScrollableContainer(parent);
    }
}
