#include "BSML/Tags/PageButtonTag.hpp"
#include "BSML-Lite/Creation/Buttons.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<PageButtonTag> pageButtonTagParser({"page-button", "pg-button"});

    UnityEngine::GameObject* PageButtonTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating Page button");
        return BSML::Lite::CreatePageButton(parent);
    }
}
