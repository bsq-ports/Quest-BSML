#include "BSML/TypeHandlers/ButtonHandler.hpp"
#include "Helpers/delegates.hpp"
#include "logging.hpp"

#include "UnityEngine/Events/UnityAction.hpp"

#include <optional>

using namespace UnityEngine;
using namespace UnityEngine::Events;
using namespace UnityEngine::UI;

namespace BSML {
    namespace {
        /// @brief Reads componentType.data[key] (see ComponentTypeWithData::data for what this
        /// map holds), returning nullopt if the attribute wasn't given or was left empty.
        std::optional<std::string> GetAttribute(const ComponentTypeWithData& componentType, const std::string& key) {
            auto it = componentType.data.find(key);
            if (it == componentType.data.end() || it->second.empty()) return std::nullopt;
            return it->second;
        }
    }

    static ButtonHandler buttonHandler;
    ButtonHandler::Base::PropMap ButtonHandler::get_props() const {
        return {
            {"onClick", {"on-click"}},
            {"clickEvent", {"click-event", "event-click"}}
        };
    }

    ButtonHandler::Base::SetterMap ButtonHandler::get_setters() const {
        return {};
    }

    void ButtonHandler::HandleType(const ComponentTypeWithData& componentType, BSMLParserParams& parserParams) {
        Base::HandleType(componentType, parserParams);
        auto button = i2c::try_cast<Button*>(componentType.component);
        if (!button) {
            ERROR("ButtonHandler::HandleType given a component that is not a Button");
            return;
        }
        auto event = Button::ButtonClickedEvent::New_ctor();
        button->set_onClick(event);
    }

    void ButtonHandler::HandleTypeAfterParse(const ComponentTypeWithData& componentType, BSMLParserParams& parserParams) {
        Base::HandleTypeAfterParse(componentType, parserParams);

        auto button = i2c::try_cast<Button*>(componentType.component);
        if (!button) {
            ERROR("ButtonHandler::HandleTypeAfterParse given a component that is not a Button");
            return;
        }
        auto event = button->get_onClick();

        // "onClick" (markup: on-click="MethodName") names a method on the parsing host to call
        // on click; the attribute's value is a lookup key into parserParams, not a literal.
        if (auto methodName = GetAttribute(componentType, "onClick")) {
            auto action = parserParams.TryGetAction(*methodName);
            if (action) {
                event->AddListener(action->GetUnityAction());
            }
        }

        // "clickEvent" (markup: click-event="eventName") instead wires the click to a named
        // BSMLEvent registered on parserParams, invoked by whoever emits that event elsewhere.
        if (auto eventName = GetAttribute(componentType, "clickEvent")) {
            auto parserEvent = parserParams.GetEvent(*eventName);
            auto action = MakeUnityAction([parserEvent](){
                if (!parserEvent.expired()) {
                    parserEvent.lock()->Invoke();
                } else {
                    ERROR("Event pointer expired, are you saving your parser params?");
                }
            });
            event->AddListener(action);
        }
    }
}
