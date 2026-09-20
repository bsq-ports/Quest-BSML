#include "BSML/Tags/ClickableImageTag.hpp"
#include "BSML-Lite/Creation/Image.hpp"
#include "Helpers/utilities.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<ClickableImageTag> imageTagParser({"clickable-image", "clickable-img"});

    UnityEngine::GameObject* ClickableImageTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating Image");
        // TODO: maybe use a default placeholder sprite instead? maybe a BSML image
        auto image = BSML::Lite::CreateClickableImage(parent, Utilities::ImageResources::GetBlankSprite(), {.sizeDelta = {20, 20}});
        return image->get_gameObject();
    }
}
