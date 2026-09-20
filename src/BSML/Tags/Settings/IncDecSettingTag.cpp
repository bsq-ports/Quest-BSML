#include "BSML/Tags/Settings/IncDecSettingTag.hpp"
#include "BSML-Lite/Creation/Settings.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    UnityEngine::GameObject* IncDecSettingTagBase::CreateObject(UnityEngine::Transform* parent) const {
        return BSML::Lite::CreateIncDecSettingBase(get_type(), parent)->get_gameObject();
    }
}
