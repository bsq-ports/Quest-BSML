#include "BSML/Tags/RawImageTag.hpp"
#include "BSML-Lite/Creation/Image.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<RawImageTag> rawImageTagParser({"raw-image", "raw-img"});

    UnityEngine::GameObject* RawImageTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating RawImage");
        auto image = BSML::Lite::CreateRawImage(parent, nullptr, {.sizeDelta = {20, 20}});
        return image->get_gameObject();
    }
}
