#include "BSML/TypeHandlers/Settings/IncrementSettingHandler.hpp"

namespace BSML {
    static IncrementSettingHandler incrementSettingHandler{};

    IncrementSettingHandler::Base::PropMap IncrementSettingHandler::get_props() const {
        return {
            {"increment", {"increment"}},
            {"minValue", {"min"}},
            {"maxValue", {"max"}},
            {"isInt", {"integer-only"}},
            {"digits", {"digits"}}
        };
    }

    IncrementSettingHandler::Base::SetterMap IncrementSettingHandler::get_setters() const {
        return {
            {"increment",   [](auto component, auto value){ component->increments = static_cast<float>(value); }},
            {"minValue",   [](auto component, auto value){ component->minValue = static_cast<float>(value); }},
            {"maxValue",   [](auto component, auto value){ component->maxValue = static_cast<float>(value); }},
            {"isInt",   [](auto component, auto value){ component->isInt = static_cast<bool>(value); }},
            {"digits",   [](auto component, auto value){ component->digits = static_cast<int>(value); }},
        };
    }
}