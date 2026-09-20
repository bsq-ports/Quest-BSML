#pragma once

#include <concepts>
#include <type_traits>

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Component.hpp"

namespace BSML::Concepts {
    template<typename T, typename U>
    concept BSMLConvertible = std::is_convertible_v<T, U>;

    template<typename T>
    concept HasGameObject = !BSMLConvertible<T, UnityEngine::GameObject*> && requires (T a) { {a->get_gameObject() } -> BSMLConvertible<UnityEngine::GameObject*>; };

    template<typename T>
    concept HasTransform = !BSMLConvertible<T, UnityEngine::Transform*> && requires (T a) { {a->get_transform() } -> BSMLConvertible<UnityEngine::Transform*>; };

    /// @brief Any type convertible to a raw UnityEngine::Component* — i.e. a
    /// codegen'd Unity component pointer (e.g. UnityEngine::UI::Button*).
    /// Use this instead of a repeated ad-hoc `requires(std::is_convertible_v<T,
    /// UnityEngine::Component*>)` clause on component-creation templates.
    template<typename T>
    concept ComponentPointer = BSMLConvertible<T, UnityEngine::Component*>;
}
