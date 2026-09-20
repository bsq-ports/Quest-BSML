#include "BSML/Tags/ProgressBarTag.hpp"
#include "BSML-Lite/Creation/Misc.hpp"
#include "BSML/Components/ExternalComponents.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Vector3.hpp"

namespace BSML {
    BSMLNodeParser<ProgressBarTag> progressBarTagParser({"progress-bar"});

    UnityEngine::GameObject* ProgressBarTag::CreateObject(UnityEngine::Transform *parent) const {
        auto bar = BSML::Lite::CreateProgressBar({0, 0, 0}, {0, 0, 0}, {1, 1, 1}, "");
        bar->get_transform()->SetParent(parent, false);
        auto gameObject = bar->get_gameObject();

        auto externalComponents = gameObject->AddComponent<BSML::ExternalComponents*>();

        externalComponents->Add(bar->headerText);
        externalComponents->Add(bar->loadingBar);

        return gameObject;
    }
}
