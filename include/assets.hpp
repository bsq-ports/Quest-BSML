#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++11-narrowing"

#include <cstdint>
#include <span>
#include <string_view>

namespace Assets {
    namespace raw {
        // --- text assets ---
        inline constexpr unsigned char MainLeftScreen[] = {
            #embed "../assets/MainLeftScreen.bsml" suffix(, 0)
        };
        inline constexpr unsigned char GameplaySetup[] = {
            #embed "../assets/GameplayMenu/GameplaySetup.bsml" suffix(, 0)
        };
        inline constexpr unsigned char GameplaySetupCell[] = {
            #embed "../assets/GameplayMenu/GameplaySetupCell.bsml" suffix(, 0)
        };
        inline constexpr unsigned char GameplayTabError[] = {
            #embed "../assets/GameplayMenu/GameplayTabError.bsml" suffix(, 0)
        };
        inline constexpr unsigned char About[] = {
            #embed "../assets/Settings/About.bsml" suffix(, 0)
        };
        inline constexpr unsigned char Buttons[] = {
            #embed "../assets/Settings/Buttons.bsml" suffix(, 0)
        };
        inline constexpr unsigned char Error[] = {
            #embed "../assets/Settings/Error.bsml" suffix(, 0)
        };
        inline constexpr unsigned char List[] = {
            #embed "../assets/Settings/List.bsml" suffix(, 0)
        };

        // --- image assets ---
        inline constexpr uint8_t CaratDown[] = { 
            #embed "../assets/carat_down.png" 
        };

        inline constexpr uint8_t CaratUp[] = { 
            #embed "../assets/carat_up.png" 
        };

        inline constexpr uint8_t Loading[] = { 
            #embed "../assets/loading.gif" 
        };

        inline constexpr uint8_t ModsIdle[] = { 
            #embed "../assets/mods_idle.png" 
        };

        inline constexpr uint8_t ModsSelected[] = { 
            #embed "../assets/mods_selected.png" 
        };

        inline constexpr uint8_t Visibility[] = { 
            #embed "../assets/visibility.png" 
        };
    }

#pragma clang diagnostic pop

    // helper: view over a null-suffixed raw text array, terminator excluded
    template <std::size_t N>
    inline std::string_view text(const unsigned char (&arr)[N]) {
        return {reinterpret_cast<const char*>(arr), N - 1};
    }

    namespace FlowCoordinators {
        inline std::string_view const MainLeftScreen = text(raw::MainLeftScreen);
    }

    namespace GameplayMenu {
        inline std::string_view const GameplaySetup = text(raw::GameplaySetup);
        inline std::string_view const GameplaySetupCell = text(raw::GameplaySetupCell);
        inline std::string_view const GameplayTabError = text(raw::GameplayTabError);
    }

    namespace Settings {
        inline std::string_view const About = text(raw::About);
        inline std::string_view const Buttons = text(raw::Buttons);
        inline std::string_view const Error = text(raw::Error);
        inline std::string_view const List = text(raw::List);
    }

    namespace Images {
        inline constexpr std::span<const uint8_t> CaratDown{raw::CaratDown};
        inline constexpr std::span<const uint8_t> CaratUp{raw::CaratUp};
        inline constexpr std::span<const uint8_t> Loading{raw::Loading};
        inline constexpr std::span<const uint8_t> ModsIdle{raw::ModsIdle};
        inline constexpr std::span<const uint8_t> ModsSelected{raw::ModsSelected};
        inline constexpr std::span<const uint8_t> Visibility{raw::Visibility};
    }
}