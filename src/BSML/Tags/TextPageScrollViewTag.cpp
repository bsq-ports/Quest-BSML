#include "BSML/Tags/TextPageScrollViewTag.hpp"
#include "BSML-Lite/Creation/Misc.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<TextPageScrollViewTag> textPageScrollViewTagParser({"text-page", "page"});

    UnityEngine::GameObject* TextPageScrollViewTag::CreateObject(UnityEngine::Transform* parent) const {
        return BSML::Lite::CreateTextPageScrollView(parent)->get_gameObject();
    }
}
