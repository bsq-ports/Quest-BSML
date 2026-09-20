#include "BSML/Tags/ButtonWithIconTag.hpp"
#include "BSML-Lite/Creation/Buttons.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<ButtonWithIconTag> buttonWithIconTagParser({"button-with-icon", "icon-button"});

    UnityEngine::GameObject* ButtonWithIconTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating Button with icon");
        return BSML::Lite::CreateIconButton(parent);
    }
}
