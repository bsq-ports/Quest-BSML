#include "BSML/TypeHandlers/Settings/ListSliderSettingHandler.hpp"

namespace BSML {
    static ListSliderSettingHandler listSliderSettingHandler{};

    ListSliderSettingHandler::Base::PropMap ListSliderSettingHandler::get_props() const {
        return {
            {"options", {"options", "choices"}},
            {"showButtons", {"show-buttons"}}
        };
    }

    ListSliderSettingHandler::Base::SetterMap ListSliderSettingHandler::get_setters() const {
        return {
            {"showButtons", [](auto component, auto value) { component->showButtons = value; }}
        };
    }

    void ListSliderSettingHandler::HandleType(const ComponentTypeWithData& componentType, BSMLParserParams& parserParams) {
        Base::HandleType(componentType, parserParams);
        auto component = i2c::try_cast<ListSliderSetting*>(componentType.component);
        if (!component) {
            ERROR("ListSliderSettingHandler::HandleType given a component that is not a ListSliderSetting");
            return;
        }
        auto& data = componentType.data;

        auto optionsItr = data.find("options");
        if (optionsItr != data.end() && !optionsItr->second.empty()) {
            auto val = parserParams.TryGetValue(optionsItr->second);
            if (val) component->values = val->GetValue<System::Collections::Generic::List_1<System::Object*>*>();
        }

        if (!component->values || component->values->get_Count() == 0) {
            ERROR("Did not give options for dropdown list! this is required!");
        }
    }
}
