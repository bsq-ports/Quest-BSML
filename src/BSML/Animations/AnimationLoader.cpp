#include "BSML/Animations/AnimationLoader.hpp"
#include "BSML/Animations/AnimationInfo.hpp"
#include "BSML/Animations/GIF/GifDecoder.hpp"
#include "GIF/GifStreaming.hpp"
#include "ImageAnimationLoader.hpp"
#include "logging.hpp"

#include "UnityEngine/SystemInfo.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/WaitUntil.hpp"
#include "UnityEngine/TextureWrapMode.hpp"
#include "UnityEngine/TextureFormat.hpp"
#include "UnityEngine/FilterMode.hpp"
#include "System/IntPtr.hpp"
#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/Resources.hpp"
#include "System/IO/File.hpp"

#include "Helpers/delegates.hpp"

#include "beatsaber-hook/shared/safeptr.hpp"
#include "beatsaber-hook/shared/arrayw.hpp"
#include "BSML/SharedCoroutineStarter.hpp"
#include "System/Func_1.hpp"
#include <mutex>

DEFINE_TYPE(BSML, AnimationLoader);

namespace BSML {
    namespace {
        custom_types::Helpers::Coroutine ProcessAnimationInfoOwned(
            std::shared_ptr<AnimationInfo> infoOwner,
            std::function<void(UnityEngine::Texture2D*, ArrayW<UnityEngine::Rect>, ArrayW<float>)> onProcessed,
            std::function<void()> onError, std::shared_ptr<detail::GifStream> stream = nullptr,
            detail::ImageAnimationCallback onImageProcessed = {});
    }

    int get_atlasSizeLimit() {
        static auto getMaxTextureSize = i2c::resolve_icall<int>("UnityEngine.SystemInfo::GetMaxTextureSize");
        static int maxSize = getMaxTextureSize.has_value() ? getMaxTextureSize.value()() : 4096;
        return maxSize >= 4096 ? 4096 : maxSize;
    }

    void AnimationLoader::Process(AnimationType type, ArrayW<uint8_t> data, std::function<void(UnityEngine::Texture2D*, ArrayW<UnityEngine::Rect>, ArrayW<float>)> onProcessed) {
        Process(type, data, onProcessed, [](){
            ERROR("Error happened while processing Animation, and error was not handled!");
        });
    }

    void AnimationLoader::Process(AnimationType type, ArrayW<uint8_t> data, std::function<void(UnityEngine::Texture2D*, ArrayW<UnityEngine::Rect>, ArrayW<float>)> onProcessed, std::function<void()> onError) {
        auto sharedStarter = BSML::SharedCoroutineStarter::get_instance();
        DEBUG("Starting animation decode");
        switch (type) {
            case AnimationType::GIF:
                sharedStarter->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(
                    detail::ProcessGifStreaming(
                        data,
                        [onProcessed, onError](auto stream) {
                            return ProcessAnimationInfoOwned(stream->info, onProcessed, onError, stream);
                        },
                        onError
                    )));
                break;
            case AnimationType::APNG:
                // TODO: implement APNG support at some point?
                if (onError) onError();
                break;
            default:
                break;
        }
    }

    custom_types::Helpers::Coroutine AnimationLoader::ProcessAnimationInfo(AnimationInfo* animationInfo, std::function<void(UnityEngine::Texture2D*, ArrayW<UnityEngine::Rect>, ArrayW<float>)> onProcessed, std::function<void()> onError) {
        // Take ownership before the returned coroutine's first resume.
        return ProcessAnimationInfoOwned(std::shared_ptr<AnimationInfo>(animationInfo), onProcessed, onError);
    }

    void detail::ProcessImageAnimation(AnimationLoader::AnimationType type, ArrayW<uint8_t> data,
        ImageAnimationCallback onProcessed, std::function<void()> onError) {
        if (type != AnimationLoader::AnimationType::GIF) {
            if (onError) onError();
            return;
        }
        SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(
            ProcessGifStreaming(data, [onProcessed, onError](auto stream) {
                return ProcessAnimationInfoOwned(stream->info, {}, onError, stream, onProcessed);
            }, onError)));
    }

    namespace {
    custom_types::Helpers::Coroutine ProcessAnimationInfoOwned(std::shared_ptr<AnimationInfo> infoOwner,
        std::function<void(UnityEngine::Texture2D*, ArrayW<UnityEngine::Rect>, ArrayW<float>)> onProcessed,
        std::function<void()> onError, std::shared_ptr<detail::GifStream> stream,
        detail::ImageAnimationCallback onImageProcessed) {
        detail::GifStreamConsumer consumer{stream};
        auto animationInfo = infoOwner.get();
        if (!animationInfo || animationInfo->frameCount <= 0 || animationInfo->width <= 0 ||
            animationInfo->height <= 0 || (!stream && animationInfo->decodedFrames.load() != animationInfo->frameCount)) {
            if (onError) onError();
            co_return;
        }
        DEBUG("ProcessAnimInfo");
        int textureSize = AnimationLoader::GetTextureSize(animationInfo);
        safe_ptr<ArrayW<UnityEngine::Texture2D*>> textureListSafe = ArrayW<UnityEngine::Texture2D*>(animationInfo->frameCount);
        ArrayW<UnityEngine::Texture2D*> textureList(textureListSafe.ptr());
        // Destroy native textures on every exit, including a cancelled coroutine.
        struct TextureCleanup {
            ArrayW<UnityEngine::Texture2D*> frames;
            safe_ptr<UnityEngine::Texture2D*, true> atlas;
            ~TextureCleanup() {
                for (auto texture : frames)
                    if (texture && texture->m_CachedPtr.m_value) UnityEngine::Object::DestroyImmediate(texture);
                if (atlas) UnityEngine::Object::DestroyImmediate(atlas.ptr());
            }
        } cleanup{textureList};
        safe_ptr<ArrayW<float>> delaysSafe = ArrayW<float>(animationInfo->frameCount);
        ArrayW<float> delays(delaysSafe.ptr());
        float lastThrottleTime = UnityEngine::Time::get_realtimeSinceStartup();

        auto layout = detail::IndexedAtlasLayout::Make(animationInfo->width, animationInfo->height,
            animationInfo->frameCount, textureSize);
        // Two reusable RGBA surfaces cost eight bytes per frame pixel. Small
        // animations need less storage as a conventional RGBA atlas.
        bool useIndexed = onImageProcessed && layout.columns &&
            int64_t(layout.width) * layout.height + int64_t(animationInfo->frameCount) * 1024 +
                int64_t(animationInfo->width) * animationInfo->height * 8 <
                int64_t(animationInfo->width) * animationInfo->height * animationInfo->frameCount * 4 &&
            detail::IndexedAnimation::Supported();
        std::vector<detail::IndexedFrame> indexedFrames;
        auto makeTexture = [&](int index, const uint8_t* data, size_t size) {
            auto texture = UnityEngine::Texture2D::New_ctor(animationInfo->width, animationInfo->height,
                UnityEngine::TextureFormat::RGBA32, false);
            textureList[index] = texture;
            texture->set_hideFlags(UnityEngine::HideFlags::DontSave);
            texture->set_wrapMode(UnityEngine::TextureWrapMode::Clamp);
            texture->LoadRawTextureData(System::IntPtr(const_cast<uint8_t*>(data)), size);
        };

        for (int currentFrameIndex = 0; currentFrameIndex < animationInfo->frameCount; currentFrameIndex++) {
            DEBUG("Frame {}", currentFrameIndex);

            std::shared_ptr<FrameInfo> currentFrameInfo;
            if (stream) {
                for (;;) {
                    auto next = stream->Read();
                    if (next.state == detail::GifStream::State::Cancelled) co_return;
                    if (next.state == detail::GifStream::State::Failed) {
                        if (onError) onError();
                        co_return;
                    }
                    currentFrameInfo = std::move(next.frame);
                    if (currentFrameInfo || next.state == detail::GifStream::State::Completed) break;
                    co_yield nullptr;
                }
            } else {
                currentFrameInfo = animationInfo->PopNextFrame();
            }
            if (!currentFrameInfo || currentFrameInfo->width != animationInfo->width ||
                currentFrameInfo->height != animationInfo->height) {
                if (onError) onError();
                co_return;
            }

            delays[currentFrameIndex] = currentFrameInfo->delay;

            auto colors = currentFrameInfo->colors.ptr();
            if (useIndexed) {
                detail::IndexedFrame frame;
                if (frame.Encode({colors->_values, static_cast<size_t>(colors.size())})) {
                    indexedFrames.push_back(std::move(frame));
                } else {
                    // A later composited frame may exceed 256 colors. Restore
                    // all earlier frames losslessly and fall back for this GIF.
                    useIndexed = false;
                    for (int i = 0; i < indexedFrames.size(); ++i) {
                        auto rgba = indexedFrames[i].Decode();
                        makeTexture(i, rgba.data(), rgba.size());
                        co_yield nullptr;
                    }
                    indexedFrames.clear();
                }
            }
            if (!useIndexed) makeTexture(currentFrameIndex, colors->_values, colors.size());
            currentFrameInfo.reset(); // Release the decoded buffer before yielding.

            if (UnityEngine::Time::get_realtimeSinceStartup() > lastThrottleTime + 0.0005f) {
                co_yield nullptr;
                lastThrottleTime = UnityEngine::Time::get_realtimeSinceStartup();
            }
        }
        // Do not publish an atlas until the producer confirms the whole decode
        // succeeded, even if the final frame was consumed before it finished.
        if (stream) {
            while (stream->GetState() == detail::GifStream::State::Decoding) co_yield nullptr;
            if (stream->GetState() == detail::GifStream::State::Cancelled) co_return;
            if (stream->GetState() != detail::GifStream::State::Completed ||
                animationInfo->decodedFrames.load() != animationInfo->frameCount) {
                if (onError) onError();
                co_return;
            }
        }
        if (useIndexed) {
            cleanup.atlas = UnityEngine::Texture2D::New_ctor(animationInfo->width, animationInfo->height,
                UnityEngine::TextureFormat::RGBA32, false, false);
            cleanup.atlas->set_hideFlags(UnityEngine::HideFlags::DontSave);
            cleanup.atlas->set_wrapMode(UnityEngine::TextureWrapMode::Clamp);
            cleanup.atlas->set_filterMode(UnityEngine::FilterMode::Bilinear);
            // Upload once and discard the CPU copy. Never Apply after CopyTexture.
            cleanup.atlas->Apply(false, true);
            auto indexed = detail::IndexedAnimation::Create(indexedFrames, layout, cleanup.atlas.ptr());
            if (indexed) {
                safe_ptr<ArrayW<UnityEngine::Rect>> rects = ArrayW<UnityEngine::Rect>(animationInfo->frameCount);
                for (auto& rect : rects.ptr()) rect = UnityEngine::Rect(0, 0, 1, 1);
                auto output = cleanup.atlas.ptr();
                cleanup.atlas = nullptr;
                onImageProcessed(output, rects.ptr(), delays, std::move(indexed));
                co_return;
            }
            UnityEngine::Object::DestroyImmediate(cleanup.atlas.ptr());
            cleanup.atlas = nullptr;
            for (int i = 0; i < indexedFrames.size(); ++i) {
                auto rgba = indexedFrames[i].Decode();
                makeTexture(i, rgba.data(), rgba.size());
                co_yield nullptr;
            }
        }
        safe_ptr<UnityEngine::Texture2D*> resultTexture = UnityEngine::Texture2D::New_ctor(animationInfo->width, animationInfo->height);

        cleanup.atlas = resultTexture.ptr();

        // The packed atlas is no longer CPU-readable. The callback owns it.
        DEBUG("Packing gif textures");
        safe_ptr<ArrayW<::UnityEngine::Rect>> atlasSafe = resultTexture->PackTextures(textureList, 2, textureSize, true);
        if (!atlasSafe.ptr() || atlasSafe.ptr().size() != animationInfo->frameCount) {
            if (onError) onError();
            co_return;
        }
        if (onImageProcessed) {
            cleanup.atlas = nullptr;
            onImageProcessed(resultTexture.ptr(), atlasSafe.ptr(), delays, nullptr);
        } else if (onProcessed) {
            cleanup.atlas = nullptr; // Ownership transfers to the completion handler.
            onProcessed(resultTexture.ptr(), atlasSafe.ptr(), delays);
        }

        co_return;
    }
    }

    int AnimationLoader::GetTextureSize(AnimationInfo* animationInfo) {
        if (!animationInfo || animationInfo->frameCount <= 0 || animationInfo->width <= 0 || animationInfo->height <= 0)
            return 0;
        // PackTextures handles fitting/downscaling at the device limit. A fixed
        // upper bound also handles one-frame and non-square GIFs without a search.
        return get_atlasSizeLimit();
    }
}
