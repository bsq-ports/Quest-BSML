#pragma once

#include "custom-types/shared/macros.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "HMUI/ImageView.hpp"
#include <optional>
#include <string_view>

DECLARE_CLASS_CODEGEN(BSML, Backgroundable, UnityEngine::MonoBehaviour) {
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, background);
    
    DECLARE_INSTANCE_METHOD(void, ApplyBackground, StringW name);
    DECLARE_INSTANCE_METHOD(void, ApplyColor, UnityEngine::Color color);
    DECLARE_INSTANCE_METHOD(void, ApplyColor0, UnityEngine::Color color);
    DECLARE_INSTANCE_METHOD(void, ApplyColor1, UnityEngine::Color color);
    DECLARE_INSTANCE_METHOD(void, ApplyGradient, UnityEngine::Color color0, UnityEngine::Color color1);
    DECLARE_INSTANCE_METHOD(void, ApplyAlpha, float alpha);

    private:
        /// @return the matching template, or nullopt if the scene has no
        /// ImageView matching all three of spriteName/objectName/parentName.
        static std::optional<HMUI::ImageView*> FindTemplate(std::string_view spriteName, std::string_view objectName, std::string_view parentName);
};