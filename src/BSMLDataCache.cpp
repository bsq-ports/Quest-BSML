#include "BSMLDataCache_internal.hpp"
#include "logging.hpp"
#include <map>

namespace BSML::DataCache {
    static auto& GetDataCache() {
        // Asset entries can register from another translation unit's static
        // constructor, before this translation unit has been initialized.
        static std::map<std::string, const Entry*> dataCache;
        return dataCache;
    }
    void RegisterEntry(std::string key, const Entry* value) {
        INFO("Registering Data, Key: {}", key);
        auto& dataCache = GetDataCache();
        auto itr = dataCache.find(key);
        if (itr != dataCache.end()) {
            ERROR("Registering the same key for datacache twice, don't do this!");
            ERROR("offending key: {}", key);
            return;
        }
        dataCache.emplace(key, value);
    }

    const Entry* Get(std::string key) {
        INFO("Getting data for key {}", key);
        auto& dataCache = GetDataCache();
        auto itr = dataCache.find(key);
        if (itr == dataCache.end()) {
            ERROR("Could not find key in datacache: {}", key);
            return nullptr;
        }
        return itr->second;
    }
}
