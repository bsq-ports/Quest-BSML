#include "BSML/TypeHandlers/ModalViewHandler.hpp"

namespace BSML {
    static ModalViewHandler modalViewHandler{};

    ModalViewHandler::Base::PropMap ModalViewHandler::get_props() const {
        return {
            {"id", {"id"}},
            {"showEvent", {"show-event"}},
            {"hideEvent", {"hide-event"}},
            {"clickOffCloses", {"click-off-closes", "clickerino-offerino-closerino"}},
            {"moveToCenter", {"move-to-center"}}
        };
    }

    ModalViewHandler::Base::SetterMap ModalViewHandler::get_setters() const {
        return {
            {"clickOffCloses", [](auto component, auto value){ component->dismissOnBlockerClicked = value; }},
            {"moveToCenter", [](auto component, auto value){ component->moveToCenter = value; }},
        };
    }

    void ModalViewHandler::HandleType(const ComponentTypeWithData& componentType, BSMLParserParams& parserParams) {
        Base::HandleType(componentType, parserParams);
        auto modalView = i2c::try_cast<BSML::ModalView*>(componentType.component);
        if (!modalView) {
            ERROR("ModalViewHandler::HandleType given a component that is not a ModalView");
            return;
        }
        auto& data = componentType.data;

        // Match PC's markup default; the setter below still honors an explicit false.
        modalView->moveToCenter = true;

        auto showEventItr = data.find("showEvent");
        if (showEventItr != data.end()) {
            parserParams.AddEvent(showEventItr->second, std::bind(&BSML::ModalView::Show, modalView));
        }

        auto hideEventItr = data.find("hideEvent");
        if (hideEventItr != data.end()) {
            parserParams.AddEvent(hideEventItr->second, std::bind(&BSML::ModalView::Hide, modalView));
        }

        auto idItr = data.find("id");
        if (idItr != data.end()) {
            std::string id = idItr->second;
            auto showMinfo = i2c::find_method((Il2CppObject*) modalView, {"Show", 0});
            auto hideMinfo = i2c::find_method((Il2CppObject*) modalView, {"Hide", 0});

            if (showMinfo) parserParams.AddAction(id + "#Show", new BSMLAction(modalView, showMinfo));
            if (hideMinfo) parserParams.AddAction(id + "#Hide", new BSMLAction(modalView, hideMinfo));
        }
    }
}
