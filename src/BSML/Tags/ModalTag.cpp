#include "BSML/Tags/ModalTag.hpp"
#include "BSML-Lite/Creation/Layout.hpp"
#include "BSML/Components/ModalView.hpp"
#include "logging.hpp"

namespace BSML {
    static BSMLNodeParser<ModalTag> modalTagParser({"modal"});

    UnityEngine::GameObject* ModalTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating Modal");
        return BSML::Lite::CreateModal(parent, {0, 0}, {0, 0}, nullptr)->get_gameObject();
    }
}
