#include "BSML/TypeHandlers/ClickableTextHandler.hpp"

namespace BSML {
    static ClickableTextHandler clickableTextHandler;
    ClickableTextHandler::Base::PropMap ClickableTextHandler::get_props() const {
        return {
            {"onClick", {"on-click"}},
            {"clickEvent", {"click-event", "event-click"}},
            { "highlightColor", {"highlight-color"}},
            { "defaultColor", {"default-color"}}
        };
    }

    ClickableTextHandler::Base::SetterMap ClickableTextHandler::get_setters() const {
        return {
            { "highlightColor", [](auto component, auto value){ component->set_highlightColor(value); }},
            { "defaultColor",   [](auto component, auto value){ component->set_defaultColor(value); }}
        };
    }

    void ClickableTextHandler::HandleTypeAfterParse(const ComponentTypeWithData& componentType, BSMLParserParams& parserParams) {
        auto clickableText = reinterpret_cast<ClickableText*>(componentType.component);

        auto onClickItr = componentType.data.find("onClick");
        if (onClickItr != componentType.data.end() && !onClickItr->second.empty()) {
            auto action = parserParams.TryGetAction(onClickItr->second);
            if (action) clickableText->onClick += action->GetFunction();
            else ERROR("Action '{}' could not be found", onClickItr->second);
        }

        auto clickEventItr = componentType.data.find("clickEvent");
        if (clickEventItr != componentType.data.end() && !clickEventItr->second.empty()) {
            // Keep this scope's event alive for the lifetime of the click callback.
            auto parserEvent = parserParams.GetEvent(clickEventItr->second).lock();
            clickableText->onClick += [parserEvent](){
                parserEvent->Invoke();
            };
        }

        Base::HandleTypeAfterParse(componentType, parserParams);
    }
}
