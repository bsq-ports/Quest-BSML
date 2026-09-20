#pragma once

#include "../../_config.h"
#include "TypeHandler.hpp"
#include "../Components/CustomListTableData.hpp"
#include "HMUI/TableView.hpp"

/// @brief Parses a table type XML attribute string ("divider-line"/"radio-buttons"/etc.)
/// into an HMUI::TableView::TableType. Defined in CustomListTableDataHandler.cpp; also
/// used by CustomCellListTableDataHandler.cpp, hence the header declaration.
HMUI::TableView::TableType stringToTableType(const std::string& str);

namespace BSML {
    class BSML_EXPORT CustomListTableDataHandler : public TypeHandler<BSML::CustomListTableData*> {
        public:
            using Base = TypeHandler<BSML::CustomListTableData*>;
            CustomListTableDataHandler() : Base() {}

            virtual Base::PropMap get_props() const override;
            virtual Base::SetterMap get_setters() const override;
            virtual void HandleType(const ComponentTypeWithData& componentType, BSMLParserParams& parserParams) override;
    };
}
