#pragma once

#include "../../_config.h"
#include "beatsaber-hook/shared/safeptr.hpp"
#include "beatsaber-hook/shared/arrayw.hpp"

#include <memory>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace BSML {
    class FrameInfo;
    class BSML_EXPORT AnimationInfo {
        public:
            enum class State { Starting, Decoding, Completed, Failed, Cancelled };
            struct ReadResult {
                State state;
                std::shared_ptr<FrameInfo> frame;
            };
            static constexpr size_t StreamingCapacity = 2;

            ~AnimationInfo();
            bool isInitialized = false;

            /// @brief amount of frames in the animation, not neccesarily the amount of frames you can safely get
            int frameCount = 0;

            /// @brief how many frames have been fully decoded
            std::atomic<std::size_t> decodedFrames{0};

            /// @brief width of the animation
            int width = 0;
            /// @brief height of the animation
            int height = 0;

            /// @brief gets the next decoded frame
            std::shared_ptr<FrameInfo> PopNextFrame() {
                return Read().frame;
            }

            /// @brief adds a frame to the frames vector, though adding frames after frameCount should be impossible
            std::shared_ptr<FrameInfo> AddFrame(int width, int height) {
                std::lock_guard<std::mutex> lock(framesAccessMutex);
                return frames.emplace(std::make_shared<FrameInfo>(width, height));
            }

            /// @brief publish an already populated frame
            bool PushFrame(std::shared_ptr<FrameInfo> frame) {
                std::unique_lock lock(framesAccessMutex);
                spaceAvailable.wait(lock, [this] {
                    return state != State::Decoding || !streaming || frames.size() < StreamingCapacity;
                });
                if (state != State::Decoding) return false;
                frames.push(std::move(frame));
                ++decodedFrames;
                return true;
            }

            // Publish dimensions/frame count before consumers read them. Completed-result
            // decoding uses an unbounded queue because no consumer runs until it finishes.
            bool Initialize(bool streamFrames = false) {
                std::lock_guard lock(framesAccessMutex);
                if (state != State::Starting) return false;
                streaming = streamFrames;
                isInitialized = true;
                state = State::Decoding;
                return true;
            }

            ReadResult Read() {
                std::lock_guard lock(framesAccessMutex);
                std::shared_ptr<FrameInfo> frame;
                if (!frames.empty()) {
                    frame = std::move(frames.front());
                    frames.pop();
                    spaceAvailable.notify_one();
                }
                // Read completion and queue contents together to avoid treating a
                // concurrently published final frame as truncated input.
                return {state, std::move(frame)};
            }

            State GetState() {
                std::lock_guard lock(framesAccessMutex);
                return state;
            }

            void Finish(bool success) {
                std::lock_guard lock(framesAccessMutex);
                if (state == State::Cancelled) return;
                state = success ? State::Completed : State::Failed;
                if (!success) ClearFrames();
                spaceAvailable.notify_all();
            }

            void Cancel() {
                std::lock_guard lock(framesAccessMutex);
                state = State::Cancelled;
                ClearFrames();
                spaceAvailable.notify_all();
            }

        private:
            void ClearFrames() {
                while (!frames.empty()) frames.pop();
            }
            std::mutex framesAccessMutex;
            std::condition_variable spaceAvailable;
            std::queue<std::shared_ptr<FrameInfo>> frames;
            State state = State::Starting;
            bool streaming = false;
    };

    class BSML_EXPORT FrameInfo {
        public:
            int width, height, bpp;
            safe_ptr<ArrayW<uint8_t>> colors;
            int delay;
            FrameInfo(int width, int height, int bpp = 4);
    };

    namespace detail {
        // Wake a blocked producer when its consuming coroutine is destroyed.
        // The producer retains its own shared owner until it exits.
        struct AnimationInfoConsumer {
            explicit AnimationInfoConsumer(std::shared_ptr<AnimationInfo> info) : info(std::move(info)) {}
            AnimationInfoConsumer(const AnimationInfoConsumer&) = delete;
            AnimationInfoConsumer& operator=(const AnimationInfoConsumer&) = delete;
            std::shared_ptr<AnimationInfo> info;
            ~AnimationInfoConsumer() { if (info) info->Cancel(); }
        };
    }
}
