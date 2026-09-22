#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <set>
#include <vector>

// Only the Unity/IL2CPP boundary is simulated. Request and updater logic are
// compiled from production sources by tests/host/CMakeLists.txt.
#define DEFINE_TYPE(...)
namespace UnityEngine {
struct Object {
    struct { uintptr_t m_value = 1; } m_CachedPtr;
};
struct Sprite : Object {};
namespace Networking {
struct UnityWebRequest {
    int aborts = 0;
    int disposals = 0;
    void Abort() { ++aborts; }
    void Dispose() { ++disposals; }
};
}
namespace UI {
struct Image : Object {
    Sprite* sprite = nullptr;
    std::function<void()> dirty;
    void set_sprite(Sprite* value) {
        assert(m_CachedPtr.m_value);
        if (sprite == value) return;
        sprite = value;
        // Listeners may unregister themselves or replace the callback.
        auto callback = dirty;
        if (callback) callback();
    }
};
}
}
namespace BSML {
class AnimationStateUpdater;
struct Images {
    std::vector<UnityEngine::UI::Image*> values;
    void Add(UnityEngine::UI::Image* image) { values.push_back(image); }
    void Remove(UnityEngine::UI::Image* image) {
        auto it = std::find(values.begin(), values.end(), image);
        if (it != values.end()) values.erase(it);
    }
};
struct Sprites {
    std::vector<UnityEngine::Sprite*> values;
    explicit operator bool() const { return !values.empty(); }
    int size() const { return static_cast<int>(values.size()); }
    UnityEngine::Sprite* operator[](int index) const { return values.at(index); }
};
struct AnimationControllerData {
    Sprites sprites;
    int uvIndex = 0;
    Images active;
    std::set<AnimationStateUpdater*> users;
    Images* get_activeImages() { return &active; }
    void Add(AnimationStateUpdater* updater) { assert(users.insert(updater).second); }
    void Remove(AnimationStateUpdater* updater) { assert(users.erase(updater) == 1); }
};
class AnimationStateUpdater : public UnityEngine::Object {
public:
    struct Enabled {
        AnimationStateUpdater* owner;
        bool value = true;
        std::function<void()> changed;
        operator bool() const { return value; }
        void operator=(bool next) {
            assert(owner->m_CachedPtr.m_value);
            if (next == value) return;
            value = next;
            if (owner->active) {
                if (next) owner->OnEnable();
                else owner->OnDisable();
            }
            auto callback = changed;
            if (callback) callback();
        }
    };
    UnityEngine::UI::Image* image = nullptr;
    AnimationControllerData* _controllerData = nullptr;
    uint64_t imageLoadGeneration = 0;
    UnityEngine::Networking::UnityWebRequest* imageDownload = nullptr;
    Enabled enabled{this};
    bool active = true;
    bool get_isActiveAndEnabled() const { return active && enabled; }
    void CancelImageLoad();
    AnimationControllerData* get_controllerData();
    void set_controllerData(AnimationControllerData*);
    void OnEnable();
    void OnDisable();
    void OnDestroy();
private:
    void ApplyCurrentFrame();
};
}
