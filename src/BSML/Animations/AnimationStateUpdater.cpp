#include "BSML/Animations/AnimationStateUpdater.hpp"

DEFINE_TYPE(BSML, AnimationStateUpdater);

namespace BSML {
    AnimationControllerData* AnimationStateUpdater::get_controllerData() {
        return _controllerData;
    }

    void AnimationStateUpdater::set_controllerData(AnimationControllerData* value) {
        if (_controllerData) {
            OnDisable();
            _controllerData->Remove(this);
        }
        _controllerData = value;
        if (_controllerData) _controllerData->Add(this);

        if (get_isActiveAndEnabled()) {
            OnEnable();
        } else {
            ApplyCurrentFrame();
        }
    }

    void AnimationStateUpdater::ApplyCurrentFrame() {
        // Defensive checks, hot reload can invalidate anything
        if (!_controllerData || !image) return;
        auto sprites = _controllerData->sprites;
        auto index = _controllerData->uvIndex;
        if (!sprites || index < 0 || index >= sprites.size()) return;
        UnityW<UnityEngine::Sprite> sprite = sprites[index];
        if (sprite) image->set_sprite(sprite);
    }

    void AnimationStateUpdater::OnEnable() {
        if (_controllerData && image) {
            _controllerData->get_activeImages()->Add(image);
            ApplyCurrentFrame();
        }
    }

    void AnimationStateUpdater::OnDisable() {
        if (_controllerData) {
            _controllerData->get_activeImages()->Remove(image);
        }
    }

    void AnimationStateUpdater::OnDestroy() {
        if (_controllerData) {
            _controllerData->get_activeImages()->Remove(image);

            _controllerData->Remove(this);
        }
        _controllerData = nullptr;
        image = nullptr;
    }
}
