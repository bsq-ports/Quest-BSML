#include "BSML.hpp"
#include "BSML/ViewControllers/HotReloadViewController.hpp"
#include "BSML/Parsing/ParseException.hpp"
#include "Helpers/creation.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/GameObject.hpp"
#include "logging.hpp"
#include <cstdio>
#include <fstream>
#include <fcntl.h>
#include <sys/stat.h>

namespace BSML {
    std::string RunHotReloadTests() {
        int passed = 0, failed = 0;
        auto check = [&](std::string_view label, bool condition) {
            condition ? ++passed : ++failed;
            INFO("HOT-RELOAD-TEST {}: {}", condition ? "PASS" : "FAIL", label);
        };
        auto view = Helpers::CreateViewController<HotReloadViewController*>();
        auto path = fmt::format("/sdcard/ModData/com.beatgames.beatsaber/Mods/BSML/hot-reload-test-{}.bsml", view->GetInstanceID());
        auto write = [&](std::string_view markup, time_t timestamp) {
            std::ofstream file(path, std::ios::trunc);
            file << markup;
            file.close();
            if (!file) throw std::runtime_error("Could not write hot reload test file");
            timespec times[] = {{timestamp, 0}, {timestamp, 0}};
            if (utimensat(AT_FDCWD, path.c_str(), times, 0) != 0)
                throw std::runtime_error("Could not set test file timestamp");
        };
        auto hasText = [&](std::string_view needle) {
            for (auto text : view->GetComponentsInChildren<TMPro::TextMeshProUGUI*>(true))
                if (std::string(text->get_text()) == needle) return true;
            return false;
        };
        try {
            mkdir("/sdcard/ModData/com.beatgames.beatsaber/Mods/BSML", 0777);
            check("inherited Awake creates watcher", view->fileWatcher != nullptr);
            if (!view->fileWatcher) throw std::runtime_error("Watcher was not created");
            auto watcher = view->fileWatcher;
            watcher->filePath = path;
            watcher->checkInterval = 0;
            std::remove(path.c_str());
            // Existing clients construct directly beneath the controller transform.
            BSML::parse_and_construct("<text text='Initial content'/>", view->get_transform(), view);
            auto initial = view->get_transform()->GetChild(0)->get_gameObject();
            watcher->Update();
            check("missing file preserves initial controls", hasText("Initial content"));

            constexpr time_t timestamp = 1700000000;
            write("<text text='File content'/>", timestamp);
            watcher->Update();
            check("new file replaces initial controls", hasText("File content") && !hasText("Initial content") && !initial->m_CachedPtr.m_value);
            auto loaded = view->get_transform()->GetChild(0)->get_gameObject();
            watcher->Update();
            check("unchanged poll keeps controls", view->get_transform()->GetChild(0)->get_gameObject() == loaded);
            write("<text text='Same timestamp edit'/>", timestamp);
            watcher->Update();
            check("same-timestamp edit is ignored", hasText("File content"));
            write("<text text='Older timestamp edit'/>", timestamp - 1);
            watcher->Update();
            check("older-timestamp edit is ignored", hasText("File content"));
            write("<text text='Newer edit'/>", timestamp + 1);
            watcher->Update();
            check("newer-timestamp edit reloads", hasText("Newer edit") && !hasText("File content"));
            loaded = view->get_transform()->GetChild(0)->get_gameObject();
            write("<text text='Newer edit'/>", timestamp + 2);
            watcher->Update();
            check("new timestamp with identical content keeps controls", view->get_transform()->GetChild(0)->get_gameObject() == loaded);

            watcher->runCheck = false;
            write("<text text='Paused edit'/>", timestamp + 3);
            watcher->Update();
            check("paused watcher preserves controls", hasText("Newer edit"));
            watcher->runCheck = true;
            watcher->Update();
            check("resuming loads pending edit", hasText("Paused edit"));
            std::remove(path.c_str());
            watcher->Update();
            check("deleting file preserves last loaded controls", hasText("Paused edit"));

            write("<text>", timestamp + 4);
            try {
                watcher->Update();
                check("invalid file propagates parse error", false);
            } catch (const ParseException&) {
                check("invalid file propagates parse error", true);
            }
            write("<text text='Recovered'/>", timestamp + 5);
            watcher->Update();
            check("newer valid file recovers after error", hasText("Recovered"));
            watcher->filePath.clear();
            watcher->Update();
            check("empty path preserves controls", hasText("Recovered"));
        } catch (const std::exception& error) {
            check(fmt::format("unexpected exception: {}", error.what()), false);
        }
        std::remove(path.c_str());
        UnityEngine::Object::DestroyImmediate(view->get_gameObject());
        auto result = fmt::format("Hot reload: {} passed, {} failed.", passed, failed);
        INFO("HOT-RELOAD-TEST RESULT: {}", result);
        return result;
    }
}
