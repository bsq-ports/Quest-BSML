#include "BSML/Tags/GradientTextTag.hpp"
#include "BSML-Lite/Creation/Text.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<GradientTextTag> textTagParser({"gradient-text"});

    UnityEngine::GameObject* GradientTextTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating Gradient Text");
        return BSML::Lite::CreateGradientText(parent)->get_gameObject();
    }
}
