#pragma once

#include "../../_config.h"
#include "../TransformWrapper.hpp"
#include "../../BSML/Components/CustomListTableData.hpp"
#include <optional>

namespace BSML::Lite {

    /// @brief creates a custom list based on the level/pack/simple lists
    /// @param parent what to parent it to
    /// @param anchoredPosition position override; leave unset (default) to keep the template's natural position
    /// @param sizeDelta size override; leave unset (default) to keep the template's natural size
    /// @param onCellWithIdxClicked the callback called when a cell is clicked
    /// @param activate whether to leave the list active and ready to use (the default) or inactive —
    /// pass false if you still need to finish configuring it (e.g. setting a custom size) before it's shown
    /// @return created table data to add data to
    BSML_EXPORT BSML::CustomListTableData* CreateList(const TransformWrapper& parent, std::optional<UnityEngine::Vector2> anchoredPosition = std::nullopt, std::optional<UnityEngine::Vector2> sizeDelta = std::nullopt, std::function<void(int)> onCellWithIdxClicked = nullptr, bool activate = true);

    /// @brief creates a custom list based on the level/pack/simple lists, sized to a convenient default
    /// @param parent what to parent it to
    /// @param sizeDelta the sizeDelta
    /// @param onCellWithIdxClicked the callback called when a cell is clicked
    /// @return created table data to add data to
    static inline BSML::CustomListTableData* CreateList(const TransformWrapper& parent, UnityEngine::Vector2 sizeDelta, std::function<void(int)> onCellWithIdxClicked = nullptr) {
        return CreateList(parent, UnityEngine::Vector2{0, 0}, sizeDelta, onCellWithIdxClicked);
    }

    /// @brief creates a custom list based on the level/pack/simple lists
    /// @param parent what to parent it to
    /// @param anchoredPosition the position
    /// @param sizeDelta the sizeDelta
    /// @param onCellWithIdxClicked the callback called when a cell is clicked
    /// @return created table data to add data to
    BSML_EXPORT BSML::CustomListTableData* CreateScrollableList(const TransformWrapper& parent, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, std::function<void(int)> onCellWithIdxClicked = nullptr);

    /// @brief creates a custom list based on the level/pack/simple lists
    /// @param parent what to parent it to
    /// @param sizeDelta the sizeDelta
    /// @param onCellWithIdxClicked the callback called when a cell is clicked
    /// @return created table data to add data to
    static inline BSML::CustomListTableData* CreateScrollableList(const TransformWrapper& parent, UnityEngine::Vector2 sizeDelta, std::function<void(int)> onCellWithIdxClicked = nullptr) {
        return CreateScrollableList(parent, {0, 0}, sizeDelta, onCellWithIdxClicked);
    }

    /// @brief creates a custom list based on the level/pack/simple lists
    /// @param parent what to parent it to
    /// @param onCellWithIdxClicked the callback called when a cell is clicked
    /// @return created table data to add data to
    static inline BSML::CustomListTableData* CreateScrollableList(const TransformWrapper& parent, std::function<void(int)> onCellWithIdxClicked = nullptr) {
        return CreateScrollableList(parent, {0, 0}, {35.0f, 60.0f}, onCellWithIdxClicked);
    }

    /// @brief creates a List with a custom HMUI::TableView::IDataSource
    /// @param type type that implements HMUI::TableView::IDataSource & is a component (Monobehaviour)
    /// @param parent what to parent it to
    /// @param anchoredPosition the position
    /// @param sizeDelta the sizeDelta
    /// @param onCellWithIdxClicked the callback called when a cell is clicked
    BSML_EXPORT HMUI::TableView::IDataSource* CreateCustomSourceList(System::Type* type, const TransformWrapper& parent, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, std::function<void(int)> onCellWithIdxClicked = nullptr);

    /// @brief concept to ensure you pass a valid IDataSource
    template<typename T>
    concept TableView_IDataSource =
    requires(T a) {
        { a->CellForIdx((HMUI::TableView*) nullptr, 0) } -> std::same_as<HMUI::TableCell*>;
        { a->NumberOfCells() } -> std::same_as<int>;
        { a->CellSize() } -> std::same_as<float>;
    } ||
    requires(T a) {
        {a->i_IDataSource() } -> std::same_as<HMUI::TableView::IDataSource*>;
    };

    /// @brief creates a List with a custom HMUI::TableView::IDataSource
    /// @param T class that implements HMUI::TableView::IDataSource & is a component (Monobehaviour)
    /// @param parent what to parent it to
    /// @param anchoredPosition the position
    /// @param sizeDelta the sizeDelta
    /// @param onCellWithIdxClicked the callback called when a cell is clicked
    template<TableView_IDataSource T>
    requires(std::is_convertible_v<T, UnityEngine::MonoBehaviour*>)
    T CreateCustomSourceList(const TransformWrapper& parent, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, std::function<void(int)> onCellWithIdxClicked = nullptr) {
        return reinterpret_cast<T>(CreateCustomSourceList(i2c::cs_type_of<T>(), parent, anchoredPosition, sizeDelta, onCellWithIdxClicked));
    }

    /// @brief creates a List with a custom HMUI::TableView::IDataSource
    /// @param T class that implements HMUI::TableView::IDataSource & is a component (Monobehaviour)
    /// @param parent what to parent it to
    /// @param sizeDelta the sizeDelta
    /// @param onCellWithIdxClicked the callback called when a cell is clicked
    template<TableView_IDataSource T>
    requires(std::is_convertible_v<T, UnityEngine::MonoBehaviour*>)
    T CreateCustomSourceList(const TransformWrapper& parent, UnityEngine::Vector2 sizeDelta, std::function<void(int)> onCellWithIdxClicked = nullptr) {
        return reinterpret_cast<T>(CreateCustomSourceList(i2c::cs_type_of<T>(), parent, {0, 0}, sizeDelta, onCellWithIdxClicked));
    }

    /// @brief creates a List with a custom HMUI::TableView::IDataSource
    /// @param T class that implements HMUI::TableView::IDataSource & is a component (Monobehaviour)
    /// @param parent what to parent it to
    /// @param onCellWithIdxClicked the callback called when a cell is clicked
    template<TableView_IDataSource T>
    requires(std::is_convertible_v<T, UnityEngine::MonoBehaviour*>)
    T CreateCustomSourceList(const TransformWrapper& parent, std::function<void(int)> onCellWithIdxClicked = nullptr) {
        return reinterpret_cast<T>(CreateCustomSourceList(i2c::cs_type_of<T>(), parent, {0, 0}, {35.0f, 60.0f}, onCellWithIdxClicked));
    }

    /// @brief creates a List with a custom IDataSource
    /// @param type type of class that implements HMUI::TableView::IDataSource
    /// @param parent what to parent it to
    /// @param anchoredPosition the position
    /// @param sizeDelta the sizeDelta
    /// @param onCellWithIdxClicked the callback called when a cell is clicked
    /// @return created IDataSource, as resulting from the passed type
    BSML_EXPORT HMUI::TableView::IDataSource* CreateScrollableCustomSourceList(System::Type* type, const TransformWrapper& parent, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, std::function<void(int)> onCellWithIdxClicked = nullptr);

    /// @brief creates a List with a custom IDataSource
    /// @tparam T type of class that implements HMUI::TableView::IDataSource
    /// @param parent what to parent it to
    /// @param anchoredPosition the position
    /// @param sizeDelta the sizeDelta
    /// @param onCellWithIdxClicked the callback called when a cell is clicked
    /// @return created T
    template<TableView_IDataSource T>
    requires(std::is_convertible_v<T, UnityEngine::MonoBehaviour*>)
    T CreateScrollableCustomSourceList(const TransformWrapper& parent, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, std::function<void(int)> onCellWithIdxClicked = nullptr) {
        return reinterpret_cast<T>(CreateScrollableCustomSourceList(i2c::cs_type_of<T>(), parent, anchoredPosition, sizeDelta, onCellWithIdxClicked));
    }

    /// @brief creates a List with a custom IDataSource
    /// @tparam T type of class that implements HMUI::TableView::IDataSource
    /// @param parent what to parent it to
    /// @param sizeDelta the sizeDelta
    /// @param onCellWithIdxClicked the callback called when a cell is clicked
    /// @return created T
    template<TableView_IDataSource T>
    requires(std::is_convertible_v<T, UnityEngine::MonoBehaviour*>)
    T CreateScrollableCustomSourceList(const TransformWrapper& parent, UnityEngine::Vector2 sizeDelta, std::function<void(int)> onCellWithIdxClicked = nullptr) {
        return reinterpret_cast<T>(CreateScrollableCustomSourceList(i2c::cs_type_of<T>(), parent, {0, 0}, sizeDelta, onCellWithIdxClicked));
    }

    /// @brief creates a List with a custom IDataSource
    /// @tparam T type of class that implements HMUI::TableView::IDataSource
    /// @param parent what to parent it to
    /// @param onCellWithIdxClicked the callback called when a cell is clicked
    /// @return created T
    template<TableView_IDataSource T>
    requires(std::is_convertible_v<T, UnityEngine::MonoBehaviour*>)
    T CreateScrollableCustomSourceList(const TransformWrapper& parent, std::function<void(int)> onCellWithIdxClicked = nullptr) {
        return reinterpret_cast<T>(CreateScrollableCustomSourceList(i2c::cs_type_of<T>(), parent, {0, 0}, {35.0f, 65.0f}, onCellWithIdxClicked));
    }

    /// @brief creates a custom list whose cells are described by an embedded BSML XML string
    /// (each top-level child element becomes a cell template, mirroring the <custom-list> tag)
    /// @param parent what to parent it to
    /// @param bsmlString raw BSML XML describing the cell template(s)
    /// @return created container GameObject (the actual list GameObject is parented under it)
    BSML_EXPORT UnityEngine::GameObject* CreateCustomList(const TransformWrapper& parent, std::string_view bsmlString);
}
