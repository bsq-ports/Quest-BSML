#pragma once

#include "../_config.h"
#include "../concepts.hpp"
#include "beatsaber-hook/shared/types.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Component.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Transform.hpp"

namespace BSML::Lite {
    /// @brief A wrapper for transforms, components and gameobjects to automatically be converted into a gameObject
    struct BSML_EXPORT GameObjectWrapper {
        constexpr GameObjectWrapper(UnityEngine::GameObject* gameObject) noexcept : gameObject(gameObject) {}

        template<Concepts::HasGameObject T>
        GameObjectWrapper(T t) : GameObjectWrapper(t->get_gameObject()) {}

        template<Concepts::BSMLConvertible<UnityEngine::GameObject*> T>
        GameObjectWrapper(T t) : GameObjectWrapper(static_cast<UnityEngine::GameObject*>(t)) {}

        // il2cpp wrapper type
        explicit GameObjectWrapper(void* i) : gameObject(static_cast<UnityEngine::GameObject*>(i)) {}
        constexpr inline void* convert() const noexcept { return const_cast<void*>(static_cast<const void*>(gameObject)); }

        UnityEngine::GameObject* operator->() const noexcept { return const_cast<UnityEngine::GameObject*>(gameObject); }
        operator UnityEngine::GameObject*() const noexcept { return const_cast<UnityEngine::GameObject*>(gameObject); }
        UnityEngine::GameObject* gameObject;
    };
}
MARK_REF_T(BSML::Lite::GameObjectWrapper);

template<>
struct ::i2c::type_check::no_arg_class<BSML::Lite::GameObjectWrapper> {
    static inline Il2CppClass* get() {
        return i2c::class_of<UnityEngine::GameObject*>();
    }
};
static_assert(sizeof(BSML::Lite::GameObjectWrapper) == sizeof(UnityEngine::GameObject*));
