#include "BSML/Tags/LoadingIndicatorTag.hpp"
#include "BSML-Lite/Creation/Misc.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<LoadingIndicatorTag> loadingIndicatorTagParser({"loading", "loading-indicator"});

    UnityEngine::GameObject* LoadingIndicatorTag::CreateObject(UnityEngine::Transform* parent) const {
        return BSML::Lite::CreateLoadingIndicator(parent);
    }
}
