#include "BSML/Tags/SettingsContainerTag.hpp"
#include "BSML-Lite/Creation/Layout.hpp"
#include "logging.hpp"

namespace BSML {
    static BSMLNodeParser<SettingsContainerTag> settingsContainerTagParser({"settings-scroll-view", "scrollable-settings-container", "settings-container"});

    UnityEngine::GameObject* SettingsContainerTag::CreateObject(UnityEngine::Transform* parent) const {
        INFO("Creating settings container");
        return BSML::Lite::CreateScrollableSettingsContainer(parent);
    }
}
