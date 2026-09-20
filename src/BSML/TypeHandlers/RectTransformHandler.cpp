#include "BSML/TypeHandlers/RectTransformHandler.hpp"
#include "Helpers/utilities.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/GameObject.hpp"
#include "HMUI/HoverHint.hpp"
#include "Helpers/getters.hpp"

void AddHoverHint(UnityEngine::RectTransform* rectTransform, const std::string& text) {
    auto hoverHint = rectTransform->GetComponent<HMUI::HoverHint*>();
    if (!hoverHint) {
        hoverHint = rectTransform->get_gameObject()->AddComponent<HMUI::HoverHint*>();
    }
    hoverHint->set_text(text);
    hoverHint->_hoverHintController = BSML::Helpers::GetHoverHintController();
}

// for now, because I wanna have the option later
void AddHoverHintKey(UnityEngine::RectTransform* rectTransform, const std::string& key) {
    AddHoverHint(rectTransform, key);
}

namespace BSML {
    static RectTransformHandler rectTransformHandler{};

    RectTransformHandler::Base::PropMap RectTransformHandler::get_props() const {
        return {
            {"anchorMinX", {"anchor-min-x"}},
            {"anchorMinY", {"anchor-min-y"}},
            {"anchorMin",  {"anchor-min"}},
            {"anchorMaxX", {"anchor-max-x"}},
            {"anchorMaxY", {"anchor-max-y"}},
            {"anchorMax",  {"anchor-max"}},
            {"anchorPosX", {"anchored-position-x", "anchor-pos-x"}},
            {"anchorPosY", {"anchored-position-y", "anchor-pos-y"}},
            {"anchorPos",  {"anchored-position", "anchor-pos"}},
            {"sizeDeltaX", {"size-delta-x"}},
            {"sizeDeltaY", {"size-delta-y"}},
            {"sizeDelta", {"size-delta"}},
            {"pivotX", {"pivot-x"}},
            {"pivotY", {"pivot-y"}},
            {"pivot", {"pivot"}},
            {"hoverHint", {"hover-hint"}},
            {"hoverHintKey", {"hover-hint-key"}},
            {"active", {"active"}},
            {"name", {"name"}},
            {"localScale", {"local-scale", "scale" }},
        };
    }

    RectTransformHandler::Base::SetterMap RectTransformHandler::get_setters() const {
        return {
            {"anchorMinX",      [](auto component, auto value){ component->set_anchorMin(UnityEngine::Vector2(static_cast<float>(value), component->get_anchorMin().y)); }},
            {"anchorMinY",      [](auto component, auto value){ component->set_anchorMin(UnityEngine::Vector2(component->get_anchorMin().x, static_cast<float>(value))); }},
            {"anchorMin",       [](auto component, auto value){ component->set_anchorMin(static_cast<UnityEngine::Vector2>(value)); }},
            {"anchorMaxX",      [](auto component, auto value){ component->set_anchorMax(UnityEngine::Vector2(static_cast<float>(value), component->get_anchorMax().y)); }},
            {"anchorMaxY",      [](auto component, auto value){ component->set_anchorMax(UnityEngine::Vector2(component->get_anchorMax().x, static_cast<float>(value))); }},
            {"anchorMax",       [](auto component, auto value){ component->set_anchorMax(static_cast<UnityEngine::Vector2>(value)); }},
            {"anchorPosX",      [](auto component, auto value){ component->set_anchoredPosition(UnityEngine::Vector2(static_cast<float>(value), component->get_anchoredPosition().y)); }},
            {"anchorPosY",      [](auto component, auto value){ component->set_anchoredPosition(UnityEngine::Vector2(component->get_anchoredPosition().x, static_cast<float>(value))); }},
            {"anchorPos",       [](auto component, auto value){ component->set_anchoredPosition(static_cast<UnityEngine::Vector2>(value)); }},
            {"sizeDeltaX",      [](auto component, auto value){ component->set_sizeDelta(UnityEngine::Vector2(static_cast<float>(value), component->get_sizeDelta().y)); }},
            {"sizeDeltaY",      [](auto component, auto value){ component->set_sizeDelta(UnityEngine::Vector2(component->get_sizeDelta().x, static_cast<float>(value))); }},
            {"sizeDelta",       [](auto component, auto value){ component->set_sizeDelta(static_cast<UnityEngine::Vector2>(value)); }},
            {"pivotX",          [](auto component, auto value){ component->set_pivot(UnityEngine::Vector2(static_cast<float>(value), component->get_pivot().y)); }},
            {"pivotY",          [](auto component, auto value){ component->set_pivot(UnityEngine::Vector2(component->get_pivot().x, static_cast<float>(value))); }},
            {"pivot",           [](auto component, auto value){ component->set_pivot(static_cast<UnityEngine::Vector2>(value)); }},
            {"hoverHint",       [](auto component, auto value){ AddHoverHint(component, value); }},
            {"hoverHintKey",    [](auto component, auto value){ AddHoverHintKey(component, value); }},
            {"active",          [](auto component, auto value){ component->get_gameObject()->SetActive(static_cast<bool>(value)); }},
            {"name",            [](auto component, auto value){ component->set_name(value); }},
            {"localScale",      [](auto component, auto value){ component->localScale = value.parseVector3(1); }}
        };
    }
}
