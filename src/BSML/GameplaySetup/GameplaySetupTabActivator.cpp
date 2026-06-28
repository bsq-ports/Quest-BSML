#include "BSML/GameplaySetup/GameplaySetupTabActivator.hpp"
#include "logging.hpp"

DEFINE_TYPE(BSML, GameplaySetupTabActivator);

namespace BSML {
    void GameplaySetupTabActivator::OnEnable() {
        switch (menuSource) {
            case MenuSource::Method:
                if (didActivate) didActivate(get_gameObject(), !_activatedBefore);
                break;
            case MenuSource::Component:
                if (mb) {
                    auto* didActivate = i2c::functions::class_get_method_from_name(mb->klass, "DidActivate", 1);
                    if (didActivate) i2c::run_method(mb, didActivate, !_activatedBefore);
                    else ERROR("Class '{}::{}' did not implement 'void DidActivate(bool)' so it could not be called!", mb->klass->namespaze, mb->klass->name);
                }
                break;
            case MenuSource::BSMLContent: [[fallthrough]];
            case MenuSource::ViewController: [[fallthrough]];
            case MenuSource::FlowCoordinator:
                return;
        }
        _activatedBefore = true;
    }

    void GameplaySetupTabActivator::OnDisable() {
        if (mb && mb->m_CachedPtr.m_value && menuSource == MenuSource::Component) {
            auto* didDeactivate = i2c::functions::class_get_method_from_name(mb->klass, "DidDeactivate", 0);
            if (didDeactivate) i2c::run_method(mb, didDeactivate);
        }
    }
}
