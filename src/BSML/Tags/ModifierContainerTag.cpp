#include "BSML/Tags/ModifierContainerTag.hpp"
#include "BSML-Lite/Creation/Layout.hpp"
#include "logging.hpp"

namespace BSML {
    static BSMLNodeParser<ModifierContainerTag> modifierContainerTagParser({"modifier-container"});

    UnityEngine::GameObject* ModifierContainerTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating ModifierContainer");
        return BSML::Lite::CreateModifierContainer(parent)->get_gameObject();
    }
}
