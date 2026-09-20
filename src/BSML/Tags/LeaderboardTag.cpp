#include "BSML/Tags/LeaderboardTag.hpp"
#include "BSML-Lite/Creation/Misc.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<LeaderboardTag> leaderboardTagParser({"leaderboard", "custom-leaderboard"});

    UnityEngine::GameObject* LeaderboardTag::CreateObject(UnityEngine::Transform* parent) const {
        return BSML::Lite::CreateLeaderboard(parent)->get_gameObject();
    }
}
