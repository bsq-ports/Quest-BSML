#include "BSML/ViewControllers/BSMLViewController.hpp"
#include "BSML/Parsing/BSMLParser.hpp"
#include "BSML/Parsing/ParseException.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Vector2.hpp"
#include "logging.hpp"

#include <unordered_map>

DEFINE_TYPE(BSML, BSMLViewController);

namespace {
    std::string EscapeXml(std::string_view text) {
        std::string escaped;
        for (char c : text) {
            switch (c) {
                case '&': escaped += "&amp;"; break;
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                case '\'': escaped += "&apos;"; break;
                case '"': escaped += "&quot;"; break;
                default: escaped += c; break;
            }
        }
        return escaped;
    }
}

namespace BSML {
    StringW BSMLViewController::get_Content() { return ""; }

    StringW BSMLViewController::get_FallbackContent() {
        return DefaultFallbackContent();
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

    namespace {
        // Legacy path: get_Content/get_FallbackContent overridden-by-name on a
        // derived IL2CPP class. Used only when `contentProvider` isn't set — see
        // BSMLContentProvider.hpp for the preferred, reflection-free alternative
        // (a plain virtual call works fine across mod .so boundaries, since every
        // BSML mod — BSML included — is a native shared library built with the
        // same NDK toolchain; it's only the *game's* IL2CPP-generated classes that
        // genuinely require reflection). Still cache the MethodInfo* per klass
        // instead of re-resolving by string every call.
        const MethodInfo* GetContentMethod(Il2CppClass* klass, const char* name) {
            static std::unordered_map<Il2CppClass*, const MethodInfo*> cache;
            auto it = cache.find(klass);
            if (it != cache.end()) return it->second;
            auto methodInfo = i2c::functions::class_get_method_from_name(klass, name, 0);
            cache.emplace(klass, methodInfo);
            return methodInfo;
        }

        StringW LegacyGetContent(BSML::BSMLViewController* self) {
            auto methodInfo = GetContentMethod(self->klass, "get_Content");
            if (!methodInfo) throw ParseException("View controller Content is null");
            return i2c::run_method<StringW>(self, methodInfo);
        }

        StringW LegacyGetFallbackContent(BSML::BSMLViewController* self) {
            auto methodInfo = GetContentMethod(self->klass, "get_FallbackContent");
            if (!methodInfo) throw ParseException("View controller FallbackContent is null");
            return i2c::run_method<StringW>(self, methodInfo);
        }
    }

    void BSMLViewController::ParseWithFallback() {
        if (destroyed) return;
        ClearContents();
        try {
            StringW content = contentProvider ? contentProvider->GetContent() : LegacyGetContent(this);
            if (!content) throw ParseException("View controller Content is null");
            BSMLParser::parse_and_construct(std::string(content), contentObject->get_transform(), this);
        } catch (const std::exception& error) {
            ERROR("Error parsing BSML: {}", error.what());
            ClearContents();
            StringW fallback = contentProvider ? contentProvider->GetFallbackContent() : LegacyGetFallbackContent(this);
            if (!fallback) throw ParseException("View controller FallbackContent is null");
            std::string markup = fallback;
            auto message = EscapeXml(error.what());
            for (size_t pos = 0; (pos = markup.find("{0}", pos)) != std::string::npos; pos += message.size())
                markup.replace(pos, 3, message);
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

    StringW DefaultFallbackContent() {
        return R"(<bg>
            <vertical child-control-height='false' child-control-width='true' child-align='UpperCenter' pref-width='110' pad-left='3' pad-right='3'>
                <horizontal bg='panel-top' pad-left='10' pad-right='10' horizontal-fit='PreferredSize' vertical-fit='PreferredSize'>
                    <text text='Invalid BSML' font-size='10'/>
                </horizontal>
            </vertical>
            <text-page text='{0}' anchor-min-x='0.1' anchor-max-x='0.9' anchor-max-y='0.8'/>
        </bg>)";
    }

    StringW BSMLContentProvider::GetFallbackContent() {
        return DefaultFallbackContent();
    }
}

