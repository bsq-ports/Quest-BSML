#include "BSML/ComponentTypeWithData.hpp"
#include "BSML/Parsing/ParseException.hpp"
#include "logging.hpp"
#include "System/IConvertible.hpp"
#include "System/Globalization/CultureInfo.hpp"

std::string BSMLValueToString(BSML::BSMLValue* value, Il2CppTypeEnum type);

namespace {
    StringW ObjectToInvariantString(System::Object* object) {
        if (!i2c::try_cast<System::IConvertible*>(object)) return object->ToString();

        // Resolve the object's implementation, including explicit IConvertible methods.
        const auto toStringSlot = i2c::metadata_getter<&System::IConvertible::ToString>::method_info()->slot;
        const auto toStringMethod = i2c::find_method(object->klass, {i2c::class_of<System::IConvertible*>(), toStringSlot});
        if (!toStringMethod) throw BSML::ParseException("Could not resolve IConvertible.ToString");

        // runtime_invoke expects the unboxed payload when the method belongs to a value type.
        void* instance = object;
        if (i2c::functions::class_is_valuetype(toStringMethod->klass)) {
            instance = i2c::functions::object_unbox(reinterpret_cast<Il2CppObject*>(object));
        }

        // BSML numbers must use a decimal point regardless of the user's locale.
        void* arguments[] = {System::Globalization::CultureInfo::get_InvariantCulture()};
        Il2CppException* exception = nullptr;
        auto result = i2c::functions::runtime_invoke(toStringMethod, instance, arguments, &exception);
        if (exception) throw BSML::ParseException(i2c::exception_to_string(exception));
        return reinterpret_cast<Il2CppString*>(result);
    }

    Il2CppTypeEnum GetValueType(const BSML::BSMLValue& value) {
        if (value.fieldInfo) return value.fieldInfo->type->type;
        if (value.getterInfo) return value.getterInfo->return_type->type;
        // Macro-defined and custom values are read through the virtual object getter.
        return Il2CppTypeEnum::IL2CPP_TYPE_OBJECT;
    }

    std::string ResolveAttributeValue(const std::string& text, const std::string& attribute,
                                     const BSML::BSMLParserParams& parserParams) {
        if (!text.starts_with('~')) return text;

        const auto valueName = text.substr(1);
        auto value = parserParams.TryGetValue(valueName);
        if (!value) {
            throw BSML::ParseException(fmt::format("Attribute '{}': could not find value '{}'", attribute, valueName));
        }

        try {
            return BSMLValueToString(value, GetValueType(*value));
        } catch (const BSML::ParseException& error) {
            throw BSML::ParseException(fmt::format("Attribute '{}': {}", attribute, error.what()));
        }
    }
}

namespace BSML {
    std::map<std::string, std::string> ComponentTypeWithData::GetParameters(const std::map<std::string, std::string>& allParams, const BSMLParserParams& parserParams, const std::map<std::string, std::vector<std::string>>& props) {
        std::map<std::string, std::string> result;

        for (const auto& [property, aliases] : props) {
            // The first supplied alias wins, in the order declared by the handler.
            for (const auto& alias : aliases) {
                const auto attribute = allParams.find(alias);
                if (attribute == allParams.end()) continue;
                result[property] = ResolveAttributeValue(attribute->second, alias, parserParams);
                break;
            }
        }

        return result;
    }
}

std::string BSMLValueToString(BSML::BSMLValue* value, Il2CppTypeEnum type) {
    switch (type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_END: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_VOID:
            return "void";
        case Il2CppTypeEnum::IL2CPP_TYPE_BOOLEAN:
            return value->GetValue<bool>() ? "true" : "false";
        case Il2CppTypeEnum::IL2CPP_TYPE_CHAR: {
            char val[3] = "\0\0";
            *reinterpret_cast<Il2CppChar*>(val) = value->GetValue<Il2CppChar>();
            return fmt::format("{}", val);
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_I1:
            return fmt::format("{}", value->GetValue<int8_t>());
        case Il2CppTypeEnum::IL2CPP_TYPE_U1:
            return fmt::format("{}", value->GetValue<uint8_t>());
        case Il2CppTypeEnum::IL2CPP_TYPE_I2:
            return fmt::format("{}", value->GetValue<int16_t>());
        case Il2CppTypeEnum::IL2CPP_TYPE_U2:
            return fmt::format("{}", value->GetValue<uint16_t>());
        case Il2CppTypeEnum::IL2CPP_TYPE_I4:
            return fmt::format("{}", value->GetValue<int32_t>());
        case Il2CppTypeEnum::IL2CPP_TYPE_U4:
            return fmt::format("{}", value->GetValue<uint32_t>());
        case Il2CppTypeEnum::IL2CPP_TYPE_I8:
            return fmt::format("{}", value->GetValue<int64_t>());
        case Il2CppTypeEnum::IL2CPP_TYPE_U8:
            return fmt::format("{}", value->GetValue<uint64_t>());
        case Il2CppTypeEnum::IL2CPP_TYPE_R4:
            return fmt::format("{}", value->GetValue<float>());
        case Il2CppTypeEnum::IL2CPP_TYPE_R8:
            return fmt::format("{}", value->GetValue<double>());
        case Il2CppTypeEnum::IL2CPP_TYPE_STRING: {
            auto text = value->GetValue<StringW>();
            if (!text) throw BSML::ParseException(fmt::format("Value '{}' is null", value->name));
            return text;
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_PTR: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_BYREF: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_VAR: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_ARRAY: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_TYPEDBYREF:
            return "UNKNOWN TYPE VALUE";
        case Il2CppTypeEnum::IL2CPP_TYPE_I:
            return fmt::format("{}", value->GetValue<int>());
        case Il2CppTypeEnum::IL2CPP_TYPE_U:
            return fmt::format("{}", value->GetValue<uint>());
        case Il2CppTypeEnum::IL2CPP_TYPE_FNPTR:
            return "0";
        case Il2CppTypeEnum::IL2CPP_TYPE_CLASS: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_OBJECT: {
            auto object = value->GetValue();
            if (!object) throw BSML::ParseException(fmt::format("Value '{}' is null", value->name));
            auto text = ObjectToInvariantString(object);
            if (!text) throw BSML::ParseException(fmt::format("Value '{}' converted to a null string", value->name));
            return text;
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_MVAR: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_CMOD_REQD: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_CMOD_OPT: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_INTERNAL: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_MODIFIER: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_SENTINEL: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_PINNED: [[fallthrough]];
        case Il2CppTypeEnum::IL2CPP_TYPE_ENUM: [[fallthrough]];
        default:
            return "UNKNOWN VALUE TYPE";
    }
}
