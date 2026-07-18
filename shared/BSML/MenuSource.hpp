#pragma once

#include "../_config.h"
#include "beatsaber-hook/shared/types.hpp"

namespace BSML {
    enum class BSML_EXPORT MenuSource {
        BSMLContent,
        FlowCoordinator,
        ViewController,
        Method,
        Component,
    };
}

template<>
struct ::i2c::type_check::no_arg_class<BSML::MenuSource> {
    static inline Il2CppClass* get() {
        return i2c::class_of<int>();
    }
};
static_assert(sizeof(BSML::MenuSource) == sizeof(int));
