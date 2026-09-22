#pragma once

#include "BSML/Animations/AnimationInfo.hpp"
#include <condition_variable>

namespace BSML::detail {
    // Internal streaming transport. The public AnimationInfo layout and the
    // completed-result GifDecoder::Process API remain unchanged.
    class GifStream {
    public:
        enum class State { Starting, Decoding, Completed, Failed, Cancelled };
        struct ReadResult {
            State state;
            std::shared_ptr<FrameInfo> frame;
        };
        static constexpr size_t Capacity = 2;
        const std::shared_ptr<AnimationInfo> info = std::make_shared<AnimationInfo>();

        // Publish dimensions/frame count before the consumer starts reading.
        bool Initialize() {
            std::lock_guard lock(mutex);
            if (state != State::Starting) return false;
            info->isInitialized = true;
            state = State::Decoding;
            return true;
        }

        bool Push(std::shared_ptr<FrameInfo> frame) {
            std::unique_lock lock(mutex);
            spaceAvailable.wait(lock, [this] { return state != State::Decoding || buffered < Capacity; });
            if (state != State::Decoding) return false;
            info->PushFrame(std::move(frame));
            ++info->decodedFrames;
            ++buffered;
            return true;
        }

        ReadResult Read() {
            std::lock_guard lock(mutex);
            auto frame = info->PopNextFrame();
            if (frame) {
                --buffered;
                spaceAvailable.notify_one();
            }
            // Observe completion and queue contents under the same lock, so a
            // last-frame publication cannot be mistaken for truncated input.
            return {state, std::move(frame)};
        }

        State GetState() {
            std::lock_guard lock(mutex);
            return state;
        }

        void Finish(bool success) {
            std::lock_guard lock(mutex);
            if (state == State::Cancelled) return;
            state = success ? State::Completed : State::Failed;
            if (!success) ClearFrames();
            spaceAvailable.notify_all();
        }

        void Cancel() {
            std::lock_guard lock(mutex);
            state = State::Cancelled;
            ClearFrames();
            spaceAvailable.notify_all();
        }

    private:
        void ClearFrames() {
            while (info->PopNextFrame()) {}
            buffered = 0;
        }
        std::mutex mutex;
        std::condition_variable spaceAvailable;
        State state = State::Starting;
        size_t buffered = 0;
    };

    // The producer holds its own shared owner; dropping the consumer wakes a
    // blocked producer without joining its thread on Unity's main thread.
    struct GifStreamConsumer {
        explicit GifStreamConsumer(std::shared_ptr<GifStream> stream) : stream(std::move(stream)) {}
        GifStreamConsumer(const GifStreamConsumer&) = delete;
        GifStreamConsumer& operator=(const GifStreamConsumer&) = delete;
        std::shared_ptr<GifStream> stream;
        ~GifStreamConsumer() { if (stream) stream->Cancel(); }
    };
}
