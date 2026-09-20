#pragma once

#include "../_config.h"
#include "../concepts.hpp"
#include "TransformWrapper.hpp"
#include "BSML/Components/ExternalComponents.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Component.hpp"
#include "UnityEngine/Object.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "beatsaber-hook/shared/safeptr.hpp"
#include <string_view>

namespace GlobalNamespace {
    class Signal;
}
namespace BeatSaber::Haptics {
    class HapticPresetSO;
    class HapticFeedbackManager;
}

namespace BSML::Lite {
    /// @brief Aggregate result of InstantiatePrefab: the instantiated component plus the
    /// GameObject/RectTransform/ExternalComponents created alongside it. Fields are
    /// safe_ptr<T> so they stay valid (RAII: pinned against GC collection) for the
    /// entire multi-step fixup sequence a caller typically performs afterward.
    template<Concepts::ComponentPointer T>
    struct InstantiatedPrefab {
        safe_ptr<T> component;
        safe_ptr<UnityEngine::GameObject*> gameObject;
        safe_ptr<UnityEngine::RectTransform*> rectTransform;
        safe_ptr<BSML::ExternalComponents*> externalComponents;
    };

    /// @brief Instantiates `prefab` under `parent`, names and activates it, and
    /// registers the component + its RectTransform on a freshly-added ExternalComponents.
    /// This is the common first step every prefab-based BSML component creation performs.
    /// @tparam T a codegen'd UnityEngine::Component subtype pointer (e.g. UnityEngine::UI::Button*)
    template<Concepts::ComponentPointer T>
    InstantiatedPrefab<T> InstantiatePrefab(T prefab, const TransformWrapper& parent, std::string_view name) {
        T component = UnityEngine::Object::Instantiate(prefab, parent, false);
        component->set_name(name);
        // get_gameObject()/transform.cast<>() each return their own UnityW<X> shape
        // (X is a pointer for get_gameObject(), a bare class for cast<>() — the two
        // aren't interconvertible), so use `auto` and let UnityW's own operator->/
        // implicit conversions do the work directly, rather than naming an explicit
        // (and easy to get subtly wrong) wrapper or raw-pointer type up front.
        auto gameObject = component->get_gameObject();
        gameObject->SetActive(true);
        auto rectTransform = component->transform.template cast<UnityEngine::RectTransform>();
        auto externalComponents = gameObject->template AddComponent<BSML::ExternalComponents*>();
        externalComponents->Add(component);
        externalComponents->Add(rectTransform);
        // safe_ptr<T>'s converting constructor takes T* by reference, so a UnityW<...>
        // argument here would need two chained user-defined conversions (UnityW's own
        // operator T*, then the safe_ptr constructor) — not allowed implicitly, so this
        // one spot needs an explicit unwrap.
        return InstantiatedPrefab<T>{
            component,
            static_cast<UnityEngine::GameObject*>(gameObject),
            static_cast<UnityEngine::RectTransform*>(rectTransform),
            externalComponents
        };
    }

    /// @brief Finds `relativePath` under `root` (or uses `root` itself if `relativePath`
    /// is empty), destroys any Polyglot localizer sitting on it, sets its TextMeshProUGUI
    /// text (rich text enabled), and registers that TextMeshProUGUI on `externalComponents`
    /// if non-null.
    /// @return the found TextMeshProUGUI, or nullptr if the path/component wasn't found
    BSML_EXPORT TMPro::TextMeshProUGUI* StripLocalizedTextAndSet(UnityEngine::Transform* root, std::string_view relativePath, StringW text, BSML::ExternalComponents* externalComponents = nullptr);

    /// @brief Applies the standard "let the layout group derive size from content" fixup
    /// used by prefab-instantiated BSML components: optionally destroys a stray
    /// LayoutElement blocking the fitter, adds a PreferredSize/PreferredSize
    /// ContentSizeFitter, finds a LayoutGroup among `searchLayoutGroupRoot`'s children,
    /// and ensures a LayoutElement exists — registering everything it touches on
    /// `externalComponents`.
    /// @param strayLayoutElementParent transform to strip an existing LayoutElement from
    /// before adding the fitter (nullptr to skip; e.g. the "Content" child on buttons)
    BSML_EXPORT void ApplyStandardSizingFixups(UnityEngine::GameObject* gameObject, UnityEngine::Transform* strayLayoutElementParent, UnityEngine::Component* searchLayoutGroupRoot, BSML::ExternalComponents* externalComponents);

    /// @brief The scene's shared "menu shockwave" click signal, used by clickable
    /// text/image components for their click feedback effect. Looked up once and cached.
    BSML_EXPORT GlobalNamespace::Signal* GetClickedSignal();

    /// @brief The shared low-intensity haptic preset used by clickable text/image
    /// components' click feedback. Created once and cached.
    BSML_EXPORT BeatSaber::Haptics::HapticPresetSO* GetClickHapticPreset();

    /// @brief The scene's HapticFeedbackManager, used to play GetClickHapticPreset().
    BSML_EXPORT BeatSaber::Haptics::HapticFeedbackManager* GetClickHapticFeedbackManager();
}
