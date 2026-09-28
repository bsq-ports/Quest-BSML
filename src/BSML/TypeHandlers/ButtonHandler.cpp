#include "BSML/TypeHandlers/ButtonHandler.hpp"
#include "Helpers/delegates.hpp"
#include "logging.hpp"

#include "UnityEngine/Events/UnityAction.hpp"

// custom-types' lambda delegate wrapper is not marked as finalizable. Use a
// managed target so the listener both retains and eventually releases its event.
DECLARE_CLASS_CODEGEN(BSML, ButtonEventCallback, System::Object) {
    DECLARE_CTOR(ctor);
    DECLARE_OVERRIDE_METHOD_MATCH(void, Finalize, &System::Object::Finalize);
public:
    std::shared_ptr<BSML::BSMLEvent> event;
};

DEFINE_TYPE(BSML, ButtonEventCallback);

using namespace UnityEngine;
using namespace UnityEngine::Events;
using namespace UnityEngine::UI;

namespace BSML {
    void ButtonEventCallback::ctor() {
        INVOKE_CTOR();
        INVOKE_BASE_CTOR(i2c::class_of<System::Object*>());
    }

    void ButtonEventCallback::Finalize() {
        this->~ButtonEventCallback();
        i2c::run_method(this, i2c::metadata_getter<&System::Object::Finalize>::method_info());
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
            auto callback = ButtonEventCallback::New_ctor();
            callback->event = parserParams.GetEvent(clickEventItr->second).lock();
            // The wrapper retains its managed target; the captureless trampoline
            // owns no native resources that would require wrapper finalization.
            auto action = custom_types::MakeDelegate<UnityAction*>(callback,
                std::function<void(ButtonEventCallback*)>([](ButtonEventCallback* self) {
                    self->event->Invoke();
                }));
            event->AddListener(action);
        }
    }
}
