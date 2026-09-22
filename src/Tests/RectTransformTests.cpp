#include "BSML/Parsing/BSMLParser.hpp"
#include "BSML/Parsing/ParseException.hpp"
#include "StringParseHelper.hpp"
#include "BSML/TypeHandlers/TypeHandler.hpp"
#include "custom-types/shared/macros.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/UI/LayoutGroup.hpp"
#include "UnityEngine/UI/LayoutRebuilder.hpp"
#include "UnityEngine/TextAnchor.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "logging.hpp"
#include <cmath>
#include "System/Globalization/CultureInfo.hpp"
#include "System/Globalization/NumberFormatInfo.hpp"

DECLARE_CLASS_CODEGEN(BSML, RectTransformTestHost, System::Object) {
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, sample);
    DECLARE_INSTANCE_FIELD(StringW, objectName);
    DECLARE_INSTANCE_FIELD(StringW, nullString);
    DECLARE_INSTANCE_FIELD(System::Object*, nullObject);
    DECLARE_INSTANCE_FIELD(System::Object*, numericObject);
    DECLARE_INSTANCE_FIELD(System::Object*, _captorClanTag);
    DECLARE_CTOR(ctor);
};
DEFINE_TYPE(BSML, RectTransformTestHost);
void BSML::RectTransformTestHost::ctor() {
    objectName = "BoundObjectName";
    nullString = nullptr;
    nullObject = nullptr;
    double number = 1.5;
    numericObject = reinterpret_cast<System::Object*>(i2c::functions::value_box(i2c::class_of<double>(), &number));
    _captorClanTag = this;
}

namespace BSML {
    // These tests inspect real parsed Unity objects, including layout-driven geometry.
    std::string RunRectTransformTests() {
        using namespace UnityEngine;
        int passed = 0, failed = 0;
        std::string failures;
        auto check = [&](const std::string& label, bool matches) {
            if (matches) ++passed;
            else { ++failed; failures += "\nFAIL: " + label; }
            INFO("RECT-TEST {}: {}", matches ? "PASS" : "FAIL", label);
        };
        auto near = [](float a, float b) { return std::abs(a - b) < 0.01f; };
        auto vector = [&](const std::string& label, Vector2 actual, Vector2 expected) {
            check(fmt::format("{} ({:.2f}, {:.2f}; expected {:.2f}, {:.2f})", label, actual.x, actual.y, expected.x, expected.y),
                near(actual.x, expected.x) && near(actual.y, expected.y));
        };
        auto construct = [&](const std::string& markup, auto inspect) {
            auto root = GameObject::New_ctor("BSMLRectTransformTest");
            auto parent = root->AddComponent<RectTransform*>();
            parent->set_sizeDelta({200, 120});
            auto host = RectTransformTestHost::New_ctor();
            try {
                auto parser = BSMLParser::parse_and_construct(markup, parent, host);
                check("id binding: " + markup, host->sample != nullptr);
                if (host->sample) inspect(host->sample->transform.cast<RectTransform>(), parent);
            } catch (const std::exception& error) {
                check(fmt::format("unexpected exception for {}: {}", markup, error.what()), false);
            }
            Object::DestroyImmediate(root);
        };

        auto reject = [&](const std::string& markup, const std::string& expectedMessage) {
            auto root = GameObject::New_ctor("BSMLInvalidAttributeTest");
            auto parent = root->AddComponent<RectTransform*>();
            auto host = RectTransformTestHost::New_ctor();
            try {
                BSMLParser::parse_and_construct(markup, parent, host);
                check("must reject " + markup, false);
            } catch (const ParseException& error) {
                check("reject " + markup, true);
                check("error identifies " + expectedMessage, std::string(error.what()).find(expectedMessage) != std::string::npos);
            } catch (const std::exception& error) {
                check(fmt::format("wrong exception for {}: {}", markup, error.what()), false);
            }
            Object::DestroyImmediate(root);
        };

        {
            struct OrderProbe : TypeHandler<RectTransform*> {
                std::vector<std::string> afterChildren;
                std::vector<std::string> afterParse;
                PropMap get_props() const override { return {{"order-probe", {"order-probe"}}}; }
                SetterMap get_setters() const override { return {}; }
                void HandleTypeAfterChildren(const ComponentTypeWithData& data, BSMLParserParams&) override {
                    if (auto it = data.data.find("order-probe"); it != data.data.end()) afterChildren.push_back(it->second);
                }
                void HandleTypeAfterParse(const ComponentTypeWithData& data, BSMLParserParams&) override {
                    if (auto it = data.data.find("order-probe"); it != data.data.end()) afterParse.push_back(it->second);
                }
            } probe;
            construct("<horizontal id='sample' order-probe='parent'><horizontal order-probe='first'><text order-probe='grandchild'/></horizontal><text order-probe='second'/></horizontal><text order-probe='next-root'/>", [&](auto, auto) {
                check("after-parse preserves PC parent-first document order", probe.afterParse == std::vector<std::string>{"parent", "first", "grandchild", "second", "next-root"});
                check("after-children retains child-first document order", probe.afterChildren == std::vector<std::string>{"grandchild", "first", "second", "parent", "next-root"});
            });
        }

        {
            using System::Globalization::CultureInfo;
            struct RestoreCulture {
                CultureInfo* previous = CultureInfo::get_CurrentCulture();
                ~RestoreCulture() { CultureInfo::set_CurrentCulture(previous); }
            } restore;
            CultureInfo::set_CurrentCulture(CultureInfo::New_ctor("fr-FR"));
            auto host = RectTransformTestHost::New_ctor();
            auto oldFrench = std::string(host->numericObject->ToString());
            CultureInfo::set_CurrentCulture(CultureInfo::get_InvariantCulture());
            auto oldInvariant = std::string(host->numericObject->ToString());
            CultureInfo::set_CurrentCulture(CultureInfo::New_ctor("fr-FR"));
            auto toString = i2c::functions::class_get_method_from_name(host->numericObject->klass, "ToString", 0);
            Il2CppException* formatError = nullptr;
            auto ordinaryFrench = reinterpret_cast<Il2CppString*>(i2c::functions::runtime_invoke(toString,
                i2c::functions::object_unbox(reinterpret_cast<Il2CppObject*>(host->numericObject)), nullptr, &formatError));
            if (formatError) throw ParseException(i2c::exception_to_string(formatError));
            std::string ordinaryText = StringW(ordinaryFrench);
            INFO("CULTURE-AB boxed 1.5: old French='{}', old invariant='{}', correctly invoked ordinary French='{}', old French accepted as float={}",
                oldFrench, oldInvariant, ordinaryText, StringParseHelper(oldFrench).tryParseFloat().has_value());
            check("ordinary managed formatting uses the French decimal comma", ordinaryText == "1,5");
            check("French-formatted decimal is rejected by invariant parsing", !StringParseHelper(ordinaryText).tryParseFloat());
            check("test culture uses a decimal comma", std::string(CultureInfo::get_CurrentCulture()->get_NumberFormat()->get_NumberDecimalSeparator()) == ",");
            construct("<text id='sample' text='~numericObject' font-size='~numericObject'/>", [&](auto rect, auto) {
                auto text = rect->get_gameObject()->template GetComponent<TMPro::TextMeshProUGUI*>();
                check("object binding formats a boxed decimal invariantly", text && std::string(text->get_text()) == "1.5");
                check("invariant object binding remains a valid numeric attribute", text && near(text->get_fontSize(), 1.5f));
            });
            construct("<text id='sample' text='~_captorClanTag'/>", [&](auto rect, auto) {
                auto text = rect->get_gameObject()->template GetComponent<TMPro::TextMeshProUGUI*>();
                check("nonconvertible object binding preserves ToString", text && std::string(text->get_text()) == std::string(host->ToString()));
            });
        }

        for (const std::string attribute : {"anchor-min", "anchor-max", "pivot", "anchored-position", "anchor-pos", "size-delta"}) {
            for (const std::string invalid : {"", "bad", "1 bad", "bad 2", "1 2 3"})
                reject("<text " + attribute + "='" + invalid + "'/>", "Vector2");
        }
        for (const std::string invalid : {"", "bad", "1 bad", "1 2 bad", "1 2 3 4"})
            reject("<text local-scale='" + invalid + "'/>", "localScale");
        for (const std::string attribute : {"anchor-min-x", "anchor-min-y", "anchor-max-x", "anchor-max-y", "pivot-x", "pivot-y", "anchored-position-x", "anchored-position-y", "anchor-pos-x", "anchor-pos-y", "size-delta-x", "size-delta-y"}) {
            for (const std::string invalid : {"", "bad", "1 2"})
                reject("<text " + attribute + "='" + invalid + "'/>", "float");
        }
        for (const std::string invalid : {"", "bad"}) {
            reject("<text active='" + invalid + "'/>", "active");
            for (const std::string attribute : {"align", "font-align"})
                reject("<text " + attribute + "='" + invalid + "'/>", "alignment");
            for (const std::string attribute : {"child-align", "child-alignment"})
                reject("<horizontal " + attribute + "='" + invalid + "'/>", "child-alignment");
        }
        // Every supplied typed value must fail rather than silently choosing a default.
        for (const std::string attribute : {"font-size", "outline-width", "line-spacing"})
            reject("<text " + attribute + "='bad'/>", "float");
        // LayoutElement attributes only apply to tags that carry that component.
        reject("<horizontal pref-width='bad'/>", "float");
        for (const std::string invalid : {"0x1p2", "1e999", "1e40", "1,5", "+-1"})
            reject("<text font-size='" + invalid + "'/>", "float");
        for (const std::string attribute : {"rich-text", "word-wrapping", "bold"})
            reject("<text " + attribute + "='bad'/>", "bool");
        for (const std::string attribute : {"color", "face-color", "outline-color"})
            reject("<text " + attribute + "='not-a-color'/>", "color");
        reject("<text overflow-mode='invalid'/>", "overflow mode");
        reject("<text align='TopLeft, BottomRight'/>", "alignment");
        reject("<horizontal child-align='UpperLeft, LowerRight'/>", "child-alignment");
        reject("<horizontal horizontal-fit='invalid'/>", "horizontal-fit");
        reject("<horizontal vertical-fit='invalid'/>", "vertical-fit");
        for (const std::string invalid : {"", "bad", "1 bad", "1 2 3 4 5", "2147483648"})
            reject("<horizontal pad='" + invalid + "'/>", "padding");
        for (const std::string attribute : {"pad-top", "pad-bottom", "pad-left", "pad-right"})
            reject("<horizontal " + attribute + "='2147483648'/>", "integer");
        for (const std::string attribute : {"increment", "min", "max", "integer-only", "digits", "apply-on-change"})
            reject("<increment-setting " + attribute + "='bad'/>", "parse");
        reject("<page-button direction='invalid'/>", "direction");
        reject("<list list-direction='invalid'/>", "list direction");
        reject("<list list-style='invalid'/>", "list style");
        reject("<scroll-indicator handle-color='not-a-color'/>", "color");
        for (const std::string malformed : {"", "<!-- no element -->", "<text/ >", "<text/>trailing text", "<![CDATA[text]]><text/>", "<text>", "<vertical><text/></horizontal>", "<text text='broken>"})
            reject(malformed, "XML");
        reject("<not-a-bsml-tag/>", "not-a-bsml-tag");
        reject("<vertical><text/><not-a-bsml-tag/></vertical>", "not-a-bsml-tag");
        construct("<text id='sample' font-size=' +3.5e1 ' rich-text=' TRUE ' color='#12345678' align=' Center '/>", [&](auto rect, auto) {
            auto text = rect->template GetComponent<TMPro::TextMeshProUGUI*>();
            check("valid numeric, bool, color and enum values", text && near(text->get_fontSize(), 35)
                && text->get_richText() && near(text->get_color().a, 120.0f / 255));
        });
        for (const std::string field : {"nullString", "nullObject"}) {
            for (const std::string attribute : {"name", "pivot", "anchored-position", "local-scale", "align", "active"})
                reject("<text " + attribute + "='~" + field + "'/>", field + "' is null");
        }
        for (const std::string attribute : {"name", "text", "pivot", "anchored-position", "local-scale", "align", "active"}) {
            reject("<text " + attribute + "='~missingValue'/>", "could not find value 'missingValue'");
        }
        reject("<text text='~version'/>", "could not find value 'version'");
        reject("<text text='~'/>", "could not find value ''");
        reject("<text text='~missingValue' label='Literal alias'/>", "Attribute 'text'");
        construct("<macro.define name='definedName' value='MacroName'/><text id='sample' name='~definedName'/>", [&](auto rect, auto) {
            check("macro-defined values remain readable", std::string(rect->get_name()) == "MacroName");
        });
        construct("<macro.as-host host='_captorClanTag'><text id='sample' name='~objectName'/></macro.as-host>", [&](auto rect, auto) {
            check("as-host resolves a named host without integer conversion", std::string(rect->get_name()) == "BoundObjectName");
        });
        reject("<macro.as-host host='missingHost'><text/></macro.as-host>", "could not find value 'missingHost'");
        reject("<macro.as-host host=''><text/></macro.as-host>", "could not find value ''");
        reject("<macro.as-host host='~missingHost'><text/></macro.as-host>", "Attribute 'host'");
        reject("<macro.define name='outerOnly' value='secret'/><macro.as-host host='_captorClanTag'><text text='~outerOnly'/></macro.as-host>", "could not find value 'outerOnly'");
        construct("<macro.define name='hostName' value='_captorClanTag'/><macro.as-host host='~hostName'><text id='sample' name='~objectName'/></macro.as-host>", [&](auto rect, auto) {
            check("as-host resolves a bound host name using the host property", std::string(rect->get_name()) == "BoundObjectName");
        });
        construct("<macro.as-host><text name='Skipped'/></macro.as-host><text id='sample'/>", [&](auto, auto parent) {
            check("as-host without host attribute skips children", parent->get_childCount() == 1);
        });
        construct("<macro.as-host host='nullObject'><text name='NullHostChild'/></macro.as-host><text id='sample'/>", [&](auto, auto parent) {
            check("as-host with an existing null value still constructs children", parent->Find("NullHostChild") != nullptr && parent->get_childCount() == 2);
        });
        reject("<macro.as-host host='nullObject'><text text='~objectName'/></macro.as-host>", "could not find value 'objectName'");
        {
            auto root = GameObject::New_ctor("BSMLAsHostScopeTest");
            auto parent = root->AddComponent<RectTransform*>();
            auto outer = RectTransformTestHost::New_ctor();
            auto inner = RectTransformTestHost::New_ctor();
            inner->objectName = "InnerHostName";
            outer->_captorClanTag = inner;
            try {
                BSMLParser::parse_and_construct("<macro.as-host host='_captorClanTag'><text id='sample' name='~objectName'/></macro.as-host><text id='sample' name='~objectName'/>", parent, outer);
                check("as-host binds children to the selected host", inner->sample && std::string(inner->sample->get_name()) == "InnerHostName");
                check("as-host preserves the outer host for following siblings", outer->sample && std::string(outer->sample->get_name()) == "BoundObjectName");
            } catch (const std::exception& error) {
                check(fmt::format("unexpected as-host exception: {}", error.what()), false);
            }
            Object::DestroyImmediate(root);
        }
        // Valid names and omitted values must remain accepted after invalid inputs.
        construct("<text id='sample' pivot=' 0.25  0.75 ' anchored-position=' 13 -7 ' local-scale=' 2 3 ' active=' TRUE '/>", [&](auto rect, auto) {
            vector("whitespace in pivot", rect->get_pivot(), {.25f, .75f});
            vector("whitespace in position", rect->get_anchoredPosition(), {13, -7});
            auto scale = rect->get_localScale();
            check("two-component scale defaults only Z", near(scale.x, 2) && near(scale.y, 3) && near(scale.z, 1));
        });
        construct("<text id='sample' pivot='0.25' anchor-pos='7' local-scale='2'/>", [&](auto rect, auto) {
            vector("scalar pivot", rect->get_pivot(), {.25f, .25f});
            vector("scalar position", rect->get_anchoredPosition(), {7, 7});
            auto scale = rect->get_localScale();
            check("scalar scale", near(scale.x, 2) && near(scale.y, 2) && near(scale.z, 2));
        });
        for (const std::string input : {"", "bad", "1 bad", "1 2 3 4"}) {
            check("tryParseVector2 remains nonthrowing: " + input, !StringParseHelper(input).tryParseVector2());
            check("tryParseVector3 remains nonthrowing: " + input, !StringParseHelper(input).tryParseVector3(1));
        }

        for (const std::string tag : {"text", "button", "vertical"}) {
            for (const std::string name : {"LiteralObjectName", "~objectName", ""}) {
                construct("<" + tag + " id='sample' name='" + name + "' text='Visible label'/>", [&](auto rect, auto) {
                    auto expected = name == "~objectName" ? "BoundObjectName" : name;
                    check(tag + " name '" + name + "'", std::string(rect->get_name()) == expected);
                    if (tag != "vertical") {
                        auto text = rect->template GetComponentInChildren<TMPro::TextMeshProUGUI*>();
                        check(tag + " keeps visible text", text && std::string(text->get_text()) == "Visible label");
                    }
                });
            }
        }
        construct("<text id='sample'/>", [&](auto rect, auto) {
            check("omitted name keeps default", std::string(rect->get_name()) == "BSMLText");
        });

        struct Position { const char* name; float x; float y; TextAnchor alignment; };
        const Position positions[] = {
            {"UpperLeft", 0, 1, TextAnchor::UpperLeft}, {"UpperCenter", .5f, 1, TextAnchor::UpperCenter},
            {"UpperRight", 1, 1, TextAnchor::UpperRight}, {"MiddleLeft", 0, .5f, TextAnchor::MiddleLeft},
            {"MiddleCenter", .5f, .5f, TextAnchor::MiddleCenter}, {"MiddleRight", 1, .5f, TextAnchor::MiddleRight},
            {"LowerLeft", 0, 0, TextAnchor::LowerLeft}, {"LowerCenter", .5f, 0, TextAnchor::LowerCenter},
            {"LowerRight", 1, 0, TextAnchor::LowerRight},
        };
        for (const std::string alias : {"anchored-position", "anchor-pos"}) {
            for (const auto& p : positions) {
                auto markup = fmt::format("<text id='sample' anchor-min='{0} {1}' anchor-max='{0} {1}' pivot='0.25 0.75' size-delta='32 18' {2}='13 -7'/>", p.x, p.y, alias);
                construct(markup, [&](auto rect, auto parent) {
                    std::string label = alias + " " + p.name;
                    vector(label + " min", rect->get_anchorMin(), {p.x, p.y});
                    vector(label + " max", rect->get_anchorMax(), {p.x, p.y});
                    vector(label + " pivot", rect->get_pivot(), {.25f, .75f});
                    vector(label + " size preserved", rect->get_sizeDelta(), {32, 18});
                    vector(label + " position", rect->get_anchoredPosition(), {13, -7});
                    auto local = rect->get_localPosition();
                    vector(label + " parent-relative geometry", {local.x, local.y}, {200 * (p.x - .5f) + 13, 120 * (p.y - .5f) - 7});
                    parent->set_sizeDelta({300, 180});
                    local = rect->get_localPosition();
                    vector(label + " resized parent", {local.x, local.y}, {300 * (p.x - .5f) + 13, 180 * (p.y - .5f) - 7});
                });
            }
        }
        for (const std::string alias : {"anchored-position", "anchor-pos"}) {
            construct("<text id='sample' anchored-position='3 4' " + alias + "-x='17'/>", [&](auto rect, auto) {
                vector(alias + " x preserves y", rect->get_anchoredPosition(), {17, 4});
            });
            construct("<text id='sample' anchored-position='3 4' " + alias + "-y='-9'/>", [&](auto rect, auto) {
                vector(alias + " y preserves x", rect->get_anchoredPosition(), {3, -9});
            });
        }
        construct("<text id='sample' anchor-pos='90 90' anchored-position='3 4'/>", [&](auto rect, auto) {
            vector("PC alias wins when both are supplied", rect->get_anchoredPosition(), {3, 4});
        });
        construct("<text id='sample' pivot='0.25 0.75' pivot-x='0.1' size-delta='32 18'/>", [&](auto rect, auto) {
            vector("pivot-x preserves y", rect->get_pivot(), {.1f, .75f});
            vector("pivot leaves size intact", rect->get_sizeDelta(), {32, 18});
        });
        construct("<text id='sample' pivot='0.25 0.75' pivot-y='0.9' size-delta='32 18'/>", [&](auto rect, auto) {
            vector("pivot-y preserves x", rect->get_pivot(), {.25f, .9f});
        });
        construct("<text id='sample' anchor-min='0.25 0.2' anchor-max='0.75 0.8' pivot='0.25 0.75' size-delta='-10 -8' anchored-position='13 -7'/>", [&](auto rect, auto parent) {
            vector("stretched size", {rect->get_rect().get_width(), rect->get_rect().get_height()}, {90, 64});
            auto local = rect->get_localPosition();
            vector("stretched anchor reference uses pivot", {local.x, local.y}, {-12, 11});
            parent->set_sizeDelta({300, 180});
            vector("stretched size after resize", {rect->get_rect().get_width(), rect->get_rect().get_height()}, {140, 100});
        });

        for (const std::string alias : {"child-alignment", "child-align"}) {
            for (const auto& p : positions) {
                auto markup = fmt::format("<horizontal id='sample' anchor-min='0.5 0.5' anchor-max='0.5 0.5' size-delta='100 60' anchored-position='13 -7' horizontal-fit='Unconstrained' vertical-fit='Unconstrained' {0}='{1}' pad='2' child-control-width='false' child-control-height='false' child-expand-width='false' child-expand-height='false'><text size-delta='20 10' pivot='0.25 0.75' anchored-position='33 44'/></horizontal>", alias, p.name);
                construct(markup, [&](auto rect, auto) {
                    UI::LayoutRebuilder::ForceRebuildLayoutImmediate(rect);
                    std::string label = alias + " " + p.name;
                    auto layout = rect->template GetComponent<UI::LayoutGroup*>();
                    check(label + " enum", layout && layout->get_childAlignment() == p.alignment);
                    vector(label + " container position preserved", rect->get_anchoredPosition(), {13, -7});
                    auto child = rect->GetChild(0).template cast<RectTransform>();
                    vector(label + " child size", child->get_sizeDelta(), {20, 10});
                    vector(label + " layout drives child position", child->get_anchoredPosition(), {7 + 76 * p.x, -4.5f - 46 * (1 - p.y)});
                });
            }
        }
        for (const std::string alias : {"align", "font-align"}) {
            construct("<text id='sample' " + alias + "='BottomRight' anchored-position='13 -7'/>", [&](auto rect, auto) {
                auto text = rect->template GetComponent<TMPro::TextMeshProUGUI*>();
                check(alias + " text alignment", text && text->get_alignment() == TMPro::TextAlignmentOptions::BottomRight);
                vector(alias + " preserves anchor position", rect->get_anchoredPosition(), {13, -7});
            });
        }
        auto result = fmt::format("Names and layout: {} passed, {} failed.{}", passed, failed, failures);
        INFO("RECT-TEST RESULT: {}", result);
        return result;
    }
}
