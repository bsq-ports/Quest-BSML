#pragma once
#include "BSML/ViewControllers/BSMLViewController.hpp"
#include "TMPro/TMP_Text.hpp"
#include "HMUI/ImageView.hpp"
#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"

DECLARE_CLASS_CODEGEN(BSML, GifTestPage, HMUI::ViewController) {
    DECLARE_INSTANCE_FIELD(TMPro::TMP_Text*, report);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::HorizontalLayoutGroup*, previewRoot);
    DECLARE_INSTANCE_FIELD(BSML::BSMLViewController*, fixtureView);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, preview);
    DECLARE_INSTANCE_FIELD(int, runSerial);
    DECLARE_INSTANCE_FIELD(bool, loading);
    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::ViewController::DidActivate, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
    DECLARE_OVERRIDE_METHOD_MATCH(void, DidDeactivate, &HMUI::ViewController::DidDeactivate, bool removedFromHierarchy, bool screenSystemDisabling);
    DECLARE_INSTANCE_METHOD(void, RunOffline);
    DECLARE_INSTANCE_METHOD(void, RunNetwork);
    DECLARE_INSTANCE_METHOD(void, Stop);
    DECLARE_INSTANCE_METHOD(void, ShowA);
    DECLARE_INSTANCE_METHOD(void, ShowB);
    DECLARE_INSTANCE_METHOD(void, ShowSlowB);
    DECLARE_INSTANCE_METHOD(void, ShowBad);
    DECLARE_INSTANCE_METHOD(void, Rebuild);
    DECLARE_INSTANCE_METHOD(void, ToggleLoading);
    DECLARE_INSTANCE_METHOD(void, ToggleActive);
    DECLARE_INSTANCE_METHOD(void, NoFlash);
    DECLARE_INSTANCE_METHOD(void, SawFlash);
    public:
        void ResetPreview();
        void Log(std::string text);
        void Show(std::string path);
};
