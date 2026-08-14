#include "BSML/TypeHandlers/TextSegmentedControlHandler.hpp"
#include "beatsaber-hook/shared/listw.hpp"
#include "Helpers/delegates.hpp"

namespace BSML {
    static TextSegmentedControlHandler textSegmentedControlHandler{};

    TextSegmentedControlHandler::Base::PropMap TextSegmentedControlHandler::get_props() const {
        return {
            {"selectCell", {"select-cell", "cell-select"}},
            {"data", {"contents", "data"}}
        };
    }

    TextSegmentedControlHandler::Base::SetterMap TextSegmentedControlHandler::get_setters() const {
        return {};
    }

    void TextSegmentedControlHandler::HandleType(const ComponentTypeWithData& componentType, BSMLParserParams& parserParams) {
        auto textControl = reinterpret_cast<HMUI::TextSegmentedControl*>(componentType.component);
        auto& data = componentType.data;

        auto dataItr = data.find("data");
        if (dataItr != data.end() && !dataItr->second.empty()) {
            auto val = parserParams.TryGetValue(dataItr->second);
            auto texts = ListW<StringW>::New();
            auto data = val ? val->GetObjectList() : nullptr;

            if (data) {
                for (auto obj : data) texts->Add(obj ? obj->ToString() : StringW(""));
            } else {
                ERROR("Could not find value {}", dataItr->second);
            }

            if (texts->get_Count() > 0) {
                textControl->SetTexts(*reinterpret_cast<System::Collections::Generic::List_1<StringW>*>(texts.convert()), nullptr);
            } else {
                ERROR("TextSegmentedControl needs to have at least 1 value!");
                ERROR("This means BSML could not find field '{0}' or method 'get_{0}'", dataItr->second);
            }
        }

        auto selectCellItr = data.find("selectCell");
        if (selectCellItr != data.end() && !selectCellItr->second.empty()) {
            auto action = parserParams.TryGetAction(selectCellItr->second);
            if (action) textControl->add_didSelectCellEvent(action->GetSystemAction<UnityW<HMUI::SegmentedControl>, int>());
            else ERROR("Action '{}' could not be found", selectCellItr->second);
        }

        Base::HandleType(componentType, parserParams);
    }
}
