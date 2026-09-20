#include "BSML/Tags/Settings/GenericSliderSettingTag.hpp"
#include "BSML-Lite/Creation/Settings.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    UnityEngine::GameObject* GenericSliderSettingTagBase::CreateObject(UnityEngine::Transform* parent) const {
        return BSML::Lite::CreateGenericSliderSettingBase(get_type(), parent)->get_gameObject();
    }
}
