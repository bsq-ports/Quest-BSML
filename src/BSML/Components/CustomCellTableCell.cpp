#include "BSML/Components/CustomCellTableCell.hpp"
#include "beatsaber-hook/shared/listw.hpp"

DEFINE_TYPE(BSML, CustomCellTableCell);

namespace BSML {
    void CustomCellTableCell::ctor() {
        selectedTags = ListW<UnityEngine::GameObject*>::New();
        hoveredTags = ListW<UnityEngine::GameObject*>::New();
        neitherTags = ListW<UnityEngine::GameObject*>::New();
    }

    void CustomCellTableCell::Finalize() {
        // clear saved shared ptr
        parserParams = nullptr;
        this->~CustomCellTableCell();

        auto objectFinalize = i2c::metadata_getter<&System::Object::Finalize>::method_info();
        i2c::run_method(this, objectFinalize);
    }

    void CustomCellTableCell::Reused() {
        if (dataObject) i2c::run_method((Il2CppObject*) dataObject, "Reused", this);
    }

    void CustomCellTableCell::SelectionDidChange(HMUI::SelectableCell::TransitionType transitionType) {
        if (get_selected() && dataObject) {
            i2c::run_method((Il2CppObject*) dataObject, "Select", this);
        }

        RefreshVisuals();
    }

    void CustomCellTableCell::HighlightDidChange(HMUI::SelectableCell::TransitionType transitionType) {
        RefreshVisuals();
    }

    void CustomCellTableCell::RefreshVisuals() {
        bool selected = get_selected();
        bool hovered = get_highlighted();

        for (auto go : selectedTags) go->SetActive(selected);
        for (auto go : hoveredTags) go->SetActive(hovered);
        for (auto go : neitherTags) go->SetActive(!(selected || hovered));
    }

    void CustomCellTableCell::SetupPostParse(const BSMLParserParams& parserParams) {
        auto selected = parserParams.GetObjectsWithTag("selected");
        auto hovered = parserParams.GetObjectsWithTag("hovered");
        auto neither = parserParams.GetObjectsWithTag("un-selected-un-hovered");

        selectedTags->EnsureCapacity(selected.size());
        for (auto go : selected) selectedTags->Add(go);
        hoveredTags->EnsureCapacity(hovered.size());
        for (auto go : hovered) hoveredTags->Add(go);
        neitherTags->EnsureCapacity(neither.size());
        for (auto go : neither) neitherTags->Add(go);

        if (dataObject) i2c::run_method((Il2CppObject*) dataObject, "Setup", this);
    }
}
