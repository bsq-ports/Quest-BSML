#include <cstring>
#include "GifValidation.hpp"
#include "BSML/Animations/GIF/GifDecoder.hpp"
#include "logging.hpp"
#include "beatsaber-hook/shared/threading.hpp"

#include "EasyGifReader.h"

std::string errToString(const EasyGifReader::Error& err) {
    switch (err) {
        default:
        case EasyGifReader::Error::UNKNOWN: return "UNKNOWN";
        case EasyGifReader::Error::INVALID_OPERATION: return "INVALID_OPERATION";
        case EasyGifReader::Error::OPEN_FAILED: return "OPEN_FAILED";
        case EasyGifReader::Error::READ_FAILED: return "READ_FAILED";
        case EasyGifReader::Error::INVALID_FILENAME: return "INVALID_FILENAME";
        case EasyGifReader::Error::NOT_A_GIF_FILE: return "NOT_A_GIF_FILE";
        case EasyGifReader::Error::INVALID_GIF_FILE: return "INVALID_GIF_FILE";
        case EasyGifReader::Error::OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        case EasyGifReader::Error::CLOSE_FAILED: return "CLOSE_FAILED";
        case EasyGifReader::Error::NOT_READABLE: return "NOT_READABLE";
        case EasyGifReader::Error::IMAGE_DEFECT: return "IMAGE_DEFECT";
        case EasyGifReader::Error::UNEXPECTED_EOF: return "UNEXPECTED_EOF";
    }
}

namespace {
    void DecodeFrames(ArrayW<uint8_t> gifData, BSML::AnimationInfo* animationInfo,
        const std::function<void()>& onError, bool streaming = false);

    // The worker and coroutine share ownership until both have stopped using the
    // input and partial frames. Only the coroutine invokes user callbacks.
    struct DecodeJob {
        explicit DecodeJob(ArrayW<uint8_t> bytes) : data(bytes) {}
        safe_ptr<ArrayW<uint8_t>> data;
        std::unique_ptr<BSML::AnimationInfo> info = std::make_unique<BSML::AnimationInfo>();
    };
}

namespace BSML {
    custom_types::Helpers::Coroutine GifDecoder::ProcessStreaming(ArrayW<uint8_t> data,
        std::function<custom_types::Helpers::Coroutine(std::shared_ptr<AnimationInfo>)> consume,
        std::function<void()> onError) {
        auto info = std::make_shared<AnimationInfo>();
        detail::AnimationInfoConsumer consumer{info};
        try {
            il2cpp_thread([data = safe_ptr<ArrayW<uint8_t>>(data), info]() {
                DecodeFrames(data.ptr(), info.get(), {}, true);
            }).detach();
        } catch (const std::exception& error) {
            ERROR("Could not start GIF decoder: {}", error.what());
            info->Finish(false);
        }

        while (info->GetState() == AnimationInfo::State::Starting) co_yield nullptr;
        if (info->GetState() == AnimationInfo::State::Cancelled) co_return;
        if (info->GetState() == AnimationInfo::State::Failed) {
            if (onError) onError();
            co_return;
        }
        // Once the consumer starts, it handles late decoder errors. Nesting the
        // coroutine keeps cancellation attached to the entire loading pipeline.
        if (consume) co_yield custom_types::Helpers::CoroutineHelper::New(consume(info));
    }
    custom_types::Helpers::Coroutine GifDecoder::Process(ArrayW<uint8_t> data, std::function<void(AnimationInfo*)> onFinished) {
        co_yield custom_types::Helpers::CoroutineHelper::New(Process(data, onFinished, [](){
            ERROR("Unhandled gif processing error ocurred!");
        }));
        co_return;
    }

    custom_types::Helpers::Coroutine GifDecoder::Process(ArrayW<uint8_t> data, std::function<void(AnimationInfo*)> onFinished, std::function<void()> onError) {
        auto job = std::make_shared<DecodeJob>(data);
        try {
            il2cpp_thread([job]() {
                DecodeFrames(job->data.ptr(), job->info.get(), {});
            }).detach();
        } catch (const std::exception& error) {
            ERROR("Could not start GIF decoder: {}", error.what());
            job->info->Finish(false);
        }

        auto state = job->info->GetState();
        while (state == AnimationInfo::State::Starting || state == AnimationInfo::State::Decoding) {
            co_yield nullptr;
            state = job->info->GetState();
        }
        if (state == AnimationInfo::State::Failed) {
            if (onError) onError();
        } else if (state == AnimationInfo::State::Completed && onFinished) {
            // ProcessAnimationInfo takes ownership, including on failure.
            onFinished(job->info.release());
        }
    }

    void GifDecoder::ProcessingThread(ArrayW<uint8_t> gifData, AnimationInfo* animationInfo) {
        ProcessingThread(gifData, animationInfo, [](){
            ERROR("Unhandled Gif Processing Error ocurred!");
        });
    }

    void GifDecoder::ProcessingThread(ArrayW<uint8_t> gifData, AnimationInfo* animationInfo, std::function<void()> onError) {
        DecodeFrames(gifData, animationInfo, onError);
    }
}

namespace {
    void DecodeFrames(ArrayW<uint8_t> gifData, BSML::AnimationInfo* animationInfo,
        const std::function<void()>& onError, bool streaming) {
        DEBUG("Open gif");
        try {
            if (!animationInfo || !gifData ||
                !BSML::detail::ValidateGif({gifData->_values, static_cast<size_t>(gifData.size())}))
                throw EasyGifReader::Error::INVALID_GIF_FILE;
            auto gifReader = EasyGifReader::openMemory(gifData->_values, gifData.size());
            int width = gifReader.width(), height = gifReader.height(), frameCount = gifReader.frameCount();

            animationInfo->decodedFrames = 0;
            animationInfo->frameCount = frameCount;
            animationInfo->width = width;
            animationInfo->height = height;
            if (!animationInfo->Initialize(streaming)) return;

            DEBUG("iterating gif frames");
            for (const auto& gifFrame : gifReader) {
                auto outputFrameInfo = std::make_shared<BSML::FrameInfo>(gifFrame.width(), gifFrame.height());

                const uint8_t* pixels = (const uint8_t*)gifFrame.pixels();
                // get end of the data
                uint8_t* colorData = outputFrameInfo->colors.ptr().begin() + outputFrameInfo->colors.ptr().size();
                int height = gifFrame.height();
                int rowSize = sizeof(uint32_t) * gifFrame.width();
                // we need to iterate the given data in reverse due to unity's texture system
                for (int y = 0; y < height; y++) {
                    // pre-decrement because we start at end of data
                    colorData -= rowSize;
                    // EasyGifReader already composites transparency into RGBA.
                    std::memcpy(colorData, pixels, rowSize);
                    pixels += rowSize;
                }

                // delay in millis
                outputFrameInfo->delay = gifFrame.duration().milliseconds();

                // Only publish complete pixels and delay data. Streaming waits
                // here when the consumer has two frames queued already.
                if (!animationInfo->PushFrame(std::move(outputFrameInfo))) return;
            }
            animationInfo->Finish(true);
        } catch (EasyGifReader::Error gifError) {
            ERROR("Gif error: {}", errToString(gifError));
            if (animationInfo) animationInfo->Finish(false);
            if (onError) onError();
        } catch (const std::exception& error) {
            ERROR("GIF decode failed: {}", error.what());
            if (animationInfo) animationInfo->Finish(false);
            if (onError) onError();
        } catch (...) {
            ERROR("GIF decode failed with an unknown exception");
            if (animationInfo) animationInfo->Finish(false);
            if (onError) onError();
        }
    }
}
