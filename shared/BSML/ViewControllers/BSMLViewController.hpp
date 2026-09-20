#pragma once

#include <memory>

#include "../../_config.h"
#include "beatsaber-hook/shared/stringw.hpp"

#include "custom-types/shared/macros.hpp"
#include "HMUI/ViewController.hpp"
#include "UnityEngine/GameObject.hpp"

namespace BSML {
    /// @brief BSML's built-in "Invalid BSML" fallback page. {0} is replaced with
    /// the XML-escaped exception message.
    BSML_EXPORT StringW DefaultFallbackContent();

    /// @brief Pure C++ interface for supplying a BSMLViewController's XML content,
    /// dispatched via ordinary C++ virtual calls instead of IL2CPP reflection.
    struct BSML_EXPORT BSMLContentProvider {
        virtual ~BSMLContentProvider() = default;

        /// @brief The BSML markup to parse for this ViewController's content. Required.
        virtual StringW GetContent() = 0;

        /// @brief Markup to parse instead if GetContent's markup fails to parse.
        /// {0} is replaced with the XML-escaped exception message. Optional — the
        /// default implementation returns BSML's built-in "Invalid BSML" page.
        virtual StringW GetFallbackContent();
    };
}


DECLARE_CLASS_CODEGEN(BSML, BSMLViewController, HMUI::ViewController) {
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, contentObject);
    DECLARE_INSTANCE_FIELD(bool, destroyed);

    // Legacy path: a derived IL2CPP class overriding these by name still works
    // (resolved reflectively, once per klass) when `contentProvider` below isn't
    // set. Prefer `contentProvider` for new code — plain C++ virtual dispatch,
    // no reflection, and the compiler enforces the signature.
    DECLARE_INSTANCE_METHOD(StringW, get_Content);
    // {0} is replaced with the XML-escaped exception message, as on PC.
    DECLARE_INSTANCE_METHOD(StringW, get_FallbackContent);
    DECLARE_INSTANCE_METHOD(void, ClearContents);
    DECLARE_INSTANCE_METHOD(void, ParseWithFallback);
    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::ViewController::DidActivate, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
    DECLARE_OVERRIDE_METHOD_MATCH(void, OnDestroy, &HMUI::ViewController::OnDestroy);

    public:
        // Set this to provide XML content via BSMLContentProvider (ordinary C++
        // virtual dispatch) instead of overriding get_Content/get_FallbackContent
        // by name. Preferred over the legacy reflective path above.
        std::unique_ptr<BSMLContentProvider> contentProvider;
};
