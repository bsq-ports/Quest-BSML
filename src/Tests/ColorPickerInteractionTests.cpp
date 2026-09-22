#include "BSML.hpp"
#include "BSMLDataCache.hpp"
#include "BSML/SharedCoroutineStarter.hpp"
#include "BSML/Components/ExternalComponents.hpp"
#include "BSML/Components/Settings/ColorSetting.hpp"
#include "BSML/TypeHandlers/ModalColorPickerHandler.hpp"
#include "BSML-Lite/Creation/Settings.hpp"
#include "GlobalNamespace/ColorHueSlider.hpp"
#include "GlobalNamespace/ColorSaturationValueSlider.hpp"
#include "HMUI/ColorGradientSlider.hpp"
#include "HMUI/ScrollView.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/EventSystems/EventSystem.hpp"
#include "UnityEngine/EventSystems/PointerEventData.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "UnityEngine/WaitForSecondsRealtime.hpp"
#include "logging.hpp"
#include <cmath>
#include <vector>

static bool SameColor(UnityEngine::Color a, UnityEngine::Color b) {
    return std::abs(a.r-b.r) < 0.001f && std::abs(a.g-b.g) < 0.001f
        && std::abs(a.b-b.b) < 0.001f && std::abs(a.a-b.a) < 0.001f;
}

DECLARE_CLASS_CODEGEN(BSML, ColorInteractionProbe, System::Object) {
    DECLARE_CTOR(ctor);
    DECLARE_INSTANCE_FIELD(int, changes);
    DECLARE_INSTANCE_FIELD(int, dones);
    DECLARE_INSTANCE_FIELD(int, cancels);
    DECLARE_INSTANCE_FIELD(int, changesAtCancel);
    DECLARE_INSTANCE_FIELD(UnityEngine::Color, preview);
    DECLARE_INSTANCE_FIELD(UnityEngine::Color, saved);
    DECLARE_INSTANCE_FIELD(bool, committedBeforeDone);
    DECLARE_INSTANCE_FIELD(BSML::GenericSettingWrapper*, binding);
    DECLARE_INSTANCE_FIELD(BSML::ColorSetting*, setting);
    DECLARE_INSTANCE_METHOD(void, Changed, UnityEngine::Color color);
    DECLARE_INSTANCE_METHOD(void, Done, UnityEngine::Color color);
    DECLARE_INSTANCE_METHOD(void, Cancelled);
};
DEFINE_TYPE(BSML, ColorInteractionProbe);

void BSML::ColorInteractionProbe::ctor() {}
void BSML::ColorInteractionProbe::Changed(UnityEngine::Color color) { ++changes; preview = color; }
void BSML::ColorInteractionProbe::Done(UnityEngine::Color color) {
    ++dones;
    saved = color; // Simulates persistence owned by a Lite caller.
    committedBeforeDone = (!binding || SameColor(binding->GetValue<UnityEngine::Color>(), color))
        && (!setting || SameColor(setting->get_currentColor(), color));
}
void BSML::ColorInteractionProbe::Cancelled() { ++cancels; changesAtCancel = changes; }

DECLARE_CLASS_CODEGEN(BSML, ColorInteractionTests, System::Object) {
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, testScroll);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, results);
    DECLARE_INSTANCE_FIELD(UnityEngine::Transform*, liteContainer);
    DECLARE_INSTANCE_FIELD(BSML::ColorSetting*, immediate);
    DECLARE_INSTANCE_FIELD(BSML::ModalColorPicker*, deferredPicker);
    DECLARE_INSTANCE_FIELD(BSML::ColorSetting*, deferred);
    DECLARE_INSTANCE_FIELD(BSML::ModalColorPicker*, standalone);
    DECLARE_INSTANCE_FIELD(BSML::ModalColorPicker*, uncentered);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Button*, applyDeferred);
    DECLARE_INSTANCE_FIELD(BSML::ColorSetting*, liteRow);
    DECLARE_INSTANCE_FIELD(BSML::ModalColorPicker*, liteModal);
    DECLARE_INSTANCE_FIELD(BSML::ColorSetting*, liteNoCallbacks);
    DECLARE_INSTANCE_FIELD(BSML::ModalColorPicker*, modalNoCallbacks);
    DECLARE_INSTANCE_FIELD(BSML::ColorInteractionProbe*, immediateProbe);
    DECLARE_INSTANCE_FIELD(BSML::ColorInteractionProbe*, deferredProbe);
    DECLARE_INSTANCE_FIELD(BSML::ColorInteractionProbe*, standaloneProbe);
    DECLARE_INSTANCE_FIELD(BSML::ColorInteractionProbe*, liteRowProbe);
    DECLARE_INSTANCE_FIELD(BSML::ColorInteractionProbe*, liteModalProbe);
    DECLARE_INSTANCE_FIELD(UnityEngine::Color, immediateValue);
    DECLARE_INSTANCE_FIELD(UnityEngine::Color, deferredValue);
    DECLARE_INSTANCE_FIELD(UnityEngine::Color, standaloneValue);
    DECLARE_INSTANCE_FIELD(int, immediateWrites);
    DECLARE_INSTANCE_FIELD(int, deferredWrites);
    DECLARE_INSTANCE_FIELD(int, standaloneWrites);
    DECLARE_INSTANCE_FIELD(int, passed);
    DECLARE_INSTANCE_FIELD(int, failed);
    DECLARE_INSTANCE_FIELD(bool, running);
    DECLARE_CTOR(ctor);
    DECLARE_INSTANCE_METHOD(UnityEngine::Color, get_immediateColor);
    DECLARE_INSTANCE_METHOD(void, set_immediateColor, UnityEngine::Color color);
    DECLARE_INSTANCE_METHOD(UnityEngine::Color, get_deferredColor);
    DECLARE_INSTANCE_METHOD(void, set_deferredColor, UnityEngine::Color color);
    DECLARE_INSTANCE_METHOD(UnityEngine::Color, get_standaloneColor);
    DECLARE_INSTANCE_METHOD(void, set_standaloneColor, UnityEngine::Color color);
    DECLARE_INSTANCE_METHOD(void, ImmediateChanged, UnityEngine::Color color);
    DECLARE_INSTANCE_METHOD(void, DeferredChanged, UnityEngine::Color color);
    DECLARE_INSTANCE_METHOD(void, OpenStandalone);
    DECLARE_INSTANCE_METHOD(void, OpenLiteModal);
    DECLARE_INSTANCE_METHOD(void, ToggleLite);
    DECLARE_INSTANCE_METHOD(void, Run);
    DECLARE_INSTANCE_METHOD(void, PostParse);
    public:
        void Check(std::string_view name, bool ok);
        void Click(UnityEngine::UI::Button* button);
        void ClickModal(BSML::ModalColorPicker* picker, std::string_view text);
        custom_types::Helpers::Coroutine RunTests();
};
DEFINE_TYPE(BSML, ColorInteractionTests);

namespace BSML {
    void ColorInteractionTests::ctor() {
        immediateProbe = ColorInteractionProbe::New_ctor();
        deferredProbe = ColorInteractionProbe::New_ctor();
        standaloneProbe = ColorInteractionProbe::New_ctor();
        liteRowProbe = ColorInteractionProbe::New_ctor();
        liteModalProbe = ColorInteractionProbe::New_ctor();
        immediateValue = {0.2f, 0.7f, 1, 1};
        deferredValue = immediateValue;
        standaloneValue = immediateValue;
    }
    UnityEngine::Color ColorInteractionTests::get_immediateColor() { return immediateValue; }
    void ColorInteractionTests::set_immediateColor(UnityEngine::Color c) { immediateValue = c; ++immediateWrites; }
    UnityEngine::Color ColorInteractionTests::get_deferredColor() { return deferredValue; }
    void ColorInteractionTests::set_deferredColor(UnityEngine::Color c) { deferredValue = c; ++deferredWrites; }
    UnityEngine::Color ColorInteractionTests::get_standaloneColor() { return standaloneValue; }
    void ColorInteractionTests::set_standaloneColor(UnityEngine::Color c) { standaloneValue = c; ++standaloneWrites; }
    void ColorInteractionTests::ImmediateChanged(UnityEngine::Color c) { immediateProbe->Changed(c); }
    void ColorInteractionTests::DeferredChanged(UnityEngine::Color c) { deferredProbe->Changed(c); }
    void ColorInteractionTests::OpenStandalone() { if (!running) standalone->modalView->Show(); }
    void ColorInteractionTests::OpenLiteModal() { if (!running) liteModal->modalView->Show(); }
    void ColorInteractionTests::ToggleLite() { if (!running) liteRow->set_interactable(!liteRow->get_interactable()); }

    void ColorInteractionTests::PostParse() {
        deferredPicker = deferred->modalColorPicker;
        // This fixture has an authored position; keep the scroll layout from
        // repositioning it when it first becomes active during Show().
        auto layout = uncentered->GetComponent<UnityEngine::UI::LayoutElement*>();
        if (!layout) layout = uncentered->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
        layout->set_ignoreLayout(true);
        auto content = testScroll->_contentRectTransform;
        content->GetComponent<UnityEngine::UI::ContentSizeFitter*>()->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained);
        content->GetComponent<UnityEngine::UI::VerticalLayoutGroup*>()->set_childForceExpandWidth(true);
        auto size = content->get_sizeDelta(); size.x = 0; content->set_sizeDelta(size);
        liteRow = Lite::CreateColorPicker(liteContainer, "Lite color row", immediateValue,
            [this](auto c) { liteRowProbe->Done(c); }, [this] { liteRowProbe->Cancelled(); },
            [this](auto c) { liteRowProbe->Changed(c); });
        liteRowProbe->setting = liteRow;
        liteModal = Lite::CreateColorPickerModal(liteContainer, "Lite modal", immediateValue,
            [this](auto c) { liteModalProbe->Done(c); }, [this] { liteModalProbe->Cancelled(); },
            [this](auto c) { liteModalProbe->Changed(c); });
        liteNoCallbacks = Lite::CreateColorPicker(liteContainer, "Lite row: no callbacks", immediateValue);
        modalNoCallbacks = Lite::CreateColorPickerModal(liteContainer, "Lite modal: no callbacks", immediateValue);
        // Test all three actions against a receiver other than this parser host.
        // The temporary action map is destroyed before any interaction runs.
        standaloneProbe->binding = standalone->genericSetting;
        BSMLParserParams params{};
        params.AddAction("foreignChange", BSMLAction::MakeAction(standaloneProbe, "Changed", 1));
        params.AddAction("foreignDone", BSMLAction::MakeAction(standaloneProbe, "Done", 1));
        params.AddAction("foreignCancel", BSMLAction::MakeAction(standaloneProbe, "Cancelled", 0));
        ComponentTypeWithData component{};
        component.component = standalone;
        component.data = {{"onChange", "foreignChange"}, {"onDone", "foreignDone"}, {"onCancel", "foreignCancel"}};
        ModalColorPickerHandler handler;
        handler.HandleTypeAfterParse(component, params);
        INFO("COLOR-TEST ready: markup and Lite fixtures constructed");
        Run();
    }

    void ColorInteractionTests::Check(std::string_view name, bool ok) {
        if (ok) ++passed; else ++failed;
        INFO("COLOR-TEST {}: {}", ok ? "PASS" : "FAIL", name);
        results->set_text(fmt::format("{} passed / {} failed\n{}: {}", passed, failed, ok ? "PASS" : "FAIL", name));
    }
    void ColorInteractionTests::Click(UnityEngine::UI::Button* button) {
        auto event = UnityEngine::EventSystems::PointerEventData::New_ctor(UnityEngine::EventSystems::EventSystem::get_current());
        event->set_button(UnityEngine::EventSystems::PointerEventData::InputButton::Left);
        button->OnPointerClick(event);
    }
    void ColorInteractionTests::ClickModal(ModalColorPicker* picker, std::string_view text) {
        for (auto button : picker->GetComponentsInChildren<UnityEngine::UI::Button*>(true)) {
            auto label = button->GetComponentInChildren<TMPro::TextMeshProUGUI*>(true);
            if (label && std::string(label->get_text()) == text) { Click(button); return; }
        }
        Check(std::string("Missing modal button: ") + std::string(text), false);
        picker->modalView->Hide();
    }
    void ColorInteractionTests::Run() {
        if (running) return;
        running = true;
        SharedCoroutineStarter::StartCoroutine(RunTests());
    }

    custom_types::Helpers::Coroutine ColorInteractionTests::RunTests() {
        while (!results->get_gameObject()->get_activeInHierarchy()) co_yield nullptr;
        auto pause = [] { return reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSecondsRealtime::New_ctor(0.4f)); };
        co_yield pause();
        passed = 0; failed = 0;
        INFO("COLOR-TEST BEGIN");
        const UnityEngine::Color initial{0.2f, 0.7f, 1, 1};
        immediateValue = initial; deferredValue = initial; standaloneValue = initial;
        immediateWrites = 0; deferredWrites = 0; standaloneWrites = 0;
        immediate->ReceiveValue(); deferred->ReceiveValue(); liteRow->set_currentColor(initial);
        liteModal->set_currentColor(initial);
        for (auto p : {immediateProbe, deferredProbe, standaloneProbe, liteRowProbe, liteModalProbe}) {
            p->changes = 0; p->dones = 0; p->cancels = 0; p->changesAtCancel = 0;
            p->saved = initial; p->preview = initial; p->committedBeforeDone = false;
        }
        Check("setting ID binding exposes owned picker", deferred && deferred->modalColorPicker == deferredPicker);
        Check("nested picker is not registered externally", !deferred->GetComponent<ExternalComponents*>()->Get<ModalColorPicker*>());
        auto binding = deferredPicker->genericSetting;
        Check("nested picker has no value binding", binding && !binding->valueInfo && !binding->getterInfo && !binding->setterInfo);
        struct Case {
            const char* name; ModalColorPicker* picker; ColorSetting* setting; ColorInteractionProbe* probe;
            bool defer; bool bound; std::function<UnityEngine::Color()> saved; std::function<int()> writes;
        };
        std::vector<Case> cases{
            {"BSML immediate", immediate->modalColorPicker, immediate, immediateProbe, false, false, [this]{return immediateValue;}, [this]{return immediateWrites;}},
            {"BSML deferred", deferredPicker, deferred, deferredProbe, true, false, [this]{return deferredValue;}, [this]{return deferredWrites;}},
            {"BSML standalone / foreign actions", standalone, nullptr, standaloneProbe, false, true, [this]{return standaloneValue;}, [this]{return standaloneWrites;}},
            {"Lite row", liteRow->modalColorPicker, liteRow, liteRowProbe, false, false, [this]{return liteRowProbe->saved;}, [this]{return liteRowProbe->dones;}},
            {"Lite standalone", liteModal, nullptr, liteModalProbe, false, false, [this]{return liteModalProbe->saved;}, [this]{return liteModalProbe->dones;}}
        };
        for (auto c : cases) {
            auto check = [&](const char* step, bool ok) { Check(std::string(c.name) + ": " + step, ok); };
            auto open = [&] { if (c.setting) Click(c.setting->editButton); else c.picker->modalView->Show(); };
            auto visible = [&] { return c.picker->get_gameObject()->get_activeInHierarchy(); };
            auto synced = [&](UnityEngine::Color expected) {
                return SameColor(c.picker->get_currentColor(), expected) && SameColor(c.picker->rgbPanel->get_color(), expected)
                    && SameColor(c.picker->hsvPanel->get_color(), expected) && SameColor(c.picker->colorImage->get_color(), expected);
            };
            if (c.setting) c.setting->set_interactable(true);
            open(); co_yield pause();
            check("opens with initial color", visible() && synced(initial));
            auto rgb = c.picker->rgbPanel;
            auto hsv = c.picker->hsvPanel;
            using Event = GlobalNamespace::ColorChangeUIEventType;
            rgb->HandleSliderColorDidChange(rgb->_redSlider, {0.8f, 0.7f, 1, 1}, Event::Drag);
            rgb->HandleSliderColorDidChange(rgb->_greenSlider, {0.8f, 0.3f, 1, 1}, Event::Drag);
            rgb->HandleSliderColorDidChange(rgb->_blueSlider, {0.8f, 0.3f, 0.4f, 1}, Event::PointerUp);
            check("RGB drag/release synchronizes preview once per event", synced({0.8f,0.3f,0.4f,1}) && c.probe->changes == 3 && SameColor(c.probe->preview, c.picker->get_currentColor()));
            hsv->HandleColorSaturationOrValueDidChange(hsv->_colorSaturationValueSlider, {0.7f,0.8f}, Event::Drag);
            hsv->HandleColorHueDidChange(hsv->_colorHueSlider, 0.6f, Event::PointerUp);
            check("HSV synchronizes RGB and preview", synced(UnityEngine::Color::HSVToRGB(0.6f,0.7f,0.8f)) && c.probe->changes == 5);
            for (float value : {0.0f, 0.5f}) {
                hsv->HandleColorSaturationOrValueDidChange(hsv->_colorSaturationValueSlider, {0,value}, Event::Drag);
                hsv->_colorHueSlider->set_normalizedValue(0.85f);
                hsv->_colorHueSlider->HandleNormalizedValueDidChange(hsv->_colorHueSlider, 0.85f);
                check(value == 0 ? "hue remains movable at black" : "hue remains movable at gray",
                    std::abs(hsv->_colorHueSlider->get_normalizedValue()-0.85f) < 0.001f && synced({value,value,value,1}));
            }
            check("preview does not save", SameColor(c.saved(), initial) && c.writes() == 0);
            auto discarded = c.picker->get_currentColor();
            int changes = c.probe->changes;
            ClickModal(c.picker, "Cancel"); co_yield pause();
            check("Cancel closes without saving", !visible() && c.writes() == 0 && SameColor(c.saved(), initial));
            if (c.setting) {
                check("Cancel restores previous preview once", SameColor(c.probe->preview, initial) && c.probe->changes == changes+1 && SameColor(c.setting->get_currentColor(), initial));
                if (c.setting == liteRow) check("Lite cancel follows restored preview", c.probe->cancels == 1 && c.probe->changesAtCancel == changes+1);
            } else check("Cancel action called once", c.probe->cancels == 1);
            open(); co_yield pause();
            check("reopen uses owner value or unbound draft", visible() && synced(c.setting || c.bound ? initial : discarded));
            const UnityEngine::Color selected{0.9f,0.2f,0.35f,1};
            rgb->HandleSliderColorDidChange(rgb->_redSlider, selected, Event::PointerUp);
            changes = c.probe->changes;
            ClickModal(c.picker, "OK"); co_yield pause();
            check("OK closes without another preview callback", !visible() && c.probe->changes == changes);
            check("OK writes exactly when requested", c.writes() == (c.defer ? 0 : 1) && SameColor(c.saved(), c.defer ? initial : selected));
            if (c.setting) check("OK updates setting swatch", SameColor(c.setting->get_currentColor(), selected) && SameColor(c.setting->colorImage->get_color(), selected));
            if (!c.setting || c.setting == liteRow) check("Done callback sees committed state", c.probe->dones == 1 && c.probe->committedBeforeDone);
            open(); co_yield pause();
            check("reopen preserves accepted draft", synced(selected));
            ClickModal(c.picker, "Cancel"); co_yield pause();
            if (c.defer) { Click(applyDeferred); check("Apply action saves deferred draft once", c.writes() == 1 && SameColor(c.saved(), selected)); }
            if (c.setting) {
                c.setting->set_interactable(false); Click(c.setting->editButton); co_yield pause();
                check("disabled edit button rejects click", !visible());
                c.setting->set_interactable(true);
                c.setting->get_gameObject()->SetActive(false); c.setting->get_gameObject()->SetActive(true);
                c.setting->Setup(); c.setting->Setup();
                open(); co_yield pause();
                check("re-enabled/repeated Setup still opens", visible());
                changes = c.probe->changes;
                ClickModal(c.picker, "Cancel"); co_yield pause();
                check("repeated Setup does not duplicate callbacks", c.probe->changes == changes+1);
            }
        }
        for (auto picker : {liteNoCallbacks->modalColorPicker, modalNoCallbacks}) {
            picker->modalView->Show(); co_yield pause();
            picker->rgbPanel->HandleSliderColorDidChange(picker->rgbPanel->_redSlider, {0.1f,0.2f,0.3f,1}, GlobalNamespace::ColorChangeUIEventType::Drag);
            ClickModal(picker, "OK"); co_yield pause();
            Check("Lite optional callbacks: change and OK", !picker->get_gameObject()->get_activeInHierarchy());
            picker->modalView->Show(); co_yield pause();
            ClickModal(picker, "Cancel"); co_yield pause();
            Check("Lite optional callbacks: Cancel", !picker->get_gameObject()->get_activeInHierarchy());
        }
        standalone->modalView->dismissOnBlockerClicked = true;
        standalone->modalView->Show(); co_yield pause();
        int cancels = standaloneProbe->cancels;
        int writes = standaloneWrites;
        standalone->modalView->BlockerClicked(); co_yield pause();
        Check("click-off closes without implicit OK/Cancel", !standalone->get_gameObject()->get_activeInHierarchy() && standaloneWrites == writes && standaloneProbe->cancels == cancels);
        Check("explicit move-to-center=false overrides factory default", !uncentered->modalView->moveToCenter);
        for (auto c : cases) {
            auto check = [&](const char* step, bool ok) { Check(std::string(c.name) + ": " + step, ok); };
            auto open = [&] { if (c.setting) Click(c.setting->editButton); else c.picker->modalView->Show(); };
            auto transform = c.picker->get_transform();
            auto originalParent = transform->get_parent();
            auto viewport = testScroll->get_viewportTransform();
            check("centers by default", c.picker->modalView->moveToCenter);
            testScroll->ScrollTo(0, false); co_yield pause();
            auto contentAtTop = testScroll->_contentRectTransform->get_anchoredPosition();
            open(); co_yield pause();
            auto atTop = viewport->InverseTransformPoint(transform->get_position());
            ClickModal(c.picker, "Cancel"); co_yield pause();
            testScroll->ScrollToEnd(false); co_yield pause();
            auto contentAtBottom = testScroll->_contentRectTransform->get_anchoredPosition();
            check("fixture scrolls between top and bottom", std::abs(contentAtBottom.y-contentAtTop.y) > 1.0f);
            open(); co_yield pause();
            auto atBottom = viewport->InverseTransformPoint(transform->get_position());
            check("modal position is independent of scroll", std::abs(atTop.x-atBottom.x) < 0.01f
                && std::abs(atTop.y-atBottom.y) < 0.01f && std::abs(atTop.z-atBottom.z) < 0.01f);
            INFO("COLOR-TEST POSITION {}: top=({},{},{}) bottom=({},{},{})", c.name,
                atTop.x, atTop.y, atTop.z, atBottom.x, atBottom.y, atBottom.z);
            ClickModal(c.picker, "Cancel"); co_yield pause();
            check("closing restores original parent", transform->get_parent() == originalParent);
        }
        // Opting out must preserve the authored position when HMUI reparents the
        // modal for display, including after its scroll content has moved.
        auto uncenteredTransform = uncentered->get_transform();
        auto uncenteredParent = uncenteredTransform->get_parent();
        auto viewport = testScroll->get_viewportTransform();
        auto position = [&] { return viewport->InverseTransformPoint(uncenteredTransform->get_position()); };
        auto sameXY = [](auto a, auto b) { return std::abs(a.x-b.x) < 0.01f && std::abs(a.y-b.y) < 0.01f; };
        testScroll->ScrollTo(0, false); co_yield pause();
        auto expectedTop = position();
        uncentered->modalView->Show(); co_yield pause();
        auto actualTop = position();
        Check("centering opt-out opens at authored top position", uncentered->get_gameObject()->get_activeInHierarchy() && sameXY(actualTop, expectedTop));
        ClickModal(uncentered, "Cancel"); co_yield pause();
        Check("centering opt-out closes and restores parent at top", !uncentered->get_gameObject()->get_activeInHierarchy() && uncenteredTransform->get_parent() == uncenteredParent);
        testScroll->ScrollToEnd(false); co_yield pause();
        auto expectedBottom = position();
        Check("centering opt-out fixture moves with scroll", std::abs(expectedBottom.y-expectedTop.y) > 1.0f);
        uncentered->modalView->Show(); co_yield pause();
        auto actualBottom = position();
        Check("centering opt-out opens at authored bottom position", uncentered->get_gameObject()->get_activeInHierarchy() && sameXY(actualBottom, expectedBottom));
        Check("centering opt-out displayed position follows scroll", std::abs(actualBottom.y-actualTop.y) > 1.0f
            && std::abs((actualBottom.y-actualTop.y)-(expectedBottom.y-expectedTop.y)) < 0.01f);
        INFO("COLOR-TEST OPT-OUT POSITION: expected top=({},{}) actual top=({},{}) expected bottom=({},{}) actual bottom=({},{})",
            expectedTop.x, expectedTop.y, actualTop.x, actualTop.y, expectedBottom.x, expectedBottom.y, actualBottom.x, actualBottom.y);
        ClickModal(uncentered, "Cancel"); co_yield pause();
        Check("centering opt-out closes and restores parent at bottom", !uncentered->get_gameObject()->get_activeInHierarchy() && uncenteredTransform->get_parent() == uncenteredParent);
        testScroll->ScrollTo(0, false);
        running = false;
        results->set_text(fmt::format("Finished: {} passed / {} failed\nNow test controller dragging and placement manually. Details in logcat.", passed, failed));
        INFO("COLOR-TEST COMPLETE: {} passed, {} failed", passed, failed);
    }

    void RegisterColorInteractionTests() {
        Register::RegisterSettingsMenu("Color Interaction Test", MOD_ID "_color_interaction_test", ColorInteractionTests::New_ctor(), false);
    }
}

BSML_DATACACHE(color_interaction_test) {
    static constexpr uint8_t content[] = {
        #embed "../../assets/Settings/ColorInteractionTest.bsml"
    };
    return ArrayW<uint8_t>(std::span(content));
}
