#include "BSML/ViewControllers/BSMLViewController.hpp"
#include "BSML/Parsing/BSMLParser.hpp"
#include "BSML/Parsing/ParseException.hpp"
#include "Helpers/creation.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Vector2.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "logging.hpp"

DECLARE_CLASS_CUSTOM(BSML, FallbackTestViewController, BSML::BSMLViewController) {
    DECLARE_INSTANCE_FIELD(StringW, markup);
    DECLARE_INSTANCE_FIELD(bool, customFallback);
    DECLARE_INSTANCE_FIELD(bool, throwContent);
    DECLARE_INSTANCE_FIELD(bool, brokenFallback);
    DECLARE_INSTANCE_METHOD(StringW, get_Content);
    DECLARE_INSTANCE_METHOD(StringW, get_FallbackContent);
};
DEFINE_TYPE(BSML, FallbackTestViewController);

StringW BSML::FallbackTestViewController::get_Content() {
    if (throwContent) throw std::runtime_error("Content getter failed <&\"'>");
    return markup;
}
StringW BSML::FallbackTestViewController::get_FallbackContent() {
    if (brokenFallback) return "<text>";
    if (customFallback) return "<text text='{0}' rich-text='false'/>";
    return BSMLViewController::get_FallbackContent();
}

namespace BSML {
    BSMLViewController* UpdateViewControllerPreview(BSMLViewController* existing, UnityEngine::Transform* parent, bool invalid) {
        auto view = existing && existing->m_CachedPtr.m_value
            ? static_cast<FallbackTestViewController*>(existing)
            : Helpers::CreateViewController<FallbackTestViewController*>();
        view->get_transform()->SetParent(parent, false);
        view->markup = invalid
            ? "<text text='Partial content should disappear'/><text pivot='bad'/>"
            : "<text text='Recovered: valid content replaced the error page.' align='Center' word-wrapping='true'/>";
        view->ParseWithFallback();
        view->get_gameObject()->SetActive(true);
        return view;
    }

    std::string RunViewControllerTests() {
        int passed = 0, failed = 0;
        auto check = [&](std::string_view label, bool condition) {
            condition ? ++passed : ++failed;
            INFO("VIEW-TEST {}: {}", condition ? "PASS" : "FAIL", label);
        };
        auto view = Helpers::CreateViewController<FallbackTestViewController*>();
        auto hasText = [&](std::string_view needle) {
            for (auto text : view->contentObject->GetComponentsInChildren<TMPro::TextMeshProUGUI*>(true)) {
                if (std::string(text->get_text()).find(needle) != std::string::npos) return true;
            }
            return false;
        };
        try {
            view->markup = "<text text='Valid content'/>";
            view->DidActivate(true, true, true);
            check("first activation parses overridden Content", hasText("Valid content"));
            auto first = view->contentObject;
            view->DidActivate(false, true, true);
            check("later activation preserves contents", view->contentObject == first);
            auto rect = first->GetComponent<UnityEngine::RectTransform*>();
            auto min = rect->get_anchorMin(), max = rect->get_anchorMax();
            auto size = rect->get_sizeDelta(), pos = rect->get_anchoredPosition();
            check("contents fill controller", min.x == 0 && min.y == 0 && max.x == 1 && max.y == 1
                && size.x == 0 && size.y == 0 && pos.x == 0 && pos.y == 0);

            view->markup = "<text text='Partial content'/><text pivot='bad'/>";
            view->ParseWithFallback();
            check("parse exception produces PC error heading", hasText("Invalid BSML"));
            check("fallback displays parse error", hasText("Vector2"));
            check("partial content is removed from replacement", !hasText("Partial content"));
            check("old contents are hidden pending destruction", !first->get_activeSelf());
            check("only replacement contents are active", [&] {
                int active = 0;
                auto transform = view->get_transform();
                for (int i = 0; i < transform->get_childCount(); ++i)
                    if (transform->GetChild(i)->get_gameObject()->get_activeSelf()) ++active;
                return active == 1;
            }());

            for (const std::string invalid : {"<text>", "<unknown-tag/>", "<text font-size='bad'/>", "<text color='bad'/>", "<horizontal pad='bad'/>", "<text text='~missingValue'/>", "<macro.as-host host='missingHost'><text/></macro.as-host>"}) {
                view->markup = invalid;
                view->ParseWithFallback();
                check("all parse failures use controller fallback: " + invalid, hasText("Invalid BSML"));
            }
            view->brokenFallback = true;
            try {
                view->ParseWithFallback();
                check("malformed fallback must propagate", false);
            } catch (const ParseException&) {
                check("malformed fallback propagates without recursion", true);
            }
            view->brokenFallback = false;

            view->customFallback = true;
            view->markup = "<text pivot='&lt;&amp;&quot;&apos;&gt;'/>";
            view->ParseWithFallback();
            check("custom fallback getter is used", !hasText("Invalid BSML") && hasText("Vector2"));
            check("XML special characters survive fallback formatting", hasText("<&\"'>"));
            view->throwContent = true;
            view->ParseWithFallback();
            check("other standard exceptions are caught like PC", hasText("Content getter failed <&\"'>"));
            view->throwContent = false;
            view->markup = nullptr;
            view->ParseWithFallback();
            check("null Content produces fallback", hasText("Content is null"));

            view->markup = "<text text='Recovered'/>";
            view->ParseWithFallback();
            check("valid retry replaces fallback", hasText("Recovered") && !hasText("Vector2"));
            try {
                BSMLParser::parse_and_construct("<text pivot='bad'/>", view->contentObject->get_transform(), nullptr);
                check("direct parser keeps throwing", false);
            } catch (const ParseException&) {
                check("direct parser keeps throwing", true);
            }
            UnityEngine::Object::DestroyImmediate(view->get_gameObject());
            check("OnDestroy marks controller destroyed", view->destroyed);
            auto last = view->contentObject;
            view->ParseWithFallback();
            check("destroyed controller does not reparse", view->contentObject == last);
        } catch (const std::exception& error) {
            check(fmt::format("unexpected exception: {}", error.what()), false);
            if (view->m_CachedPtr.m_value) UnityEngine::Object::DestroyImmediate(view->get_gameObject());
        }
        auto result = fmt::format("View controller fallback: {} passed, {} failed.", passed, failed);
        INFO("VIEW-TEST RESULT: {}", result);
        return result;
    }
}
