#include "BSML/ViewControllers/BSMLViewController.hpp"
#include "Helpers/creation.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "logging.hpp"

DECLARE_CLASS_CUSTOM(BSML, SnapshotBindingTestView, BSML::BSMLViewController) {
    DECLARE_INSTANCE_FIELD(StringW, message);
    DECLARE_INSTANCE_FIELD(int, count);
    DECLARE_INSTANCE_FIELD(bool, enabledValue);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, first);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, second);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, number);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Button*, button);
    DECLARE_INSTANCE_METHOD(int, get_countValue);
    DECLARE_INSTANCE_METHOD(StringW, get_Content);
};
DEFINE_TYPE(BSML, SnapshotBindingTestView);

int BSML::SnapshotBindingTestView::get_countValue() { return count; }
StringW BSML::SnapshotBindingTestView::get_Content() {
    return "<text id='first' text='~message'/>"
        "<text id='second' text='~message'/>"
        "<text id='number' text='~countValue'/>"
        "<button id='button' text='Bound button' interactable='~enabledValue'/>";
}

namespace BSML {
    std::string RunSnapshotBindingTests() {
        int passed = 0, failed = 0;
        auto check = [&](std::string_view label, bool condition) {
            condition ? ++passed : ++failed;
            INFO("BINDING-TEST {}: {}", condition ? "PASS" : "FAIL", label);
        };
        auto view = Helpers::CreateViewController<SnapshotBindingTestView*>();
        try {
            view->message = "Initial";
            view->count = 0;
            view->enabledValue = true;
            view->ParseWithFallback();
            if (!view->first || !view->second || !view->number || !view->button)
                throw std::runtime_error("Binding fixture did not construct all controls");
            check("initial field, getter and bool bindings", std::string(view->first->get_text()) == "Initial"
                && std::string(view->second->get_text()) == "Initial"
                && std::string(view->number->get_text()) == "0" && view->button->get_interactable());
            auto original = view->contentObject;
            view->message = "Updated";
            view->count = 7;
            view->enabledValue = false;
            check("changing host values leaves constructed controls unchanged", std::string(view->first->get_text()) == "Initial"
                && std::string(view->number->get_text()) == "0" && view->button->get_interactable()
                && view->contentObject == original);
            view->ParseWithFallback();
            check("reconstructing reads updated field values", std::string(view->first->get_text()) == "Updated"
                && std::string(view->second->get_text()) == "Updated");
            check("reconstructing reads updated getter and bool values", std::string(view->number->get_text()) == "7"
                && !view->button->get_interactable());
            check("reconstructing replaces and hides old controls", view->contentObject != original && !original->get_activeSelf());
        } catch (const std::exception& error) {
            check(fmt::format("unexpected exception: {}", error.what()), false);
        }
        UnityEngine::Object::DestroyImmediate(view->get_gameObject());
        auto result = fmt::format("Snapshot bindings: {} passed, {} failed.", passed, failed);
        INFO("BINDING-TEST RESULT: {}", result);
        return result;
    }
}
