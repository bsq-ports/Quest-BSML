#include "BSML/Components/HotReloadFileWatcher.hpp"
#include "logging.hpp"
#include "BSML.hpp"
#include "BSMLFallback.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/Transform.hpp"
#include <sys/stat.h>
#include <functional>
#include <exception>

DEFINE_TYPE(BSML, HotReloadFileWatcher);

using namespace UnityEngine;

namespace {
    void ClearChildren(UnityW<Transform> transform) {
        for (int i = transform->get_childCount() - 1; i >= 0; --i) {
            auto child = transform->GetChild(i)->get_gameObject();
            child->SetActive(false);
            Object::Destroy(child);
        }
        transform->DetachChildren();
    }
}

namespace BSML {
    void HotReloadFileWatcher::ctor() {
        INVOKE_CTOR();
        runCheck = true;
        checkInterval = 10.0f;
        lastFileEdit = 0;
        // trigger a reload instantly, if you give a filepath in didactivate it immediately reloads
        timeSinceLastCheck = checkInterval;
    }

    void HotReloadFileWatcher::Update() {
        if (!runCheck || filePath.empty()) {
            return;
        }

        timeSinceLastCheck += Time::get_deltaTime();

        if (timeSinceLastCheck >= checkInterval) {
            timeSinceLastCheck = 0;
            if (!fileexists(filePath)) {
                return;
            }

            struct stat fileStat = {0};
            if (stat(filePath.c_str(), &fileStat) == 0) {
                int editTime = fileStat.st_mtime;
                bool doReload = editTime > lastFileEdit;
                if (doReload) {
                    lastFileEdit = editTime;
                    Reload();
                }
            }
        }
    }

    void HotReloadFileWatcher::Reload() {
        if (!host) {
            ERROR("Host object not set, can't hot reload!");
            return;
        }
        std::string content = readfile(filePath);
        int newHash = std::hash<std::string>()(content);
        if (newHash != fileHash) {
            fileHash = newHash;
            auto t = get_transform();
            ClearChildren(t);
            parserParams.reset();
            try {
                parserParams = BSML::parse_and_construct(content, t, host)->parserParams;
            } catch (const std::exception& error) {
                ERROR("Could not hot reload '{}': {}", filePath, error.what());
                ClearChildren(t);
                try {
                    auto markup = detail::FormatFallbackContent(R"(<bg>
                        <text-page text='{0}' rich-text='false' anchor-min-x='0.1' anchor-max-x='0.9'/>
                    </bg>)", error.what());
                    // Error UI must not invoke bindings or PostParse on the failing host.
                    parserParams = BSML::parse_and_construct(markup, t, nullptr)->parserParams;
                } catch (const std::exception& fallbackError) {
                    ERROR("Could not display hot reload error: {}", fallbackError.what());
                    ClearChildren(t);
                }
            }
        } else {
            INFO("Content hash was not different, not reloading UI");
        }
    }

    void HotReloadFileWatcher::OnDestroy() {
        parserParams.reset();
    }
}
