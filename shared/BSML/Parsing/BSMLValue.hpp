#pragma once

#include "../../_config.h"
#include "System/Object.hpp"
#include <map>

namespace BSML {
    struct BSML_EXPORT BSMLValue {
        std::string name;
        System::Object* host;

        FieldInfo* fieldInfo;
        const MethodInfo* setterInfo;
        const MethodInfo* getterInfo;

        virtual void SetValue(System::Object* val);
        virtual System::Object* GetValue();

        template<typename T>
        requires(!std::is_same_v<System::Object*, T>)
        void SetValue(T val) {
            if (fieldInfo) {
                i2c::functions::field_set_value(host, fieldInfo, &val);
            } else if (setterInfo) {
                i2c::run_method(host, setterInfo, val);
            }
        }

        template<typename T>
        requires(std::is_default_constructible_v<T> && !std::is_same_v<System::Object*, T>)
        T GetValue() {
            if (fieldInfo) {
                T val;
                i2c::functions::field_get_value(host, fieldInfo, &val);
                return val;
            } else if (getterInfo) {
                return i2c::run_method<T>(host, getterInfo);
            }
            return T{};
        }

        template<typename T>
        requires(!std::is_same_v<System::Object*, T>)
        std::optional<T> GetValueOpt() {
            if (fieldInfo) {
                if (auto value = i2c::get_field<i2c::result<T>>(host, fieldInfo))
                    return *value;
            } else if (getterInfo) {
                if (auto value = i2c::run_method<i2c::result<T>>(host, getterInfo))
                    return *value;
            }
            return std::nullopt;
        }

        static std::map<std::string, BSMLValue*> MakeValues(System::Object* host);
    };
}
