#pragma once

#include "BSML/Parsing/BSMLEvent.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace BSML::detail {
    using EventMap = std::map<std::string, std::shared_ptr<BSMLEvent>>;

    template<class Callback>
    void ForEachEventId(std::string_view ids, Callback callback) {
        // Match PC's string.Split(','): whitespace, empty IDs and duplicates are literal.
        while (true) {
            const auto comma = ids.find(',');
            callback(std::string(ids.substr(0, comma)));
            if (comma == std::string_view::npos) return;
            ids.remove_prefix(comma + 1);
        }
    }

    inline std::weak_ptr<BSMLEvent> GetEvent(EventMap& events, const std::string& ids) {
        auto itr = events.find(ids);
        if (itr != events.end()) return itr->second;

        auto event = std::make_shared<BSMLEvent>();
        if (ids.find(',') != std::string::npos) {
            ForEachEventId(ids, [&](const std::string& id) {
                // The group keeps each event alive after the parser goes out of scope.
                auto member = GetEvent(events, id).lock();
                event->Add([member] { member->Invoke(); });
            });
        }
        events.emplace(ids, event);
        return event;
    }

    inline void EmitEvent(EventMap& events, const std::string& ids) {
        if (ids.find(',') != std::string::npos) {
            auto event = GetEvent(events, ids).lock();
            event->Invoke();
            return;
        }
        auto itr = events.find(ids);
        if (itr == events.end()) return;
        auto event = itr->second;
        event->Invoke();
    }

    inline void AddEvent(EventMap& events, const std::string& ids, std::function<void()> callback) {
        ForEachEventId(ids, [&](const std::string& id) {
            GetEvent(events, id).lock()->Add(callback);
        });
    }
}
