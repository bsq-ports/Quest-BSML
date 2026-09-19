#pragma once

#include <optional>

#include "beatsaber-hook/shared/listw.hpp"
#include "custom-types/shared/macros.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/Component.hpp"

DECLARE_CLASS_CODEGEN(BSML, ExternalComponents, UnityEngine::MonoBehaviour) {
    DECLARE_INSTANCE_FIELD(ListW<UnityEngine::Component*>, components);

    DECLARE_INSTANCE_METHOD(void, Add, UnityEngine::Component* component);
    DECLARE_CTOR(ctor);

    private:
        UnityEngine::Component* FindByType(Il2CppReflectionType* type) const;

    public:
        // Throws if called on a null instance. Prefer TryGetByType for a
        // null-safe lookup that reports "not found" without throwing.
        UnityEngine::Component* GetByType(System::Type* type) const;
        UnityEngine::Component* GetByType(Il2CppReflectionType* type) const;

        // Null-safe: returns std::nullopt both when this instance is null
        // and when no registered component matches, instead of throwing.
        std::optional<UnityEngine::Component*> TryGetByType(System::Type* type) const;
        std::optional<UnityEngine::Component*> TryGetByType(Il2CppReflectionType* type) const;

        template<typename T>
        requires(std::is_convertible_v<T, UnityEngine::Component*>)
        T Get() const {
            return reinterpret_cast<T>(GetByType(static_cast<Il2CppReflectionType*>(i2c::cs_type_of<T>())));
        }

        template<typename T>
        requires(std::is_convertible_v<T, UnityEngine::Component*>)
        std::optional<T> TryGet() const {
            auto result = TryGetByType(static_cast<Il2CppReflectionType*>(i2c::cs_type_of<T>()));
            if (!result) return std::nullopt;
            return reinterpret_cast<T>(*result);
        }
};
