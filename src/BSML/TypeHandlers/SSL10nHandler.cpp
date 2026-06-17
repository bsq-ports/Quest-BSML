#include "BSML/TypeHandlers/SSL10nHandler.hpp"
#include "SSL10n.hpp"

namespace BSML {
    static SSL10nHandler ssl10nHandler{};

    SSL10nHandler::Base::PropMap SSL10nHandler::get_props() const {
        return {
            {"textKey", {"text-key"}},
        };
    }

    SSL10nHandler::Base::SetterMap SSL10nHandler::get_setters() const {
        return {
            {"textKey",        [](auto component, auto value){ 
                auto text = SSL10n::GetOptional(value); 
                if(text.has_value()) {
                    component->set_text(text.value());
                }else if(component->get_text() == "BSMLText"){
                    // replace the default value to translation keys
                    component->set_text(value);
                }else{
                    // fallback behavior
                }
            }},
        };
    }
}
