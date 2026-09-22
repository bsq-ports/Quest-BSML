#include <cstring>
#include "GifValidation.hpp"
#include "GifStreaming.hpp"
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
        const std::function<void()>& onError, BSML::detail::GifStream* stream = nullptr);

    // The worker and coroutine share ownership until both have stopped using the
    // input and partial frames. Only the coroutine invokes user callbacks.
    struct DecodeJob {
        explicit DecodeJob(ArrayW<uint8_t> bytes) : data(bytes) {}
        safe_ptr<ArrayW<uint8_t>> data;
        std::unique_ptr<BSML::AnimationInfo> info = std::make_unique<BSML::AnimationInfo>();
        std::atomic<bool> done{false};
        bool failed = false; // Published by the release/acquire on done.
    };
}

namespace BSML::detail {
    custom_types::Helpers::Coroutine ProcessGifStreaming(ArrayW<uint8_t> data,
        std::function<custom_types::Helpers::Coroutine(std::shared_ptr<GifStream>)> consume,
        std::function<void()> onError) {
        auto stream = std::make_shared<GifStream>();
        GifStreamConsumer consumer{stream};
        try {
            il2cpp_thread([data = safe_ptr<ArrayW<uint8_t>>(data), stream]() {
                DecodeFrames(data.ptr(), stream->info.get(), {}, stream.get());
            }).detach();
        } catch (const std::exception& error) {
            ERROR("Could not start GIF decoder: {}", error.what());
            stream->Finish(false);
        }

        while (stream->GetState() == GifStream::State::Starting) co_yield nullptr;
        if (stream->GetState() == GifStream::State::Cancelled) co_return;
        if (stream->GetState() == GifStream::State::Failed) {
            if (onError) onError();
            co_return;
        }
        // Once the consumer starts, it handles late decoder errors. Nesting the
        // coroutine keeps cancellation attached to the entire loading pipeline.
        if (consume) co_yield custom_types::Helpers::CoroutineHelper::New(consume(stream));
    }
}

namespace BSML {

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
                ProcessingThread(job->data.ptr(), job->info.get(), [job]() { job->failed = true; });
                job->done.store(true, std::memory_order_release);
            }).detach();
        } catch (const std::exception& error) {
            ERROR("Could not start GIF decoder: {}", error.what());
            job->failed = true;
            job->done.store(true, std::memory_order_release);
        }

        while (!job->done.load(std::memory_order_acquire)) co_yield nullptr;
        if (job->failed) {
            if (onError) onError();
        } else if (onFinished) {
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
        const std::function<void()>& onError, BSML::detail::GifStream* stream) {
        DEBUG("Open gif");
        try {
            if (!animationInfo || !gifData ||
                !BSML::detail::ValidateGif({gifData->_values, static_cast<size_t>(gifData.size())}))
                throw EasyGifReader::Error::INVALID_GIF_FILE;
            auto gifReader = EasyGifReader::openMemory(gifData->_values, gifData.size());
            int width = gifReader.width(), height = gifReader.height(), frameCount = gifReader.frameCount();

            if (width <= 0 || height <= 0 || frameCount <= 0 ||
                uint64_t(width) * height * frameCount * 4 > 128ULL * 1024 * 1024)
                throw EasyGifReader::Error::INVALID_GIF_FILE;
            animationInfo->decodedFrames = 0;
            animationInfo->frameCount = frameCount;
            animationInfo->width = width;
            animationInfo->height = height;
            if (stream && !stream->Initialize()) return;

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
                if (stream) {
                    if (!stream->Push(std::move(outputFrameInfo))) return;
                } else {
                    animationInfo->PushFrame(std::move(outputFrameInfo));
                    ++animationInfo->decodedFrames;
                }
            }
            if (stream) stream->Finish(true);
            else animationInfo->isInitialized = true;
        } catch (EasyGifReader::Error gifError) {
            ERROR("Gif error: {}", errToString(gifError));
            if (stream) stream->Finish(false);
            if (onError) onError();
        } catch (const std::exception& error) {
            ERROR("GIF decode failed: {}", error.what());
            if (stream) stream->Finish(false);
            if (onError) onError();
        } catch (...) {
            ERROR("GIF decode failed with an unknown exception");
            if (stream) stream->Finish(false);
            if (onError) onError();
        }
    }
}
