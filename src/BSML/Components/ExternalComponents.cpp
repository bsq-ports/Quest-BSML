#include "BSML/Components/ExternalComponents.hpp"
#include "logging.hpp"

DEFINE_TYPE(BSML, ExternalComponents);

namespace BSML {
    void ExternalComponents::ctor() {
        components = ListW<UnityEngine::Component*>::New();
    }

    void ExternalComponents::Add(UnityEngine::Component* component) {
        // runtime instance null check
        if (!static_cast<const void*>(this)) throw cordl_internals::NullException("Retrieving component on nullptr external components!");

        components->Add(component);
    }

    UnityEngine::Component* ExternalComponents::FindByType(Il2CppReflectionType* type) const {
        if (!type) return nullptr;

        auto klass = i2c::functions::class_from_system_type(type);
        INFO("Getting {}::{}", klass->namespaze, klass->name);
        for (auto component : components) {
            if (i2c::functions::class_is_assignable_from(klass, component->klass)) {
                return component;
            }
        }

        return nullptr;
    }

    UnityEngine::Component* ExternalComponents::GetByType(System::Type* type) const {
        return GetByType(reinterpret_cast<Il2CppReflectionType*>(type));
    }

    UnityEngine::Component* ExternalComponents::GetByType(Il2CppReflectionType* type) const {
        // runtime instance null check
        if (!static_cast<const void*>(this)) throw cordl_internals::NullException("Retrieving component on nullptr external components!");
        return FindByType(type);
    }

    std::optional<UnityEngine::Component*> ExternalComponents::TryGetByType(System::Type* type) const {
        return TryGetByType(reinterpret_cast<Il2CppReflectionType*>(type));
    }

    std::optional<UnityEngine::Component*> ExternalComponents::TryGetByType(Il2CppReflectionType* type) const {
        if (!static_cast<const void*>(this)) return std::nullopt;
        auto result = FindByType(type);
        if (!result) return std::nullopt;
        return result;
    }
}
