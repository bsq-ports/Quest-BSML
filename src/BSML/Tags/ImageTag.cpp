#include "BSML/Tags/ImageTag.hpp"
#include "BSML-Lite/Creation/Image.hpp"
#include "Helpers/utilities.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<ImageTag> imageTagParser({"image", "img"});

    UnityEngine::GameObject* ImageTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating Image");
        // TODO: maybe use a default placeholder sprite instead? maybe a BSML image
        auto imageView = BSML::Lite::CreateImage(parent, Utilities::ImageResources::GetBlankSprite(), {.sizeDelta = {20, 20}});
        return imageView->get_gameObject();
    }
}
