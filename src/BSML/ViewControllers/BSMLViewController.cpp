#include "BSML/ViewControllers/BSMLViewController.hpp"
#include "BSML/Parsing/BSMLParser.hpp"
#include "BSML/Parsing/ParseException.hpp"
#include "BSMLFallback.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Vector2.hpp"
#include "logging.hpp"

DEFINE_TYPE(BSML, BSMLViewController);

namespace BSML {
    StringW BSMLViewController::get_Content() { return ""; }

    StringW BSMLViewController::get_FallbackContent() {
        return detail::DefaultFallbackContent;
    }

    void BSMLViewController::ClearContents() {
        if (contentObject && contentObject->m_CachedPtr.m_value) {
            contentObject->SetActive(false);
            UnityEngine::Object::Destroy(contentObject);
        }
        contentObject = UnityEngine::GameObject::New_ctor("Contents");
        auto rect = contentObject->AddComponent<UnityEngine::RectTransform*>();
        rect->SetParent(get_transform(), false);
        rect->set_anchorMin({0, 0});
        rect->set_anchorMax({1, 1});
        rect->set_sizeDelta({0, 0});
        rect->set_anchoredPosition({0, 0});
    }

    void BSMLViewController::ParseWithFallback() {
        if (destroyed) return;
        ClearContents();
        try {
            auto content = i2c::run_method<StringW>(reinterpret_cast<Il2CppObject*>(this), "get_Content");
            if (!content) throw ParseException("View controller Content is null");
            BSMLParser::parse_and_construct(std::string(content), contentObject->get_transform(), this);
        } catch (const std::exception& error) {
            ERROR("Error parsing BSML: {}", error.what());
            ClearContents();
            auto fallback = i2c::run_method<StringW>(reinterpret_cast<Il2CppObject*>(this), "get_FallbackContent");
            if (!fallback) throw ParseException("View controller FallbackContent is null");
            auto markup = detail::FormatFallbackContent(std::string(fallback), error.what());
            // Like PC, fallback failures propagate; do not recursively parse a fallback.
            BSMLParser::parse_and_construct(markup, contentObject->get_transform(), this);
        }
    }

    void BSMLViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
        if (firstActivation) ParseWithFallback();
    }

    void BSMLViewController::OnDestroy() {
        destroyed = true;
        // The generated HMUI wrapper dispatches virtually; call the base method
        // explicitly to avoid re-entering this override.
        i2c::run_method(this, i2c::metadata_getter<&HMUI::ViewController::OnDestroy>::method_info());
    }
}
