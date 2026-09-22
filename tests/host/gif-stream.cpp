#include "GifStream.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

using BSML::FrameInfo;
using BSML::detail::GifStream;
using BSML::detail::GifStreamConsumer;
using namespace std::chrono_literals;

std::shared_ptr<FrameInfo> Frame(int value) {
    auto frame = std::make_shared<FrameInfo>(64, 64);
    std::fill_n(frame->colors.ptr().begin(), frame->colors.ptr().size(), uint8_t(value));
    frame->delay = value;
    return frame;
}

void BoundedProducerAndCancellation() {
    for (bool cancel : {false, true}) {
        auto stream = std::make_shared<GifStream>();
        assert(stream->Initialize());
        assert(stream->Push(Frame(1)) && stream->Push(Frame(2)));
        std::promise<void> entered;
        auto producer = std::async(std::launch::async, [stream, &entered] {
            auto frame = Frame(3);
            entered.set_value();
            return stream->Push(std::move(frame));
        });
        entered.get_future().get();
        assert(producer.wait_for(25ms) == std::future_status::timeout);
        assert(stream->info->decodedFrames == GifStream::Capacity);
        if (cancel) stream->Cancel();
        else {
            auto next = stream->Read();
            assert(next.frame && next.frame->delay == 1);
        }
        assert(producer.wait_for(2s) == std::future_status::ready);
        assert(producer.get() == !cancel);
        if (cancel) {
            assert(!stream->Read().frame);
            stream->Finish(true); // A late producer completion cannot revive it.
            assert(stream->GetState() == GifStream::State::Cancelled);
        } else {
            stream->Finish(true);
            auto second = stream->Read(), third = stream->Read(), end = stream->Read();
            assert(second.frame->delay == 2 && third.frame->delay == 3);
            assert(end.state == GifStream::State::Completed && !end.frame);
        }
    }
}

void OrderedStreamingAndMemoryBound() {
    assert(ArrayW<uint8_t>::liveBuffers == 0);
    ArrayW<uint8_t>::peakBuffers = 0;
    auto stream = std::make_shared<GifStream>();
    constexpr int count = 1000;
    auto producer = std::async(std::launch::async, [stream] {
        stream->info->width = 64;
        stream->info->height = 64;
        stream->info->frameCount = count;
        assert(stream->Initialize());
        for (int i = 0; i < count; ++i) assert(stream->Push(Frame(i)));
        stream->Finish(true);
    });
    int received = 0;
    auto deadline = std::chrono::steady_clock::now() + 5s;
    for (;;) {
        assert(std::chrono::steady_clock::now() < deadline);
        auto next = stream->Read();
        if (next.state == GifStream::State::Starting) { std::this_thread::yield(); continue; }
        assert(stream->info->isInitialized && stream->info->width == 64 && stream->info->height == 64);
        assert(stream->info->frameCount == count);
        if (next.frame) {
            assert(next.frame->delay == received);
            auto pixels = next.frame->colors.ptr();
            for (size_t p = 0; p < pixels.size(); ++p) assert(pixels.begin()[p] == uint8_t(received));
            ++received;
        } else if (next.state == GifStream::State::Completed) break;
        else std::this_thread::yield();
    }
    assert(received == count && stream->info->decodedFrames == count);
    producer.get();
    // Two queued buffers, one pending producer, one consumer; independent of
    // the number of animation frames. This excludes giflib's own working set.
    assert(ArrayW<uint8_t>::peakBuffers <= GifStream::Capacity + 2);
    assert(ArrayW<uint8_t>::liveBuffers == 0);
}

void FailureAndConsumerTeardown() {
    auto stream = std::make_shared<GifStream>();
    stream->Finish(false);
    auto failed = stream->Read();
    assert(failed.state == GifStream::State::Failed && !failed.frame);

    stream = std::make_shared<GifStream>();
    assert(stream->Initialize());
    assert(stream->Push(Frame(1)));
    stream->Finish(false);
    failed = stream->Read();
    assert(failed.state == GifStream::State::Failed && !failed.frame);
    assert(!stream->Push(Frame(2)));

    stream = std::make_shared<GifStream>();
    std::weak_ptr<GifStream> lifetime = stream;
    auto consumer = std::make_unique<GifStreamConsumer>(stream);
    assert(stream->Initialize());
    assert(stream->Push(Frame(1)) && stream->Push(Frame(2)));
    auto producer = std::async(std::launch::async, [stream] { return stream->Push(Frame(3)); });
    stream.reset();
    consumer.reset();
    assert(producer.wait_for(2s) == std::future_status::ready);
    assert(!producer.get());
    producer = {};
    assert(lifetime.expired());
    assert(ArrayW<uint8_t>::liveBuffers == 0);

    stream = std::make_shared<GifStream>();
    stream->Cancel();
    assert(!stream->Initialize());
    assert(stream->Read().state == GifStream::State::Cancelled);
}

int main() {
    BoundedProducerAndCancellation();
    OrderedStreamingAndMemoryBound();
    FailureAndConsumerTeardown();
    std::cout << "PASS: bounded streaming, complete frames, failure, cancellation, and ownership\n";
}
