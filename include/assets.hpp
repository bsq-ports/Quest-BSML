#pragma once

#include <string_view>
#include "beatsaber-hook/shared/utils/typedefs.h"

struct IncludedAsset {

    IncludedAsset(uint8_t* start, uint8_t* end) : array(reinterpret_cast<Array<uint8_t>*>(start)) {
        array->klass = nullptr;
        array->monitor = nullptr;
        array->bounds = nullptr;
        array->max_length = end - start - 33;
        *(end - 1)= '\0';
    }

    operator ArrayW<uint8_t>() const {
        init();
        return array;
    }

    operator std::string_view() const {
        return { reinterpret_cast<char*>(array->values), array->Length() };
    }

    operator std::span<uint8_t>() const {
        return { array->values, array->Length() };
    }

    void init() const {
        if(!array->klass)
            array->klass = classof(Array<uint8_t>*);
    }

    private:
        Array<uint8_t>* array;

};

#define DECLARE_FILE(name)                         \
    extern "C" uint8_t _binary_##name##_start[];  \
    extern "C" uint8_t _binary_##name##_end[];    \
    const IncludedAsset name { _binary_##name##_start, _binary_##name##_end};

namespace IncludedAssets {

	DECLARE_FILE(GameplaySetup_bsml)
	DECLARE_FILE(GameplaySetupCell_bsml)
	DECLARE_FILE(GameplayTabError_bsml)
	DECLARE_FILE(MainLeftScreen_bsml)
	DECLARE_FILE(SettingsAbout_bsml)
	DECLARE_FILE(SettingsButtons_bsml)
	DECLARE_FILE(SettingsError_bsml)
	DECLARE_FILE(SettingsList_bsml)
	DECLARE_FILE(ToastView_bsml)
	DECLARE_FILE(mods_idle_png)
	DECLARE_FILE(mods_selected_png)
	DECLARE_FILE(visibility_png)

}
