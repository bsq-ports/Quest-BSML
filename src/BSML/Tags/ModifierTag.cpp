#include "BSML/Tags/ModifierTag.hpp"
#include "BSML-Lite/Creation/Settings.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/UI/Toggle.hpp"

namespace BSML {
    static BSMLNodeParser<ModifierTag> modifierTagParser({"modifier", "modifier-toggle"});

    UnityEngine::GameObject* ModifierTag::CreateObject(UnityEngine::Transform* parent) const {
        return BSML::Lite::CreateModifierButton(parent)->get_gameObject();
    }
}
