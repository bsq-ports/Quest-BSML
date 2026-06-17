#pragma once

#include "../../_config.h"
#include "TypeHandler.hpp"
#include "TMPro/TextMeshProUGUI.hpp"

namespace BSML {
    class BSML_EXPORT SSL10nHandler : public TypeHandler<TMPro::TextMeshProUGUI*> {
        public:
            using Base = TypeHandler<TMPro::TextMeshProUGUI*>;
            SSL10nHandler() : Base() {}

            virtual Base::PropMap get_props() const override;
            virtual Base::SetterMap get_setters() const override;
    };
}
