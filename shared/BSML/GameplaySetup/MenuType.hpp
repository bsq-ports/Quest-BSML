#pragma once

#include "../../_config.h"
#include "beatsaber-hook/shared/types.hpp"

namespace BSML {
    enum class BSML_EXPORT MenuType {
        None = 0,
        Solo = 1,
        Online = 2,
        Campaign = 4,
        Custom = 8,
        All = Solo | Online | Campaign | Custom
    };
    bool operator!(const MenuType& type);
    MenuType operator |(const MenuType& lhs, const MenuType& rhs);
    MenuType& operator |=(MenuType& lhs, const MenuType& rhs);
    MenuType operator &(const MenuType& lhs, const MenuType& rhs);
    MenuType& operator &=(MenuType& lhs, const MenuType& rhs);
}

template<>
struct ::i2c::type_check::no_arg_class<BSML::MenuType> {
    static inline Il2CppClass* get() {
        return i2c::class_of<int>();
    }
};
static_assert(sizeof(BSML::MenuType) == sizeof(int));
