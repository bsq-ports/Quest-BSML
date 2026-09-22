#pragma once

#include "BSML/Animations/AnimationStateUpdater.hpp"
#include "beatsaber-hook/shared/safeptr.hpp"
#include "beatsaber-hook/shared/stringw.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace BSML::Utilities::detail {
    // Main-thread-only request state. Rooting managed wrappers does not keep
    // their native Unity objects alive, so every UI transition must check both.
    struct ImageLoadRequest {
        safe_ptr<UnityEngine::UI::Image*, true> image;
        safe_ptr<AnimationStateUpdater*, true> stateUpdater;
        safe_ptr<StringW> path;
        const uint64_t generation;

        ImageLoadRequest(UnityEngine::UI::Image* image, AnimationStateUpdater* stateUpdater, StringW path)
            : image(image), stateUpdater(stateUpdater), path(path), generation(stateUpdater->imageLoadGeneration) {}

        bool IsCurrent() const {
            return image && stateUpdater && stateUpdater->image == image.ptr()
                && stateUpdater->imageLoadGeneration == generation;
        }

        [[nodiscard]] bool ApplyAnimation(AnimationControllerData* data) const {
            if (!IsCurrent()) return false;
            stateUpdater->set_controllerData(data);
            // Frame assignment and enabling can invoke dirty callbacks that
            // replace this request or destroy its target.
            if (!IsCurrent()) return false;
            stateUpdater->enabled = true;
            return IsCurrent();
        }

        [[nodiscard]] bool PrepareStaticImage() const {
            if (!IsCurrent()) return false;
            stateUpdater->set_controllerData(nullptr);
            if (!IsCurrent()) return false;
            stateUpdater->enabled = false;
            return IsCurrent();
        }
    };

    // The coroutine owns disposal; the component only borrows the request to
    // abort it during replacement or teardown. Never copy this owner.
    struct ImageDownload {
        std::shared_ptr<ImageLoadRequest> request;
        safe_ptr<UnityEngine::Networking::UnityWebRequest*> download;

        ImageDownload(std::shared_ptr<ImageLoadRequest> request, UnityEngine::Networking::UnityWebRequest* download)
            : request(std::move(request)), download(download) {}
        ImageDownload(const ImageDownload&) = delete;
        ImageDownload& operator=(const ImageDownload&) = delete;

        ~ImageDownload() {
            // A completion callback may already have started another download.
            if (request->stateUpdater && request->stateUpdater->imageDownload == download.ptr())
                request->stateUpdater->imageDownload = nullptr;
            download->Dispose();
        }
    };
}
