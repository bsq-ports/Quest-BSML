#include "BSML/TypeHandlers/ModalKeyboardHandler.hpp"

namespace BSML {
    static ModalKeyboardHandler modalKeyboardHandler{};

    ModalKeyboardHandler::Base::PropMap ModalKeyboardHandler::get_props() const {
        return {
            { "value", {"value"}},
            { "onEnter", {"on-enter"}},
            { "clearOnOpen", {"clear-on-open"}}
        };
    }

    ModalKeyboardHandler::Base::SetterMap ModalKeyboardHandler::get_setters() const {
        return {
            {"clearOnOpen", [](auto component, auto value){ component->clearOnOpen = value; }}
        };
    }

    void ModalKeyboardHandler::HandleType(const ComponentTypeWithData& componentType, BSMLParserParams& parserParams) {
        Base::HandleType(componentType, parserParams);
        auto modalKeyboard = i2c::try_cast<BSML::ModalKeyboard*>(componentType.component);
        if (!modalKeyboard) {
            ERROR("ModalKeyboardHandler::HandleType given a component that is not a ModalKeyboard");
            return;
        }
        auto& data = componentType.data;

        auto valueItr = data.find("value");
        if (valueItr != data.end() && !valueItr->second.empty()) {
            auto genericSetting = modalKeyboard->genericSetting;
            auto val = parserParams.TryGetValue(valueItr->second);
            if (val) {
                genericSetting->host = val->host;
                genericSetting->valueInfo = val->fieldInfo;
                genericSetting->getterInfo = val->getterInfo;
                genericSetting->setterInfo = val->setterInfo;
            }
        }
    }

    void ModalKeyboardHandler::HandleTypeAfterParse(const ComponentTypeWithData& componentType, BSMLParserParams& parserParams) {
        Base::HandleTypeAfterParse(componentType, parserParams);
        auto modalKeyboard = i2c::try_cast<BSML::ModalKeyboard*>(componentType.component);
        if (!modalKeyboard) {
            ERROR("ModalKeyboardHandler::HandleTypeAfterParse given a component that is not a ModalKeyboard");
            return;
        }
        auto& data = componentType.data;
    
        auto onEnterItr = data.find("onEnter");
        if (onEnterItr != data.end() && !onEnterItr->second.empty()) {
            auto action = parserParams.TryGetAction(onEnterItr->second);
            if (action) modalKeyboard->onEnter = action->GetFunction<StringW>();
            else ERROR("Action '{}' could not be found", onEnterItr->second);
        }
    }
}