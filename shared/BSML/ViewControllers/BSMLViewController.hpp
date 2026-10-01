#pragma once

#include "custom-types/shared/macros.hpp"
#include "HMUI/ViewController.hpp"
#include "UnityEngine/GameObject.hpp"
#include "../Parsing/BSMLParserParams.hpp"

DECLARE_CLASS_CODEGEN(BSML, BSMLViewController, HMUI::ViewController) {
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, contentObject);
    DECLARE_INSTANCE_FIELD(bool, destroyed);

    DECLARE_DEFAULT_CTOR();
    DECLARE_SIMPLE_DTOR();

    // Derived controllers supply these getters with DECLARE_INSTANCE_METHOD.
    // ParseWithFallback resolves them by name on the actual host type.
    DECLARE_INSTANCE_METHOD(StringW, get_Content);
    // {0} is replaced with the XML-escaped exception message, as on PC.
    DECLARE_INSTANCE_METHOD(StringW, get_FallbackContent);
    DECLARE_INSTANCE_METHOD(void, ClearContents);
    DECLARE_INSTANCE_METHOD(void, ParseWithFallback);
    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::ViewController::DidActivate, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
    DECLARE_OVERRIDE_METHOD_MATCH(void, OnDestroy, &HMUI::ViewController::OnDestroy);

    public:
        // Params for the current content; cleared when that content is replaced.
        std::shared_ptr<BSMLParserParams> parserParams;
};
