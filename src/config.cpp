#include "config.hpp"
#include "logging.hpp"
#include "beatsaber-hook/shared/rapidjson.hpp"
#include "beatsaber-hook/shared/utils.hpp"

config_t config;
rapidjson::Document doc{rapidjson::kNullType};

#define Save(identifier) doc.AddMember(#identifier, config.identifier, allocator)

void SaveConfig() {
    INFO("Saving Configuration...");
    doc.RemoveAllMembers();
    doc.SetObject();

    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    rapidjson::Value hiddenTabs;
    hiddenTabs.SetArray();
    for (auto& tab : config.hiddenTabs) {
        hiddenTabs.PushBack(rapidjson::Value(tab.data(), tab.size(), allocator), allocator);
    }

    doc.AddMember("hiddenTabs", hiddenTabs, allocator);

    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter writer(buf);
    doc.Accept(writer);
    writefile(get_config_path(MOD_ID), buf.GetString());

    INFO("Saved Configuration!");
}

bool LoadConfig() {
    if (doc.IsNull()) {
        auto path = get_config_path(MOD_ID);
        if (!fileexists(path)) {
            writefile(path, "{}");
            doc.SetObject();
        } else {
            doc.Parse(readfile(path));
            if (doc.HasParseError() || !doc.IsObject()) {
                WARNING("Config was invalid! Clearing.");
                doc.SetObject();
            }
        }
    }

    bool foundEverything = true;

    auto hiddenTabsItr = doc.FindMember("hiddenTabs");
    if (hiddenTabsItr != doc.MemberEnd()) {
        for (auto& tab : hiddenTabsItr->value.GetArray()) {
            config.hiddenTabs.push_back(tab.GetString());
        }
    } else
        foundEverything = false;

    if (foundEverything)
        INFO("Loaded Configuration!");
    return foundEverything;
}
