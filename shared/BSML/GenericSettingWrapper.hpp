#pragma once

#include "custom-types/shared/macros.hpp"
#include "System/Object.hpp"

DECLARE_CLASS_CODEGEN(BSML, GenericSettingWrapper, System::Object) {
    DECLARE_INSTANCE_FIELD(System::Object*, host);
    DECLARE_INSTANCE_FIELD(System::Object*, onChangeHost);
    DECLARE_INSTANCE_FIELD(bool, applyOnChange); /* default: true */
    DECLARE_CTOR(ctor);

    public:
        FieldInfo* valueInfo;
        const MethodInfo* setterInfo;
        const MethodInfo* getterInfo;
        const MethodInfo* onChangeInfo;

        System::Object* get_onChangeHost();
        System::Object* get_host();

        template<typename T>
        void SetValue(T value) {
            if (valueInfo) {
                SetField<T>(value);
            } else if (setterInfo) {
                SetProperty<T>(value);
            }
        }

        template<typename T>
        void SetField(T value) {
            i2c::set_field(get_host(), valueInfo, value);
        }

        template<typename T>
        void SetProperty(T value) {
            i2c::run_method(get_host(), setterInfo, value);
        }

        template<typename T>
        requires(std::is_default_constructible_v<T>)
        T GetValue() {
            if (valueInfo) {
                return GetField<T>();
            }
            if (getterInfo) {
                return GetProperty<T>();
            }

            return T{};
        }

        template<typename T>
        requires(std::is_default_constructible_v<T>)
        T GetField() {
            return valueInfo ? i2c::get_field<i2c::result<T>>(get_host(), valueInfo).value_or(T{}) : T{};
        }

        template<typename T>
        requires(std::is_default_constructible_v<T>)
        T GetProperty()  {
            return getterInfo ? i2c::run_method<T>(get_host(), getterInfo) : T{};
        }

        template<typename T>
        std::optional<T> GetValueOpt() {
            if (valueInfo) {
                return GetFieldOpt<T>();
            }
            if (getterInfo) {
                return GetPropertyOpt<T>();
            }
            return std::nullopt;
        }

        template<typename T>
        std::optional<T> GetFieldOpt() {
            if (valueInfo) {
                if (auto value = i2c::get_field<i2c::result<T>>(get_host(), valueInfo))
                    return *value;
            }
            return std::nullopt;
        }

        template<typename T>
        std::optional<T> GetPropertyOpt()  {
            if (getterInfo) {
                if (auto value = i2c::run_method<i2c::result<T>>(get_host(), getterInfo))
                    return *value;
            }
            return std::nullopt;
        }

        void OnChange() {
            if (onChangeInfo) i2c::run_method(get_onChangeHost(), onChangeInfo);
        }

        template<typename T>
        void OnChange(T value) {
            // ìf the given onchange info only has 1 parameter (implicit this ptr) we just use the OnChange(void) method
            if (onChangeInfo && onChangeInfo->parameters_count < 1) {
                OnChange();
                return;
            } else if (onChangeInfo){
                i2c::run_method(get_onChangeHost(), onChangeInfo, value);
            }
        }
};
