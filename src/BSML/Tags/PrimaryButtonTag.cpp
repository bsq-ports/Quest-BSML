#include "BSML/Tags/PrimaryButtonTag.hpp"
#include "GlobalNamespace/PracticeViewController.hpp"
#include "Helpers/getters.hpp"
#include "UnityEngine/UI/Button.hpp"

#include "beatsaber-hook/shared/safeptr.hpp"

using namespace UnityEngine;
using namespace UnityEngine::UI;

namespace BSML {
    static BSMLNodeParser<PrimaryButtonTag> primaryButtonTagParser({"action-button", "primary-button"});
    Button* PrimaryButtonTag::get_buttonPrefab() const {
        static safe_ptr<Button*> playButtonPrefab;
        if (!playButtonPrefab) {
            playButtonPrefab = Helpers::GetDiContainer()->Resolve<GlobalNamespace::PracticeViewController*>()->_playButton;
        }
        return playButtonPrefab.ptr();
    }
}
