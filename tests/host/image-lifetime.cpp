#include "ImageLoadRequest.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <type_traits>

using BSML::Utilities::detail::ImageLoadRequest;
using BSML::Utilities::detail::ImageDownload;

struct Fixture {
    UnityEngine::UI::Image image;
    UnityEngine::Sprite firstFrame, secondFrame, laterFrame;
    BSML::AnimationControllerData first, second;
    BSML::AnimationStateUpdater updater;
    Fixture() {
        updater.image = &image;
        first.sprites.values = {&firstFrame};
        second.sprites.values = {&secondFrame, &laterFrame};
    }
    std::shared_ptr<ImageLoadRequest> Begin() {
        updater.CancelImageLoad();
        return std::make_shared<ImageLoadRequest>(&image, &updater, "same.gif");
    }
};

void FirstFrameAndReplacement() {
    Fixture f;
    auto original = f.Begin();
    assert(original->ApplyAnimation(&f.first));
    assert(f.image.sprite == &f.firstFrame);
    auto next = f.Begin();
    assert(!original->IsCurrent() && next->IsCurrent());
    // Download/parsing has not completed: the old atlas still has an owner.
    assert(f.image.sprite == &f.firstFrame && f.first.users.contains(&f.updater));
    assert(next->ApplyAnimation(&f.second));
    assert(f.image.sprite == &f.secondFrame);
    assert(f.first.users.empty() && f.first.active.values.empty());
    assert(f.second.active.values.size() == 1);
    assert(!original->ApplyAnimation(&f.first));
    assert(!original->PrepareStaticImage());
    assert(f.image.sprite == &f.secondFrame);
}

void InactiveAndReenabled() {
    Fixture f;
    f.updater.active = false;
    auto request = f.Begin();
    assert(request->ApplyAnimation(&f.second));
    assert(f.image.sprite == &f.secondFrame && f.second.active.values.empty());
    f.second.uvIndex = 1;
    f.updater.active = true;
    f.updater.OnEnable();
    assert(f.image.sprite == &f.laterFrame && f.second.active.values.size() == 1);
    f.updater.enabled = false;
    assert(f.second.active.values.empty());
    f.second.uvIndex = 0;
    f.updater.enabled = true;
    assert(f.image.sprite == &f.secondFrame && f.second.active.values.size() == 1);
}

void FrameCallbackReentry() {
    for (int mode = 0; mode < 4; ++mode) {
        Fixture f;
        auto request = f.Begin();
        f.image.dirty = [&] {
            f.image.dirty = nullptr;
            if (mode == 0) {
                auto newer = f.Begin();
                assert(newer->ApplyAnimation(&f.second));
            } else if (mode == 1) {
                f.image.m_CachedPtr.m_value = 0;
            } else {
                f.updater.OnDestroy();
                f.updater.m_CachedPtr.m_value = 0;
                if (mode == 3) f.image.m_CachedPtr.m_value = 0;
            }
        };
        assert(!request->ApplyAnimation(&f.first));
        assert(!request->IsCurrent());
        if (mode == 0) assert(f.image.sprite == &f.secondFrame);
    }
}

void EnableAndDisableCallbackReentry() {
    for (bool disabling : {false, true}) {
        Fixture f;
        auto request = f.Begin();
        if (disabling) assert(request->ApplyAnimation(&f.first));
        else f.updater.enabled = false;
        f.updater.enabled.changed = [&] {
            f.updater.enabled.changed = nullptr;
            auto newer = f.Begin();
            assert(newer->ApplyAnimation(&f.second));
        };
        bool current = disabling ? request->PrepareStaticImage() : request->ApplyAnimation(&f.first);
        assert(!current && f.image.sprite == &f.secondFrame);
        assert(f.updater.get_controllerData() == &f.second && f.updater.enabled);
    }
}

void InvalidFramesAndTarget() {
    Fixture f;
    auto request = f.Begin();
    assert(request->ApplyAnimation(&f.first));
    f.second.uvIndex = -1;
    assert(request->ApplyAnimation(&f.second));
    assert(f.image.sprite == &f.firstFrame);
    f.second.uvIndex = 99;
    assert(request->ApplyAnimation(&f.second));
    assert(f.image.sprite == &f.firstFrame);
    f.second.uvIndex = 0;
    f.secondFrame.m_CachedPtr.m_value = 0;
    assert(request->ApplyAnimation(&f.second));
    assert(f.image.sprite == &f.firstFrame);
    UnityEngine::UI::Image otherImage;
    f.updater.image = &otherImage;
    assert(!request->IsCurrent());
    assert(!request->PrepareStaticImage());
}

void DownloadOwnershipAndTeardown() {
    static_assert(!std::is_copy_constructible_v<ImageDownload>);
    static_assert(!std::is_move_constructible_v<ImageDownload>);
    Fixture f;
    UnityEngine::Networking::UnityWebRequest oldDownload, newDownload;
    auto oldRequest = f.Begin();
    auto oldOwner = std::make_unique<ImageDownload>(oldRequest, &oldDownload);
    f.updater.imageDownload = &oldDownload;
    auto newRequest = f.Begin();
    assert(oldDownload.aborts == 1 && !oldRequest->IsCurrent());
    {
        ImageDownload newOwner(newRequest, &newDownload);
        f.updater.imageDownload = &newDownload;
        oldOwner.reset();
        assert(oldDownload.disposals == 1);
        assert(f.updater.imageDownload == &newDownload && newDownload.disposals == 0);
        assert(newRequest->ApplyAnimation(&f.first));
        f.updater.OnDestroy();
        f.updater.m_CachedPtr.m_value = 0;
        assert(newDownload.aborts == 1 && !newRequest->IsCurrent());
        assert(f.updater.image == nullptr && f.updater.get_controllerData() == nullptr);
        assert(f.first.users.empty() && f.first.active.values.empty());
    }
    assert(newDownload.disposals == 1);

    Fixture live;
    UnityEngine::Networking::UnityWebRequest failedDownload;
    auto request = live.Begin();
    try {
        ImageDownload owner(request, &failedDownload);
        live.updater.imageDownload = &failedDownload;
        throw std::runtime_error("callback failed");
    } catch (const std::runtime_error&) {}
    assert(failedDownload.disposals == 1 && live.updater.imageDownload == nullptr);
}

int main() {
    FirstFrameAndReplacement();
    InactiveAndReenabled();
    FrameCallbackReentry();
    EnableAndDisableCallbackReentry();
    InvalidFramesAndTarget();
    DownloadOwnershipAndTeardown();
    std::cout << "PASS: image request lifetime, reentry, first frames, download disposal\n";
}
