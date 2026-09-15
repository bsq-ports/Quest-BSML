#pragma once

#include "custom-types/shared/macros.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/GameObject.hpp"
#include "HMUI/NoTransitionsButton.hpp"
#include "System/Action_1.hpp"

DECLARE_CLASS_CODEGEN(BSML, ButtonIconImage, UnityEngine::MonoBehaviour) {
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Image*, image);
    DECLARE_INSTANCE_FIELD(HMUI::NoTransitionsButton*, button);
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, underline);
    DECLARE_INSTANCE_FIELD(System::Action_1<HMUI::NoTransitionsButton::SelectionState>*, selectionStateChanged);

    DECLARE_INSTANCE_METHOD(void, OnEnable);
    DECLARE_INSTANCE_METHOD(void, OnDisable);
    DECLARE_INSTANCE_METHOD(void, OnSelectionStateDidChange, HMUI::NoTransitionsButton::SelectionState state);

    public:
        void SetIcon(StringW path);
        void SetIcon(UnityEngine::Sprite* sprite);
        void SetSkew(float value);
        void SetUnderlineActive(bool active);
};
