#include "BSML/Tags/ScrollViewTag.hpp"
#include "BSML-Lite/Creation/Layout.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<ScrollViewTag> scrollViewTagParser({"scroll-view"});

    UnityEngine::GameObject* ScrollViewTag::CreateObject(UnityEngine::Transform* parent) const {
        return BSML::Lite::CreateScrollView(parent);
    }
}
