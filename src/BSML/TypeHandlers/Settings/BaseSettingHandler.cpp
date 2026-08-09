#include "BSML/TypeHandlers/Settings/BaseSettingHandler.hpp"
#include "BSML/GenericSettingWrapper.hpp"

namespace BSML {
    static BaseSettingHandler baseSettingHandler{};

    int BaseSettingHandler::get_priority() const { return 10; }
    BaseSettingHandler::Base::PropMap BaseSettingHandler::get_props() const {
        return {
            {"id", {"id"}},
            {"onChange", {"on-change"}},
            {"value", {"value"}},
            { "setEvent", {"set-event"}},
            { "getEvent", {"get-event"}},
            {"applyOnChange", {"apply-on-change"}}
        };
    }

    BaseSettingHandler::Base::SetterMap BaseSettingHandler::get_setters() const {
        return {};
    }

    void BaseSettingHandler::HandleType(const ComponentTypeWithData& componentType, BSMLParserParams& parserParams) {
        auto component = componentType.component;
        auto& data = componentType.data;

        auto genericSettings = i2c::get_field<i2c::result<GenericSettingWrapper*>>(
            reinterpret_cast<Il2CppObject*>(component), "genericSetting"
        ).value_or(nullptr);
        if (!genericSettings) {
            ERROR("Type {}::{} did not have a 'genericSetting' field", component->klass->namespaze, component->klass->name);
            return;
        }

        auto applyOnChangeItr = data.find("applyOnChange");
        if (applyOnChangeItr != data.end()) {
            auto arg = StringParseHelper(applyOnChangeItr->second);
            genericSettings->applyOnChange = arg.tryParseBool().value_or(true);
        }

        auto valueItr = data.find("value");
        if (valueItr != data.end() && !valueItr->second.empty()) {
            auto val = parserParams.TryGetValue(valueItr->second);
            if (val) {
                genericSettings->host = val->host;
                genericSettings->valueInfo = val->fieldInfo;
                genericSettings->getterInfo = val->getterInfo;
                genericSettings->setterInfo = val->setterInfo;
            }
        }

        auto onChangeItr = data.find("onChange");
        if (onChangeItr != data.end() && !onChangeItr->second.empty()) {
            auto action = parserParams.TryGetAction(onChangeItr->second);
            if (action) {
                genericSettings->onChangeHost = action->host;
                genericSettings->onChangeInfo = action->methodInfo;
            } else ERROR("Action '{}' could not be found", onChangeItr->second);
        }

        auto applyMinfo = i2c::find_method((Il2CppObject*) component, {"ApplyValue", 0});
        auto receiveMinfo = i2c::find_method((Il2CppObject*) component, {"ReceiveValue", 0});

        auto idItr = data.find("id");
        if (idItr != data.end()) {
            std::string id = idItr->second;

            if (applyMinfo) parserParams.AddAction(id + "#Apply", new BSMLAction(component, applyMinfo));
            if (receiveMinfo) parserParams.AddAction(id + "#Receive", new BSMLAction(component, receiveMinfo));
        }

        if (applyMinfo) {
            auto setEventItr = data.find("setEvent");
            std::string setEventName = "apply";
            if (setEventItr != data.end()) {
                setEventName = setEventItr->second;
            }
            parserParams.AddEvent(setEventName,
                [component, applyMinfo] mutable {
                    i2c::run_method(component, applyMinfo);
                }
            );
        }

        if (receiveMinfo) {
            auto getEventItr = data.find("getEvent");
            std::string getEventName = "cancel";
            if (getEventItr != data.end()) {
                getEventName = getEventItr->second;
            }
            parserParams.AddEvent(getEventName,
                [component, receiveMinfo] mutable {
                    i2c::run_method(component, receiveMinfo);
                }
            );
        }

        Base::HandleType(componentType, parserParams);
    }

    void BaseSettingHandler::HandleTypeAfterChildren(const ComponentTypeWithData& componentType, BSMLParserParams& parserParams) {
        auto baseSetupMethodInfo = StringParseHelper("BaseSetup").asMethodInfo(componentType.component, 0);
        if (baseSetupMethodInfo) i2c::run_method(const_cast<UnityEngine::Component*>(componentType.component), baseSetupMethodInfo);

        auto setupMethodInfo = StringParseHelper("Setup").asMethodInfo(componentType.component, 0);
        if (setupMethodInfo) i2c::run_method(const_cast<UnityEngine::Component*>(componentType.component), setupMethodInfo);

        Base::HandleTypeAfterChildren(componentType, parserParams);
    }

}
