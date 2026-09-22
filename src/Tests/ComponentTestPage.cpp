#include "BSML.hpp"
#include "BSMLDataCache.hpp"
#include "BSML-Lite/Creation/Buttons.hpp"
#include "BSML-Lite/Creation/Layout.hpp"
#include "BSML-Lite/Creation/Text.hpp"
#include "BSML/Settings/BSMLSettings.hpp"
#include "BSML/Components/CustomListTableData.hpp"
#include "BSML/Components/ButtonIconImage.hpp"
#include "BSML/Components/Keyboard/ModalKeyboard.hpp"
#include "BSML/Components/ModalColorPicker.hpp"
#include "BSML/Components/ExternalComponents.hpp"
#include "BSML/Components/Settings/ColorSetting.hpp"
#include "BSML/Components/Settings/ToggleSetting.hpp"
#include "BSML/GameplaySetup/GameplaySetup.hpp"
#include "BSML/ViewControllers/BSMLViewController.hpp"
#include "BSML/MainThreadScheduler.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/UI/Button.hpp"
#include <fstream>
#include "Helpers/getters.hpp"
#include "GlobalNamespace/UIKeyboardManager.hpp"
#include "GlobalNamespace/OptionsViewController.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "HMUI/NoTransitionsButton.hpp"
#include "HMUI/ImageView.hpp"
#include <cmath>
#include "Helpers/utilities.hpp"
#include "assets.hpp"
#include "HMUI/IconSegmentedControl.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "HMUI/ScrollView.hpp"
#include "logging.hpp"

// Submenu lists are constructed while hidden. Refresh after their visible layout is ready.
DECLARE_CLASS_CODEGEN(BSML, ComponentTestListLayout, UnityEngine::MonoBehaviour) {
    DECLARE_INSTANCE_FIELD(BSML::CustomListTableData*, data);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, status);
    DECLARE_INSTANCE_FIELD(bool, refreshPending);
    DECLARE_INSTANCE_METHOD(void, OnEnable);
    DECLARE_INSTANCE_METHOD(void, LateUpdate);
};

DEFINE_TYPE(BSML, ComponentTestListLayout);

void BSML::ComponentTestListLayout::OnEnable() {
    refreshPending = true;
}

void BSML::ComponentTestListLayout::LateUpdate() {
    if (!refreshPending || !data || !status) return;
    refreshPending = false;
    UnityEngine::Canvas::ForceUpdateCanvases();
    auto table = data->tableView;
    table->ReloadData();
    auto rect = table->_scrollView->_viewport->get_rect();
    auto message = fmt::format("{} rows | viewport {:.0f} x {:.0f} | {} active cells",
        data->data.size(), rect.get_width(), rect.get_height(), table->GetComponentsInChildren<HMUI::TableCell*>().size());
    status->set_text(message);
    INFO("Component test list: {}", message);
}

// Local test host: no public API or saved configuration.
DECLARE_CLASS_CODEGEN(BSML, ComponentTestPage, System::Object) {
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, status);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, testScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, diScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, iconScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, buttonScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, eventScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, paddingScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, spacingScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, sizingScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, liteScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, segmentScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, settingsScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, colorScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, nameScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, anchorScroll);
    DECLARE_INSTANCE_FIELD(HMUI::ScrollView*, fallbackScroll);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, hotReloadStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, anchorStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, fallbackStatus);
    DECLARE_INSTANCE_FIELD(UnityEngine::Transform*, fallbackPreviewParent);
    DECLARE_INSTANCE_FIELD(BSML::BSMLViewController*, fallbackPreview);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, buttonStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, eventStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, sizingStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, segmentStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, settingsStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, colorValuesStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, nameStatus);
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, namedText);
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, namedButton);
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, namedLayout);
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, unnamedText);
    DECLARE_INSTANCE_FIELD(UnityEngine::Transform*, liteButtonTests);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, liteButtonStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, paddingParserStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, diStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, iconStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, iconStyleStatus);
    DECLARE_INSTANCE_FIELD(BSML::ButtonIconImage*, positiveSkewIcon);
    DECLARE_INSTANCE_FIELD(BSML::ButtonIconImage*, negativeSkewIcon);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Button*, enabledIcon);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Button*, disabledIcon);
    DECLARE_INSTANCE_FIELD(BSML::ModalKeyboard*, testKeyboard);
    DECLARE_INSTANCE_FIELD(BSML::ModalColorPicker*, testColors);
    DECLARE_INSTANCE_FIELD(BSML::ModalColorPicker*, boundColorPicker);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, colorBindingStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, colorStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, colorSettingStatus);
    DECLARE_INSTANCE_FIELD(BSML::ColorSetting*, immediateColorSetting);
    DECLARE_INSTANCE_FIELD(BSML::ColorSetting*, deferredColorSetting);
    DECLARE_INSTANCE_FIELD(UnityEngine::Color, immediateColorValue);
    DECLARE_INSTANCE_FIELD(UnityEngine::Color, deferredColorValue);
    DECLARE_INSTANCE_FIELD(UnityEngine::Color, immediatePreview);
    DECLARE_INSTANCE_FIELD(UnityEngine::Color, deferredPreview);
    DECLARE_INSTANCE_FIELD(int, immediateColorWrites);
    DECLARE_INSTANCE_FIELD(int, deferredColorWrites);
    DECLARE_INSTANCE_FIELD(int, immediateColorChanges);
    DECLARE_INSTANCE_FIELD(int, deferredColorChanges);
    DECLARE_INSTANCE_FIELD(BSML::ToggleSetting*, testModifier);
    DECLARE_INSTANCE_FIELD(BSML::MenuButton*, testMenuButton);
    DECLARE_INSTANCE_FIELD(int, iconClicks);
    DECLARE_INSTANCE_FIELD(int, menuClicks);
    DECLARE_INSTANCE_FIELD(int, gameplayActivations);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, modalStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, listStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, simpleStatus);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, boxStatus);
    DECLARE_INSTANCE_FIELD(BSML::CustomListTableData*, songList);
    DECLARE_INSTANCE_FIELD(BSML::CustomListTableData*, simpleList);
    DECLARE_INSTANCE_FIELD(BSML::CustomListTableData*, boxList);
    DECLARE_INSTANCE_FIELD(ListW<StringW>, labels);
    DECLARE_INSTANCE_FIELD(StringW, keyboardValue);
    DECLARE_INSTANCE_FIELD(UnityEngine::Color, colorValue);
    DECLARE_INSTANCE_FIELD(bool, modifierValue);
    DECLARE_INSTANCE_FIELD(bool, toggleValue);
    DECLARE_INSTANCE_FIELD(float, numberValue);
    DECLARE_INSTANCE_FIELD(int, clicks);
    DECLARE_SIMPLE_DTOR();
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, bindingResult);
    DECLARE_INSTANCE_METHOD(void, CheckSnapshotBindings);
    DECLARE_CTOR(ctor);
    DECLARE_INSTANCE_METHOD(ListW<HMUI::IconSegmentedControl::DataItem*>, get_icons);
    DECLARE_INSTANCE_METHOD(ListW<BSML::CustomCellInfo*>, get_rows);
    DECLARE_INSTANCE_METHOD(StringW, get_longText);
    DECLARE_INSTANCE_METHOD(void, Click);
    DECLARE_INSTANCE_METHOD(void, IconClicked);
    DECLARE_INSTANCE_METHOD(void, ToggleIcon);
    DECLARE_INSTANCE_METHOD(void, ReadIconState);
    DECLARE_INSTANCE_METHOD(void, CheckIconStyling);
    DECLARE_INSTANCE_METHOD(void, CheckDI);
    DECLARE_INSTANCE_METHOD(void, CheckNames);
    DECLARE_INSTANCE_METHOD(void, CheckAnchors);
    DECLARE_INSTANCE_METHOD(void, CheckViewControllerFallback);
    DECLARE_INSTANCE_METHOD(void, ShowFallbackError);
    DECLARE_INSTANCE_METHOD(void, ShowFallbackRecovery);
    DECLARE_INSTANCE_METHOD(void, CheckHotReload);
    DECLARE_INSTANCE_METHOD(void, ReadValues);
    DECLARE_INSTANCE_METHOD(void, KeyboardEntered, StringW value);
    DECLARE_INSTANCE_METHOD(void, ColorChanged, UnityEngine::Color value);
    DECLARE_INSTANCE_METHOD(void, ColorDone, UnityEngine::Color value);
    DECLARE_INSTANCE_METHOD(void, ColorCancelled);
    DECLARE_INSTANCE_METHOD(UnityEngine::Color, get_immediateColor);
    DECLARE_INSTANCE_METHOD(void, set_immediateColor, UnityEngine::Color value);
    DECLARE_INSTANCE_METHOD(UnityEngine::Color, get_deferredColor);
    DECLARE_INSTANCE_METHOD(void, set_deferredColor, UnityEngine::Color value);
    DECLARE_INSTANCE_METHOD(void, ImmediateColorChanged, UnityEngine::Color value);
    DECLARE_INSTANCE_METHOD(void, DeferredColorChanged, UnityEngine::Color value);
    DECLARE_INSTANCE_METHOD(void, ReadColorSettings);
    DECLARE_INSTANCE_METHOD(void, CheckColorBindings);
    DECLARE_INSTANCE_METHOD(void, SegmentSelected, HMUI::SegmentedControl* control, int index);
    DECLARE_INSTANCE_METHOD(void, ListSelected, HMUI::TableView* table, int index);
    DECLARE_INSTANCE_METHOD(void, Back);
    DECLARE_INSTANCE_METHOD(void, PostParse);

    public:
        void UpdateStatus(StringW message);
};

DEFINE_TYPE(BSML, ComponentTestPage);

namespace BSML {
    std::string RunPaddingParserTests();
    std::string RunRectTransformTests();
    std::string RunViewControllerTests();
    std::string RunHotReloadTests();
    std::string RunSnapshotBindingTests();
    BSMLViewController* UpdateViewControllerPreview(BSMLViewController* existing, UnityEngine::Transform* parent, bool invalid);
    void RegisterColorInteractionTests();
    void ComponentTestPage::ctor() {
        INVOKE_CTOR();
        labels = ListW<StringW>::New();
        labels->Add("One");
        labels->Add("Two");
        labels->Add("Three");
        keyboardValue = "Type here";
        colorValue = {0.2f, 0.7f, 1.0f, 1.0f};
        immediateColorValue = colorValue;
        deferredColorValue = colorValue;
        immediatePreview = colorValue;
        deferredPreview = colorValue;
        numberValue = 1.0f;
        modifierValue = false;
        toggleValue = false;
        clicks = 0;
    }

    ListW<HMUI::IconSegmentedControl::DataItem*> ComponentTestPage::get_icons() {
        // The leaderboard's data is null until it is initialized; supply our own test data.
        auto result = ListW<HMUI::IconSegmentedControl::DataItem*>::New();
        result->Add(HMUI::IconSegmentedControl::DataItem::New_ctor(
            Utilities::LoadSpriteRaw(ArrayW<uint8_t>(Assets::Images::ModsIdle)), "Test icon one", true));
        result->Add(HMUI::IconSegmentedControl::DataItem::New_ctor(
            Utilities::LoadSpriteRaw(ArrayW<uint8_t>(Assets::Images::ModsSelected)), "Test icon two", true));
        result->Add(HMUI::IconSegmentedControl::DataItem::New_ctor(
            Utilities::LoadSpriteRaw(ArrayW<uint8_t>(Assets::Images::Visibility)), "Test icon three", true));
        return result;
    }

    ListW<CustomCellInfo*> ComponentTestPage::get_rows() {
        auto icons = get_icons();
        auto result = ListW<CustomCellInfo*>::New();
        for (int i = 0; i < 18; ++i) {
            auto icon = icons.size() ? icons[i % icons.size()]->get_icon() : nullptr;
            result->Add(CustomCellInfo::construct(fmt::format("Row {:02}", i + 1), "Select me, then scroll", icon));
        }
        return result;
    }

    StringW ComponentTestPage::get_longText() {
        std::string text;
        for (int i = 1; i <= 30; ++i)
            text += fmt::format("Line {:02}: scroll down and back up.\n", i);
        return text;
    }

    void ComponentTestPage::UpdateStatus(StringW message) {
        for (auto label : {status, buttonStatus, eventStatus, sizingStatus, segmentStatus, settingsStatus, colorValuesStatus}) {
            if (label) label->set_text(message);
        }
    }

    void ComponentTestPage::CheckAnchors() {
        CheckNames();
        auto result = RunRectTransformTests();
        if (anchorStatus) anchorStatus->set_text(result);
    }

    void ComponentTestPage::CheckViewControllerFallback() {
        auto result = RunViewControllerTests();
        if (fallbackStatus) fallbackStatus->set_text(result);
    }

    void ComponentTestPage::ShowFallbackError() {
        if (!fallbackPreviewParent) return;
        fallbackPreview = UpdateViewControllerPreview(fallbackPreview, fallbackPreviewParent, true);
        if (fallbackStatus) fallbackStatus->set_text("Expected: Invalid BSML and a Vector2 error below. The partial content should be gone.");
    }

    void ComponentTestPage::ShowFallbackRecovery() {
        if (!fallbackPreviewParent) return;
        fallbackPreview = UpdateViewControllerPreview(fallbackPreview, fallbackPreviewParent, false);
        if (fallbackStatus) fallbackStatus->set_text("Expected: Recovered below, with the error page replaced. You can repeat both buttons.");
    }

    void ComponentTestPage::CheckSnapshotBindings() {
        if (bindingResult) bindingResult->set_text(RunSnapshotBindingTests());
    }

    void ComponentTestPage::CheckHotReload() {
        if (hotReloadStatus) hotReloadStatus->set_text(RunHotReloadTests());
    }

    void ComponentTestPage::CheckNames() {
        int checks = 0;
        int passed = 0;
        std::string details;
        auto record = [&](const char* label, bool matches, const std::string& actual) {
            ++checks;
            if (matches) ++passed;
            details += fmt::format("{}: {} ({})\n", matches ? "PASS" : "FAIL", label, actual);
        };
        auto check = [&](const char* label, UnityEngine::GameObject* object, const char* expected) {
            record(label, object != nullptr, "id binding");
            std::string actual = object ? std::string(object->get_name()) : "<unbound>";
            record(expected, object && actual == expected, actual);
        };
        check("namedText", namedText, "StatusLabel");
        check("namedButton", namedButton, "RunNameChecks");
        check("namedLayout", namedLayout, "NameTestGroup");
        check("unnamedText", unnamedText, "BSMLText");
        auto text = namedText ? namedText->GetComponent<TMPro::TextMeshProUGUI*>() : nullptr;
        record("Visible label preserved", text && std::string(text->get_text()) == "Named text sample", "text attribute");
        auto report = fmt::format("Name checks: {}/{} passed\n{}", passed, checks, details);
        if (nameStatus) nameStatus->set_text(report);
        INFO("Component test object names: {}", report);
    }

    void ComponentTestPage::Click() {
        ++clicks;
        UpdateStatus(fmt::format("Button clicks: {}", clicks));
        modalStatus->set_text(fmt::format("Raycaster clicks: {}", clicks));
        INFO("Component test: button click {}", clicks);
    }

    void ComponentTestPage::IconClicked() {
        ++iconClicks;
        ReadIconState();
    }

    void ComponentTestPage::ToggleIcon() {
        if (!enabledIcon) return;
        enabledIcon->set_interactable(!enabledIcon->get_interactable());
        iconStatus->set_text("Left icon toggled. Move pointer away, then Read icon state.");
    }

    void ComponentTestPage::ReadIconState() {
        std::string report = fmt::format("Icon clicks: {}", iconClicks);
        auto read = [&](const char* label, UnityEngine::UI::Button* button) {
            auto icon = button ? button->GetComponent<ButtonIconImage*>() : nullptr;
            if (!icon || !icon->image) {
                report += fmt::format("\nFAIL: {} icon was not bound", label);
                return;
            }
            bool enabled = button->get_interactable();
            bool noTransitions = button->GetComponent<HMUI::NoTransitionsButton*>() != nullptr;
            report += fmt::format("\n{}: enabled={}, NoTransitions={}\nalpha={:.2f}; PC expected={:.2f}",
                label, enabled, noTransitions, icon->image->get_color().a, enabled ? 1.0f : 0.25f);
        };
        read("Left", enabledIcon);
        read("Right", disabledIcon);
        iconStatus->set_text(report);
        INFO("Component test icon state: {}", report);
    }

    void ComponentTestPage::CheckIconStyling() {
        std::string report;
        auto check = [&](const char* name, ButtonIconImage* icon, float expectedSkew, bool expectedUnderline) {
            auto imageView = icon ? UnityW<UnityEngine::UI::Image>(icon->image).try_cast<HMUI::ImageView>() : nullptr;
            if (!imageView || !icon->underline) {
                report += fmt::format("FAIL: {} missing image/underline\n", name);
                return;
            }
            float skew = imageView->get_skew();
            bool underlineActive = icon->underline->get_activeSelf();
            bool matches = std::abs(skew - expectedSkew) < 0.001f && underlineActive == expectedUnderline;
            report += fmt::format("{}: {}\n", matches ? "PASS" : "FAIL", name);
            INFO("Component test icon styling: {} skew={:.2f} expected={:.2f}, underline={} expected={}",
                name, skew, expectedSkew, underlineActive, expectedUnderline);
        };
        auto defaultIcon = enabledIcon ? enabledIcon->GetComponent<ButtonIconImage*>() : nullptr;
        auto background = enabledIcon ? enabledIcon->get_transform()->Find("BG") : nullptr;
        auto backgroundImage = background ? background->GetComponent<HMUI::ImageView*>() : nullptr;
        if (backgroundImage) check("Background skew", defaultIcon, backgroundImage->get_skew(), true);
        else report += "FAIL: missing background\n";
        check("Zero skew, hidden underline", disabledIcon ? disabledIcon->GetComponent<ButtonIconImage*>() : nullptr, 0.0f, false);
        check("Positive skew, shown underline", positiveSkewIcon, 0.5f, true);
        check("Negative skew, hidden underline", negativeSkewIcon, -0.5f, false);
        iconStyleStatus->set_text(report);
        INFO("Component test icon styling results: {}", report);
    }

    void ComponentTestPage::CheckDI() {
        std::string report;
        std::string failures;
        int passedChecks = 0;
        int failedChecks = 0;
        auto check = [&](const char* label, bool passed) {
            report += fmt::format("{}: {}\n", passed ? "PASS" : "FAIL", label);
            if (passed) ++passedChecks;
            else {
                ++failedChecks;
                failures += fmt::format("\nFAIL: {}", label);
            }
        };
        auto container = Helpers::GetDiContainer();
        check("Menu DI container", container != nullptr);
        if (container) {
            auto gameplay = container->TryResolve<GlobalNamespace::GameplaySetupViewController*>();
            check("Gameplay controller bound and used", gameplay && GameplaySetup::get_instance()->gameplaySetupViewController == gameplay);
            check("Settings controller and button", container->TryResolve<GlobalNamespace::OptionsViewController*>() && BSMLSettings::get_instance()->button);
            check("Main menu controller and test button", container->TryResolve<GlobalNamespace::MainMenuViewController*>() && testMenuButton && testMenuButton->menuButton);
            check("Modifier toggle constructed", testModifier && testModifier->toggle);
            check("RGB, HSV and color image constructed", testColors && testColors->rgbPanel && testColors->hsvPanel && testColors->colorImage);

            auto manager = container->TryResolve<GlobalNamespace::UIKeyboardManager*>();
            auto qTransform = manager ? manager->get_transform()->Find("KeyboardWrapper/Keyboard/Letters/Row/Q") : nullptr;
            auto q = qTransform ? qTransform->GetComponent<UnityEngine::UI::Button*>() : nullptr;
            auto keyboard = testKeyboard ? testKeyboard->keyboard : nullptr;
            check("Keyboard and PC Q template available", keyboard && q);
            if (keyboard && q) {
                auto savedTemplate = keyboard->baseButton;
                keyboard->SetButtonType("Q");
                check("Keyboard Q selection", keyboard->baseButton == q);
                keyboard->SetButtonType("BSML-ComponentTest-Missing-Key");
                check("Keyboard missing-name fallback", keyboard->baseButton == q);
                auto alternate = manager->GetComponentsInChildren<UnityEngine::UI::Button*>(true).front_or_default([](auto button) {
                    return button->get_name() != "Q";
                });
                if (alternate) {
                    auto name = std::string(alternate->get_name());
                    keyboard->SetButtonType(name);
                    check("Keyboard named template selection", keyboard->baseButton == alternate);
                    INFO("Component test keyboard alternate template: {}", name);
                } else {
                    report += "SKIP: No alternate keyboard template found\n";
                }
                keyboard->baseButton = savedTemplate;
            }
        }
        report += fmt::format("Menu clicks: {} | Gameplay activations: {}\nRepeat clicks, modal edits and reopening manually.", menuClicks, gameplayActivations);
        diStatus->set_text(fmt::format("DI: {} passed, {} failed\nMenu clicks: {}; gameplay visits: {}\nDetails in log.{}",
            passedChecks, failedChecks, menuClicks, gameplayActivations, failures));
        INFO("Component test DI: {}", report);
    }

    void ComponentTestPage::ReadValues() {
        auto report = fmt::format("Text: {} | Toggle: {} | Modifier: {}\nNumber: {:.1f} | RGB: {:.2f}, {:.2f}, {:.2f}",
            std::string(keyboardValue), toggleValue, modifierValue, numberValue, colorValue.r, colorValue.g, colorValue.b);
        UpdateStatus(report);
        INFO("Component test stored values: {}", report);
    }

    void ComponentTestPage::KeyboardEntered(StringW value) {
        UpdateStatus("Keyboard: " + std::string(value));
        INFO("Component test keyboard submitted: {}", std::string(value));
    }

    void ComponentTestPage::ColorChanged(UnityEngine::Color value) {
        auto message = fmt::format("Preview RGB: {:.2f}, {:.2f}, {:.2f}\nStored RGB: {:.2f}, {:.2f}, {:.2f}",
            value.r, value.g, value.b, colorValue.r, colorValue.g, colorValue.b);
        colorStatus->set_text(message);
        INFO("Component test color-change: {}", message);
    }

    void ComponentTestPage::ColorDone(UnityEngine::Color value) {
        auto matches = [&](UnityEngine::Color other) {
            return std::abs(value.r - other.r) < 0.001f && std::abs(value.g - other.g) < 0.001f
                && std::abs(value.b - other.b) < 0.001f && std::abs(value.a - other.a) < 0.001f;
        };
        auto message = fmt::format("{}: OK callback sees committed picker color\nRGB: {:.2f}, {:.2f}, {:.2f}",
            matches(colorValue) && matches(testColors->get_currentColor()) ? "PASS" : "FAIL", value.r, value.g, value.b);
        colorStatus->set_text(message);
        INFO("Component test on-done: {}", message);
    }

    void ComponentTestPage::ColorCancelled() {
        auto message = fmt::format("Cancel callback. Stored RGB: {:.2f}, {:.2f}, {:.2f}\nReopen to check the saved color is restored.",
            colorValue.r, colorValue.g, colorValue.b);
        colorStatus->set_text(message);
        INFO("Component test on-cancel: {}", message);
    }

    UnityEngine::Color ComponentTestPage::get_immediateColor() { return immediateColorValue; }

    void ComponentTestPage::set_immediateColor(UnityEngine::Color value) {
        immediateColorValue = value;
        ++immediateColorWrites;
    }

    UnityEngine::Color ComponentTestPage::get_deferredColor() { return deferredColorValue; }

    void ComponentTestPage::set_deferredColor(UnityEngine::Color value) {
        deferredColorValue = value;
        ++deferredColorWrites;
    }

    void ComponentTestPage::ImmediateColorChanged(UnityEngine::Color value) {
        immediatePreview = value;
        ++immediateColorChanges;
        ReadColorSettings();
    }

    void ComponentTestPage::DeferredColorChanged(UnityEngine::Color value) {
        deferredPreview = value;
        ++deferredColorChanges;
        ReadColorSettings();
    }

    void ComponentTestPage::ReadColorSettings() {
        auto row = [](const char* name, UnityEngine::Color saved, UnityEngine::Color draft,
                      UnityEngine::Color preview, int writes, int changes) {
            return fmt::format("{}: {} writes, {} previews\nSaved {:.2f}/{:.2f}/{:.2f}; draft {:.2f}/{:.2f}/{:.2f}\nPreview {:.2f}/{:.2f}/{:.2f}",
                name, writes, changes, saved.r, saved.g, saved.b, draft.r, draft.g, draft.b, preview.r, preview.g, preview.b);
        };
        if (!colorSettingStatus || !immediateColorSetting || !deferredColorSetting) return;
        auto report = row("Immediate", immediateColorValue, immediateColorSetting->get_currentColor(),
                          immediatePreview, immediateColorWrites, immediateColorChanges)
            + "\n" + row("Deferred", deferredColorValue, deferredColorSetting->get_currentColor(),
                          deferredPreview, deferredColorWrites, deferredColorChanges);
        colorSettingStatus->set_text(report);
        INFO("Component test color settings: {}", report);
    }

    void ComponentTestPage::CheckColorBindings() {
        std::string report;
        auto check = [&](const char* label, bool passed) {
            report += fmt::format("{}: {}\n", passed ? "PASS" : "FAIL", label);
        };
        check("ColorSetting ID", immediateColorSetting != nullptr);
        check("ModalColorPicker ID", boundColorPicker != nullptr);
        auto external = deferredColorSetting ? deferredColorSetting->GetComponent<ExternalComponents*>() : nullptr;
        check("External picker lookup", boundColorPicker && external && external->Get<ModalColorPicker*>() == boundColorPicker);
        auto binding = boundColorPicker ? boundColorPicker->genericSetting : nullptr;
        check("Nested value remains unbound", binding && !binding->valueInfo && !binding->getterInfo && !binding->setterInfo);
        auto standaloneBinding = testColors ? testColors->genericSetting : nullptr;
        check("Standalone value remains bound", standaloneBinding && standaloneBinding->valueInfo);
        colorBindingStatus->set_text(report);
        INFO("Component test color bindings: {}", report);
    }

    void ComponentTestPage::SegmentSelected(HMUI::SegmentedControl* control, int index) {
        UpdateStatus(fmt::format("{}: selected {}", std::string(control->get_name()), index + 1));
    }

    void ComponentTestPage::ListSelected(HMUI::TableView* table, int index) {
        auto message = fmt::format("Selected row {}", index + 1);
        for (auto label : {listStatus, simpleStatus, boxStatus}) {
            if (label && label->m_CachedPtr.m_value) label->set_text(message);
        }
        INFO("Component test: {} selected row {}", std::string(table->get_name()), index + 1);
    }

    void ComponentTestPage::Back() {
        BSMLSettings::get_instance()->get_modSettingsFlowCoordinator()->Back();
    }

    void ComponentTestPage::PostParse() {
        CheckNames();
        if (paddingParserStatus) paddingParserStatus->set_text(RunPaddingParserTests());
        if (liteButtonTests && liteButtonTests->get_childCount() == 0) {
            struct Variant { const char* label; const char* buttonTemplate; };
            const Variant variants[] = {
                {"No template overload", nullptr},
                {"PracticeButton template", DEFAULT_BUTTONTEMPLATE},
                {"PlayButton template", "PlayButton"},
            };
            const UnityEngine::Vector2 sizes[] = {{24, 10}, {48, 10}, {24, 18}, {0, 0}};
            int index = 0;
            for (const auto& variant : variants) {
                for (const auto& size : sizes) {
                    auto expected = size.x == 0 ? std::string("automatic size") : fmt::format("{} x {}", size.x, size.y);
                    auto name = fmt::format("L{} - {}: {}", ++index, variant.label, expected);
                    auto label = Lite::CreateText(liteButtonTests, name, TMPro::FontStyles::Normal, 3);
                    label->set_enableWordWrapping(true);

                    // Let each button report its own preferred size, without stretching it.
                    auto row = Lite::CreateHorizontalLayoutGroup(liteButtonTests);
                    row->set_childControlWidth(true);
                    row->set_childControlHeight(true);
                    row->set_childForceExpandWidth(false);
                    row->set_childForceExpandHeight(false);
                    auto clicked = [this, row, name, size] {
                        UnityEngine::Canvas::ForceUpdateCanvases();
                        auto button = row->GetComponentInChildren<UnityEngine::UI::Button*>();
                        auto rect = button->transform.cast<UnityEngine::RectTransform>()->get_rect();
                        bool matches = size.x == 0 || (std::abs(rect.get_width() - size.x) < 0.1f && std::abs(rect.get_height() - size.y) < 0.1f);
                        auto report = fmt::format("{}\nActual: {:.1f} x {:.1f} - {}", name, rect.get_width(), rect.get_height(),
                            size.x == 0 ? "content-sized" : matches ? "PASS" : "FAIL: size differs");
                        liteButtonStatus->set_text(report);
                        INFO("Lite button sizing: {}", report);
                    };
                    UnityEngine::UI::Button* button;
                    if (variant.buttonTemplate) {
                        button = size.x == 0
                            ? Lite::CreateUIButton(row, "Size test", variant.buttonTemplate, clicked)
                            : Lite::CreateUIButton(row, "Size test", variant.buttonTemplate, {0, 0}, size, clicked);
                    } else {
                        button = size.x == 0
                            ? Lite::CreateUIButton(row, "Size test", clicked)
                            : Lite::CreateUIButton(row, "Size test", UnityEngine::Vector2{0, 0}, size, clicked);
                    }
                    auto text = button->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
                    text->set_enableAutoSizing(false);
                    text->set_fontSize(3);
                    text->set_enableWordWrapping(false);
                }
            }
        }
        // Exercise both supported ID types: ColorSetting above and its nested
        // ModalColorPicker here. Keep the owning setting for state diagnostics.
        if (boundColorPicker) {
            auto parent = boundColorPicker->get_transform()->get_parent();
            deferredColorSetting = parent ? parent->GetComponent<ColorSetting*>() : nullptr;
        }
        // Keep each test page within its viewport. The shared scroll template fits
        // its width to its contents, so long diagnostic strings otherwise widen it.
        for (auto scroll : {testScroll, diScroll, iconScroll, buttonScroll, eventScroll, paddingScroll, spacingScroll, sizingScroll, liteScroll, segmentScroll, settingsScroll, colorScroll, nameScroll, anchorScroll, fallbackScroll}) {
            if (!scroll) continue;
            auto content = scroll->_contentRectTransform;
            content->GetComponent<UnityEngine::UI::ContentSizeFitter*>()->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained);
            content->GetComponent<UnityEngine::UI::VerticalLayoutGroup*>()->set_childForceExpandWidth(true);
            auto size = content->get_sizeDelta();
            size.x = 0;
            content->set_sizeDelta(size);
        }
        auto addLayoutRefresh = [](CustomListTableData* data, TMPro::TextMeshProUGUI* label) {
            if (!data || !label || data->GetComponent<ComponentTestListLayout*>()) return;
            auto refresh = data->get_gameObject()->AddComponent<ComponentTestListLayout*>();
            refresh->data = data;
            refresh->status = label;
        };
        addLayoutRefresh(songList, listStatus);
        addLayoutRefresh(simpleList, simpleStatus);
        addLayoutRefresh(boxList, boxStatus);
        INFO("Component test page constructed; exercise each section again after game reload");
    }

    void RegisterComponentTestPage() {
        if (std::ifstream("/sdcard/ModData/com.beatgames.beatsaber/Mods/BSML/run-hot-reload-tests").good()) {
            MainThreadScheduler::ScheduleUntil([] {
                for (auto menu : UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::MainMenuViewController*>())
                    if (menu->get_isActivated()) return true;
                return false;
            }, [] {
                MainThreadScheduler::ScheduleAfterTime(2.0f, [] {
                    RunHotReloadTests();
                    RunViewControllerTests();
                    RunSnapshotBindingTests();
                    MainThreadScheduler::ScheduleAfterTime(2.0f, [] {
                        RunHotReloadTests();
                    });
                });
            });
        }
        // Opt-in device verification; normal launches do not run test fixtures.
        if (std::ifstream("/sdcard/ModData/com.beatgames.beatsaber/Mods/BSML/run-parsing-tests").good()) {
            MainThreadScheduler::ScheduleUntil([] {
                for (auto menu : UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::MainMenuViewController*>())
                    if (menu->get_isActivated()) return true;
                return false;
            }, [] {
                MainThreadScheduler::ScheduleAfterTime(2.0f, [] {
                    INFO("PARSING-VERIFY BEGIN pass 1");
                    INFO("PARSING-VERIFY {}", RunPaddingParserTests());
                    INFO("PARSING-VERIFY {}", RunRectTransformTests());
                    INFO("PARSING-VERIFY {}", RunViewControllerTests());
                    INFO("PARSING-VERIFY END pass 1");
                    MainThreadScheduler::ScheduleAfterTime(2.0f, [] {
                        INFO("PARSING-VERIFY BEGIN pass 2");
                        INFO("PARSING-VERIFY {}", RunRectTransformTests());
                        INFO("PARSING-VERIFY {}", RunViewControllerTests());
                        INFO("PARSING-VERIFY END pass 2");
                        if (std::ifstream("/sdcard/ModData/com.beatgames.beatsaber/Mods/BSML/run-solo-smoke-test").good()) {
                            MainThreadScheduler::ScheduleAfterTime(1.0f, [] {
                                INFO("PARSING-VERIFY opening Solo for integration smoke test");
                                auto menu = Helpers::GetDiContainer()->Resolve<GlobalNamespace::MainMenuViewController*>();
                                menu->_soloButton->get_onClick()->Invoke();
                                MainThreadScheduler::ScheduleAfterTime(10.0f, [] {
                                    INFO("PARSING-VERIFY Solo integration remained alive for 10 seconds");
                                });
                            });
                        }
                    });
                });
            });
        }
        RegisterColorInteractionTests();
        auto host = ComponentTestPage::New_ctor();
        Register::RegisterSettingsMenu("Component Test", MOD_ID "_component_test", host, false);
        host->testMenuButton = Register::RegisterMenuButton("Component Test", "Open Mod Settings, then select Component Test", [host] {
            ++host->menuClicks;
            INFO("Component test menu button activation: {}", host->menuClicks);
            BSMLSettings::get_instance()->PresentSettings();
        });
        Register::RegisterGameplaySetupTab("BSML Test", [host](UnityEngine::GameObject* parent, bool firstActivation) {
            ++host->gameplayActivations;
            INFO("Component test gameplay tab activation: {}, first={}", host->gameplayActivations, firstActivation);
            if (firstActivation) {
                parse_and_construct("<vertical><text text='BSML gameplay setup test' align='Center'/><text text='Tab created successfully. Check again after game reload.' font-size='3' word-wrapping='true'/></vertical>", parent->get_transform());
            }
            auto title = parent->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
            if (title) title->set_text(fmt::format("Gameplay tab activations: {}", host->gameplayActivations));
        });
    }
}

BSML_DATACACHE(component_test) {
    static constexpr uint8_t content[] = {
        #embed "../../assets/Settings/ComponentTest.bsml"
    };
    return ArrayW<uint8_t>(std::span(content));
}
