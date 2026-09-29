#include "BSML/TypeHandlers/ButtonHandler.hpp"
#include "Helpers/delegates.hpp"
#include "logging.hpp"

#include "UnityEngine/Events/UnityAction.hpp"

using namespace UnityEngine;
using namespace UnityEngine::Events;
using namespace UnityEngine::UI;

namespace BSML {
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
        auto button = reinterpret_cast<Button*>(componentType.component);
        auto event = Button::ButtonClickedEvent::New_ctor();
        button->set_onClick(event);
    }

    void ButtonHandler::HandleTypeAfterParse(const ComponentTypeWithData& componentType, BSMLParserParams& parserParams) {
        Base::HandleTypeAfterParse(componentType, parserParams);

        auto button = reinterpret_cast<Button*>(componentType.component);
        auto event = button->get_onClick();

        // it was a button!
        auto onClickItr = componentType.data.find("onClick");
        if (onClickItr != componentType.data.end() && !onClickItr->second.empty()) {
            auto action = parserParams.TryGetAction(onClickItr->second);
            if (action) {
                event->AddListener(action->GetUnityAction());
            }
        }

        auto clickEventItr = componentType.data.find("clickEvent");
        if (clickEventItr != componentType.data.end() && !clickEventItr->second.empty()) {
            // The UI listener owns its event even when the parser goes out of scope.
            auto parserEvent = parserParams.GetEvent(clickEventItr->second).lock();
            auto action = MakeUnityAction([parserEvent] {
                parserEvent->Invoke();
            });
            event->AddListener(action);
        }
    }
}
