#include "GifTestPage.hpp"
#include "GifFixtures.hpp"
#include "BSML.hpp"
#include "BSMLDataCache.hpp"
#include "BSML/SharedCoroutineStarter.hpp"
#include "BSML/Animations/GIF/GifDecoder.hpp"
#include "../../src/BSML/Animations/GIF/GifStreaming.hpp"
#include "../../src/BSML/Animations/ImageAnimationLoader.hpp"
#include "UnityEngine/Graphics.hpp"
#include "UnityEngine/RenderTexture.hpp"
#include "UnityEngine/TextureFormat.hpp"
#include "UnityEngine/Color32.hpp"
#include "UnityEngine/Experimental/Rendering/GraphicsFormat.hpp"
#include "BSML/Animations/AnimationStateUpdater.hpp"
#include "BSML/Animations/AnimationController.hpp"
#include "BSML/FlowCoordinators/MainMenuHolderFlowCoordinator.hpp"
#include "UnityEngine/Resources.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "System/GC.hpp"
#include "Helpers/utilities.hpp"
#include "Helpers/creation.hpp"
#include "Helpers/delegates.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "logging.hpp"
#include <atomic>
#include <chrono>
#include <fstream>
#include <thread>

DEFINE_TYPE(BSML, GifTestPage);
BSML_DATACACHE(test_a_gif) { return ArrayW<uint8_t>(std::span(aBytes)); }
BSML_DATACACHE(test_b_gif) { return ArrayW<uint8_t>(std::span(bBytes)); }
BSML_DATACACHE(test_cancel_gif) { return ArrayW<uint8_t>(std::span(bBytes)); }
BSML_DATACACHE(test_bad_gif) { return ArrayW<uint8_t>(std::span(badBytes)); }
BSML_DATACACHE(test_stream_gif) { return ArrayW<uint8_t>(std::span(alphaBytes)); }

namespace {
    using namespace BSML;
    using custom_types::Helpers::Coroutine;
    constexpr char markup[] = {
#embed "GifTestPage.bsml" suffix(,0)
    };
    struct Result {
        std::atomic<int> success{0}, error{0};
        std::atomic<bool> valid{true};
        std::atomic<Utilities::ImageLoadError> errorType{Utilities::ImageLoadError::None};
        std::thread::id mainThread = std::this_thread::get_id();
        bool Done() const { return success + error > 0; }
        bool Success() const { return success == 1 && error == 0 && valid; }
        void CheckThread() { if (std::this_thread::get_id() != mainThread) valid = false; }
    };
    struct Run {
        safe_ptr<GifTestPage*, true> page;
        int serial;
        int failures = 0;
        bool Current() const { return page && page->runSerial == serial; }
        void Check(bool passed, std::string name) {
            if (!Current()) return;
            if (!passed) ++failures;
            page->Log((passed ? "PASS " : "FAIL ") + name);
        }
    };
    float Now() { return UnityEngine::Time::get_realtimeSinceStartup(); }
    Coroutine Wait(std::shared_ptr<Run> run, std::shared_ptr<Result> result, float seconds = 10) {
        auto end = Now() + seconds;
        while (run->Current() && !result->Done() && Now() < end) co_yield nullptr;
    }
    Coroutine Pause(std::shared_ptr<Run> run, float seconds) {
        auto end = Now() + seconds;
        while (run->Current() && Now() < end) co_yield nullptr;
    }
    auto Load(UnityEngine::UI::Image* target, std::string path, bool loading) {
        auto result = std::make_shared<Result>();
        Utilities::SetImage(target, path, loading, {}, true,
            [result] { result->CheckThread(); ++result->success; },
            [result](auto error) { result->CheckThread(); result->errorType = error; ++result->error; });
        return result;
    }
    bool CheckFrame(const std::shared_ptr<FrameInfo>& frame, const GifFixture& fixture, size_t index) {
        if (!frame || index >= fixture.hashes.size() || frame->width != fixture.width ||
            frame->height != fixture.height || frame->bpp != 4 || frame->delay != fixture.delays[index]) return false;
        uint32_t hash = 2166136261u;
        auto pixels = frame->colors.ptr();
        for (size_t j = 0; j < pixels.size(); ++j) {
            uint8_t value = pixels[j];
            if (j % 4 != 3 && pixels[j / 4 * 4 + 3] == 0) value = 0;
            hash = (hash ^ value) * 16777619u;
        }
        return hash == fixture.hashes[index];
    }
    bool CheckFrames(AnimationInfo* info, const GifFixture& fixture) {
        if (!info || info->width != fixture.width || info->height != fixture.height ||
            info->frameCount != fixture.hashes.size() || info->decodedFrames != fixture.hashes.size()) return false;
        for (size_t i = 0; i < fixture.hashes.size(); ++i) {
            if (!CheckFrame(info->PopNextFrame(), fixture, i)) return false;
        }
        return !info->PopNextFrame();
    }
    std::string Url(std::string file, bool slow = false) {
        // Every request is cold, even when the suite is repeated.
        static uint64_t nonce = 0;
        return "http://127.0.0.1:8765/" + std::string(slow ? "slow/" : "") + file + "?run=" + std::to_string(++nonce);
    }
    struct StreamObservation { std::weak_ptr<BSML::detail::GifStream> lifetime; };
    Coroutine ConsumeStream(std::shared_ptr<Run> run, std::shared_ptr<Result> result,
        GifFixture fixture, std::shared_ptr<BSML::detail::GifStream> stream,
        std::shared_ptr<StreamObservation> observation, bool abandon) {
        result->CheckThread();
        observation->lifetime = stream;
        if (stream->info->width != fixture.width || stream->info->height != fixture.height ||
            stream->info->frameCount != fixture.hashes.size()) result->valid = false;
        if (fixture.hashes.size() > BSML::detail::GifStream::Capacity) {
            auto end = Now() + 5;
            while (run->Current() && stream->info->decodedFrames < BSML::detail::GifStream::Capacity && Now() < end)
                co_yield nullptr;
            // Deliberately leave the producer blocked, including through GC.
            System::GC::Collect();
            co_yield custom_types::Helpers::CoroutineHelper::New(Pause(run, 0.2f));
            run->Check(stream->info->decodedFrames == BSML::detail::GifStream::Capacity &&
                stream->GetState() == BSML::detail::GifStream::State::Decoding,
                abandon ? "stream cancellation starts with producer blocked" : "stream stays bounded at two queued frames during GC");
        }
        if (!run->Current()) co_return;
        if (abandon) { ++result->success; co_return; }
        size_t index = 0;
        auto end = Now() + 5;
        while (run->Current() && Now() < end) {
            auto next = stream->Read();
            if (next.state == BSML::detail::GifStream::State::Failed || next.state == BSML::detail::GifStream::State::Cancelled) {
                ++result->error;
                co_return;
            }
            if (next.frame) {
                if (!CheckFrame(next.frame, fixture, index++)) result->valid = false;
                next.frame.reset();
                System::GC::Collect();
            } else if (next.state == BSML::detail::GifStream::State::Completed) {
                if (index != fixture.hashes.size()) result->valid = false;
                ++result->success;
                co_return;
            }
            co_yield nullptr; // Consume slowly enough to exercise backpressure.
        }
        result->valid = false;
        ++result->error;
    }
    Coroutine Streaming(std::shared_ptr<Run> run) {
        for (const auto& fixture : gifFixtures) {
            if (!run->Current()) co_return;
            auto result = std::make_shared<Result>();
            auto observation = std::make_shared<StreamObservation>();
            SharedCoroutineStarter::StartCoroutine(BSML::detail::ProcessGifStreaming(ArrayW<uint8_t>(fixture.bytes),
                [run, result, fixture, observation](auto stream) {
                    return ConsumeStream(run, result, fixture, stream, observation, false);
                }, [result] { result->CheckThread(); ++result->error; }));
            co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, result));
            if (!run->Current()) co_return;
            co_yield nullptr;
            run->Check(fixture.width ? result->Success() : result->error == 1 && result->success == 0 && result->valid,
                std::string("stream pixels / timing / completion: ") + fixture.name);
        }
        const auto fixture = gifFixtures[3]; // Four-frame alpha/disposal fixture.
        auto result = std::make_shared<Result>();
        auto observation = std::make_shared<StreamObservation>();
        SharedCoroutineStarter::StartCoroutine(BSML::detail::ProcessGifStreaming(ArrayW<uint8_t>(fixture.bytes),
            [run, result, fixture, observation](auto stream) {
                return ConsumeStream(run, result, fixture, stream, observation, true);
            }, [result] { result->CheckThread(); ++result->error; }));
        co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, result));
        auto end = Now() + 5;
        do {
            System::GC::Collect();
            co_yield nullptr;
        } while (run->Current() && !observation->lifetime.expired() && Now() < end);
        if (!run->Current()) co_return;
        run->Check(result->Success() && observation->lifetime.expired(), "stream consumer exit wakes producer and releases shared state");

        // Exercise the actual texture/atlas loader with more than two frames.
        run->page->ResetPreview();
        auto loaded = Load(run->page->preview, MOD_ID "_test_stream_gif", false);
        co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, loaded));
        if (!run->Current()) co_return;
        auto updater = run->page->preview->GetComponent<AnimationStateUpdater*>();
        auto data = updater ? updater->get_controllerData() : nullptr;
        run->Check(loaded->Success() && data && data->sprites.size() == fixture.hashes.size() &&
            run->page->preview->get_sprite().ptr() == data->sprites[data->uvIndex], "streamed four-frame GIF builds atlas and displays current frame");
        run->page->ResetPreview();
        run->page->Log("Streaming finished: " + std::to_string(run->failures) + " failures.");
    }
    Coroutine Offline(std::shared_ptr<Run> run) {
        // Decode directly so cache hits cannot hide malformed-input regressions.
        for (const auto& fixture : gifFixtures) {
            if (!run->Current()) co_return;
            auto result = std::make_shared<Result>();
            SharedCoroutineStarter::StartCoroutine(GifDecoder::Process(ArrayW<uint8_t>(fixture.bytes),
                [result, fixture](AnimationInfo* info) {
                    std::unique_ptr<AnimationInfo> owner(info);
                    result->CheckThread();
                    if (!CheckFrames(info, fixture)) result->valid = false;
                    ++result->success;
                }, [result] { result->CheckThread(); ++result->error; }));
            co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, result));
            if (!run->Current()) co_return;
            // Observe an extra tick for accidental double completion.
            co_yield nullptr;
            run->Check(fixture.width ? result->Success() : result->success == 0 && result->error == 1 && result->valid,
                std::string("decode / pixels / timing: ") + fixture.name + (result->Done() ? "" : " (TIMEOUT)"));
        }
        if (!run->Current()) co_return;
        run->page->ResetPreview();
        auto a = Load(run->page->preview, MOD_ID "_test_a_gif", true);
        co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, a));
        if (!run->Current()) co_return;
        run->Check(a->Success(), "valid GIF after corrupt inputs");
        if (!a->Success()) co_return;
        safe_ptr<AnimationControllerData*> oldData = run->page->preview->GetComponent<AnimationStateUpdater*>()->get_controllerData();
        auto bad = Load(run->page->preview, MOD_ID "_test_bad_gif", true);
        co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, bad));
        if (!run->Current()) co_return;
        run->Check(bad->error == 1 && bad->success == 0 && bad->valid &&
            bad->errorType == Utilities::ImageLoadError::GifParsingError &&
            run->page->preview->GetComponent<AnimationStateUpdater*>()->get_controllerData() == oldData.ptr() &&
            run->page->preview->get_sprite(), "corrupt replacement preserves A, reports once");
        auto stale = Load(run->page->preview, MOD_ID "_test_cancel_gif", true);
        auto newest = Load(run->page->preview, MOD_ID "_test_a_gif", false);
        co_yield custom_types::Helpers::CoroutineHelper::New(Pause(run, 3));
        if (!run->Current()) co_return;
        run->Check(!stale->Done() && newest->Success() && run->page->preview->GetComponent<AnimationStateUpdater*>()->get_controllerData() == oldData.ptr(),
            "newest request remains selected");
        // Keep A in use so the inactive-view check deliberately hits the shared cache.
        auto extra = Lite::CreateImage(run->page->fixtureView->contentObject->get_transform(), nullptr, {0, 0}, {1, 1});
        Load(extra, MOD_ID "_test_a_gif", false);
        auto b = Load(run->page->preview, MOD_ID "_test_b_gif", true);
        bool heldOld = true;
        auto end = Now() + 10;
        while (run->Current() && !b->Done() && Now() < end) {
            auto updater = run->page->preview->GetComponent<AnimationStateUpdater*>();
            heldOld &= updater->get_controllerData() == oldData.ptr() && run->page->preview->get_sprite();
            co_yield nullptr;
        }
        if (!run->Current()) co_return;
        auto updater = run->page->preview->GetComponent<AnimationStateUpdater*>();
        auto data = updater->get_controllerData();
        run->Check(b->Success() && heldOld && data && run->page->preview->get_sprite().ptr() == data->sprites[data->uvIndex],
            "replacement holds A, assigns current B frame immediately");
        run->page->preview->get_gameObject()->SetActive(false);
        auto inactive = Load(run->page->preview, MOD_ID "_test_a_gif", false);
        run->page->preview->get_gameObject()->SetActive(true);
        run->Check(inactive->Success() && updater->get_controllerData() == oldData.ptr() &&
            run->page->preview->get_sprite().ptr() == oldData->sprites[oldData->uvIndex], "inactive cached assignment and re-enable");
        safe_ptr<UnityEngine::UI::Image*, true> destroyedOnlyImage = extra;
        UnityEngine::Object::Destroy(extra);
        co_yield nullptr;
        if (!run->Current()) co_return;
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        oldData->CheckFrame(now + 2000);
        run->Check(!destroyedOnlyImage && !oldData->get_activeImages()->Contains(destroyedOnlyImage.ptr()),
            "playback removes a destroyed Image component");
        auto destroyed = Load(run->page->preview, MOD_ID "_test_cancel_gif", true);
        safe_ptr<UnityEngine::UI::Image*, true> oldImage = run->page->preview;
        run->page->ResetPreview();
        co_yield custom_types::Helpers::CoroutineHelper::New(Pause(run, 3));
        if (!run->Current()) co_return;
        AnimationControllerData* unused = nullptr;
        run->Check(!oldImage && !destroyed->Done() &&
            !AnimationController::get_instance()->TryGetAnimationControllerData(MOD_ID "_test_cancel_gif", unused),
            "ClearContents during decode: no callback or stale cache entry");
        run->page->Log("Offline finished: " + std::to_string(run->failures) + " failures. Check flashes visually too.");
    }
    Coroutine Network(std::shared_ptr<Run> run) {
        run->page->ResetPreview();
        auto baseline = Load(run->page->preview, Url("a.gif"), false);
        co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, baseline));
        if (!run->Current()) co_return;
        run->Check(baseline->Success(), "fixture server reachable / GIF A loaded");
        if (!baseline->Success()) {
            run->page->Log("STOP: start fixture server and adb reverse; network cases not run.");
            co_return;
        }
        for (const char* file : {"b.gif", "static.png"}) {
            for (int mode = 0; mode < 4; ++mode) {
                if (!run->Current()) co_return;
                run->page->ResetPreview();
                auto stale = Load(run->page->preview, Url(file, true), true);
                co_yield custom_types::Helpers::CoroutineHelper::New(Pause(run, 0.1f));
                if (!run->Current()) co_return;
                safe_ptr<UnityEngine::UI::Image*, true> oldImage = run->page->preview;
                if (mode == 0) {
                    auto replacement = Load(run->page->preview, MOD_ID "_test_a_gif", false);
                    co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, replacement));
                    if (!run->Current()) co_return;
                    run->Check(replacement->Success(), "replacement completed");
                } else if (mode == 1) {
                    run->page->ResetPreview(); // Calls the real ClearContents path.
                } else if (mode == 2) {
                    UnityEngine::Object::Destroy(run->page->preview);
                } else {
                    UnityEngine::Object::Destroy(run->page->preview->GetComponent<AnimationStateUpdater*>());
                }
                co_yield custom_types::Helpers::CoroutineHelper::New(Pause(run, 3));
                if (!run->Current()) co_return;
                run->Check(!stale->Done() && (mode != 1 && mode != 2 || !oldImage),
                    std::string("cancel ") + file + " / " + std::to_string(mode) + " (replace, rebuild, image, updater)");
            }
        }
        if (!run->Current()) co_return;
        run->page->ResetPreview();
        run->page->Log("Network finished: " + std::to_string(run->failures) + " failures.");
    }
    Coroutine Reentry(std::shared_ptr<Run> run) {
        run->page->ResetPreview();
        auto path = Url("b.gif", true);
        auto old = Load(run->page->preview, path, true);
        co_yield custom_types::Helpers::CoroutineHelper::New(Pause(run, 0.1f));
        if (!run->Current()) co_return;
        auto newest = Load(run->page->preview, path, true);
        co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, newest));
        if (!run->Current()) co_return;
        run->Check(!old->Done() && newest->Success(), "same URL: only newest generation completes");

        struct Followup { std::shared_ptr<Result> result; int first = 0; };
        auto followup = std::make_shared<Followup>();
        Utilities::SetImage(run->page->preview, Url("static.png"), false, {}, true,
            [run, followup] {
                ++followup->first;
                if (run->Current()) followup->result = Load(run->page->preview, Url("b.gif", true), true);
            }, [followup](auto) { followup->first = -1; });
        auto end = Now() + 10;
        while (run->Current() && !followup->result && followup->first >= 0 && Now() < end) co_yield nullptr;
        if (!run->Current()) co_return;
        if (followup->result) co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, followup->result));
        if (!run->Current()) co_return;
        run->Check(followup->first == 1 && followup->result && followup->result->Success(),
            "download completion re-entry preserves the new download");

        auto invalid = Load(run->page->preview, MOD_ID "_test_bad_gif", true);
        auto a = Load(run->page->preview, MOD_ID "_test_a_gif", false);
        co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, a));
        co_yield custom_types::Helpers::CoroutineHelper::New(Pause(run, 0.5f));
        if (!run->Current()) co_return;
        run->Check(!invalid->Done() && a->Success(), "obsolete corrupt-GIF error is suppressed");
        if (!a->Success()) co_return;

        auto peer = Lite::CreateImage(run->page->fixtureView->contentObject->get_transform(), nullptr, {0, 0}, {1, 1});
        auto warm = Load(peer, MOD_ID "_test_b_gif", false);
        co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, warm));
        if (!run->Current()) co_return;
        run->Check(warm->Success(), "dirty callback test warmed B");
        if (!warm->Success()) co_return;
        auto callback = std::make_shared<Followup>();
        safe_ptr<UnityEngine::Events::UnityAction*> action = MakeUnityAction([run, callback] {
            if (callback->first++ == 0 && run->Current())
                callback->result = Load(run->page->preview, MOD_ID "_test_a_gif", false);
        });
        run->page->preview->RegisterDirtyVerticesCallback(action.ptr());
        auto replaced = Load(run->page->preview, MOD_ID "_test_b_gif", false);
        run->page->preview->UnregisterDirtyVerticesCallback(action.ptr());
        run->Check(callback->first > 0 && callback->result && callback->result->Success() && !replaced->Done(),
            "dirty-vertices re-entry suppresses superseded GIF completion");
        run->page->ResetPreview();
        run->page->Log("Reentry finished: " + std::to_string(run->failures) + " failures.");
    }
    void Diagnostic(std::string text) {
        INFO("[GIF TEST] {}", text);
        std::ofstream("/sdcard/ModData/com.beatgames.beatsaber/bsml-gif-tests.log", std::ios::app) << text << '\n';
    }
    void ResourceStats() {
        Diagnostic("RESOURCES textures=" + std::to_string(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Texture2D*>().size()) +
            " sprites=" + std::to_string(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>().size()) +
            " images=" + std::to_string(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::UI::Image*>().size()) +
            " updaters=" + std::to_string(UnityEngine::Resources::FindObjectsOfTypeAll<AnimationStateUpdater*>().size()));
    }
    // Test-build only. ADB writes one command to this file; the Unity thread
    // consumes it once. No socket, shell execution, or production-build hook.
    struct IndexedResult {
        safe_ptr<UnityEngine::Texture2D*, true> texture;
        std::unique_ptr<BSML::detail::IndexedAnimation> animation;
        std::shared_ptr<AnimationInfo> reference;
        ~IndexedResult() { if (texture) UnityEngine::Object::DestroyImmediate(texture.ptr()); }
    };

    bool CompareExpanded(UnityEngine::Texture2D* texture, const std::shared_ptr<FrameInfo>& frame) {
        if (!texture || !frame) return false;
        using namespace UnityEngine;
        struct Readback {
            safe_ptr<RenderTexture*, true> target;
            safe_ptr<Texture2D*, true> pixels;
            safe_ptr<RenderTexture*, true> previous{RenderTexture::get_active().unsafe_ptr()};
            ~Readback() {
                RenderTexture::set_active(previous.ptr());
                if (target) { target->Release(); Object::DestroyImmediate(target.ptr()); }
                if (pixels) Object::DestroyImmediate(pixels.ptr());
            }
        } readback;
        readback.target = RenderTexture::New_ctor(frame->width, frame->height, 0, texture->get_graphicsFormat());
        if (!readback.target->Create()) return false;
        readback.pixels = Texture2D::New_ctor(frame->width, frame->height, TextureFormat::RGBA32, false, false);
        Graphics::CopyTexture(texture, 0, 0, readback.target.ptr(), 0, 0);
        RenderTexture::set_active(readback.target.ptr());
        readback.pixels->ReadPixels(Rect(0, 0, frame->width, frame->height), 0, 0, false);
        auto actual = readback.pixels->GetPixels32();
        auto expected = frame->colors.ptr();
        if (actual.size() * 4 != expected.size()) return false;
        for (int p = 0; p < actual.size(); ++p) {
            auto c = actual[p];
            if (std::abs(int(c.r) - expected[p * 4]) > 1 ||
                std::abs(int(c.g) - expected[p * 4 + 1]) > 1 ||
                std::abs(int(c.b) - expected[p * 4 + 2]) > 1 || c.a != expected[p * 4 + 3]) {
                ERROR("Indexed pixel mismatch at {}: actual {}/{}/{}/{} expected {}/{}/{}/{}", p,
                    c.r, c.g, c.b, c.a, expected[p * 4], expected[p * 4 + 1], expected[p * 4 + 2], expected[p * 4 + 3]);
                return false;
            }
        }
        return true;
    }

    Coroutine Indexed(std::shared_ptr<Run> run) {
        for (int f : {0, 1, 3, 2}) {
            if (!run->Current()) co_return;
            auto fixture = gifFixtures[f];
            auto state = std::make_shared<IndexedResult>();
            auto decoded = std::make_shared<Result>();
            SharedCoroutineStarter::StartCoroutine(GifDecoder::Process(ArrayW<uint8_t>(fixture.bytes),
                [state, decoded](AnimationInfo* info) { state->reference.reset(info); ++decoded->success; },
                [decoded] { ++decoded->error; }));
            co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, decoded));
            if (!run->Current()) co_return;
            auto loaded = std::make_shared<Result>();
            BSML::detail::ProcessImageAnimation(AnimationLoader::AnimationType::GIF, ArrayW<uint8_t>(fixture.bytes),
                [state, loaded](auto texture, auto, auto, auto animation) {
                    state->texture = texture;
                    state->animation = std::move(animation);
                    ++loaded->success;
                }, [loaded] { ++loaded->error; });
            co_yield custom_types::Helpers::CoroutineHelper::New(Wait(run, loaded));
            if (!run->Current()) co_return;
            bool expectedIndexed = f != 2;
            bool valid = decoded->Success() && loaded->Success() && state->texture &&
                bool(state->animation) == expectedIndexed;
            run->Check(valid, std::string(expectedIndexed ? "indexed GPU path selected: " : "small GIF RGBA fallback: ") + fixture.name);
            if (valid && expectedIndexed) {
                int compared = 0;
                for (int i = 0; i < fixture.hashes.size(); ++i) {
                    if (!run->Current()) co_return;
                    auto frame = state->reference->PopNextFrame();
                    if (!state->animation->Expand(i) || !CompareExpanded(state->texture.ptr(), frame)) { valid = false; break; }
                    ++compared;
                    co_yield nullptr;
                }
                run->Check(valid, std::string("GPU pixels / alpha / orientation: ") + fixture.name + " (" + std::to_string(compared) + " frames)");
            }
        }
        if (run->Current()) run->page->Log("Indexed finished: " + std::to_string(run->failures) + " failures.");
    }

    Coroutine Commands() {
        constexpr auto path = "/sdcard/ModData/com.beatgames.beatsaber/bsml-gif-tests.command";
        Diagnostic("Command listener ready.");
        while (true) {
            auto next = Now() + 0.5f;
            while (Now() < next) co_yield nullptr;
            std::ifstream input(path);
            std::string command;
            if (!(input >> command)) continue;
            input.close();
            auto registration = MainMenuRegistration::get_registration("GIF Tests");
            if (command == "open") {
                auto menus = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::MainMenuViewController*>();
                bool ready = false;
                for (auto menu : menus) if (menu && menu->m_CachedPtr.m_value && menu->get_isActiveAndEnabled()) ready = true;
                if (!ready) continue;
            }
            std::remove(path);
            Diagnostic("COMMAND " + command);
            try {
                if (command == "stats") { ResourceStats(); continue; }
                if (command == "gc") { System::GC::Collect(); continue; }
                if (command == "open") { registration->Present(); continue; }
                auto page = reinterpret_cast<GifTestPage*>(registration->viewController);
                if (!page || !page->m_CachedPtr.m_value || !page->fixtureView) {
                    Diagnostic("COMMAND FAILED: open the GIF Tests page first.");
                    continue;
                }
                if (command == "streaming") {
                    page->Stop();
                    page->Log("Streaming suite started.");
                    SharedCoroutineStarter::StartCoroutine(Streaming(std::make_shared<Run>(Run{page, page->runSerial})));
                }
                else if (command == "indexed") {
                    page->Stop();
                    page->Log("Indexed suite started.");
                    SharedCoroutineStarter::StartCoroutine(Indexed(std::make_shared<Run>(Run{page, page->runSerial})));
                }
                else if (command == "reentry") {
                    page->Stop();
                    page->Log("Reentry suite started.");
                    SharedCoroutineStarter::StartCoroutine(Reentry(std::make_shared<Run>(Run{page, page->runSerial})));
                }
                else if (command == "offline") page->RunOffline();
                else if (command == "network") page->RunNetwork();
                else if (command == "a") page->ShowA();
                else if (command == "b") page->ShowB();
                else if (command == "slow-b") page->ShowSlowB();
                else if (command == "bad") page->ShowBad();
                else if (command == "loading") page->ToggleLoading();
                else if (command == "rebuild") page->Rebuild();
                else if (command == "active") page->ToggleActive();
                else if (command == "stop") page->Stop();
                else Diagnostic("COMMAND FAILED: unknown command.");
            } catch (const std::exception& error) {
                Diagnostic(std::string("COMMAND FAILED: ") + error.what());
            }
        }
    }

}

namespace BSML {
    void GifTestPage::Log(std::string text) {
        Diagnostic(text);
        if (report && report->m_CachedPtr.m_value) {
            std::string history = report->get_text();
            history += "\n" + text;
            while (std::count(history.begin(), history.end(), '\n') > 5) history.erase(0, history.find('\n') + 1);
            report->set_text(history);
        }
    }
    void GifTestPage::ResetPreview() {
        fixtureView->ClearContents();
        preview = Lite::CreateImage(fixtureView->contentObject->get_transform(), nullptr, {0, 0}, {35, 26});
        preview->set_preserveAspect(true);
    }
    void GifTestPage::DidActivate(bool firstActivation, bool, bool) {
        if (!firstActivation) return;
        loading = true;
        parse_and_construct(markup, get_transform(), this);
        fixtureView = Helpers::CreateViewController<BSMLViewController*>();
        auto go = fixtureView->get_gameObject();
        go->set_name("GIF test content");
        auto rect = fixtureView->get_rectTransform();
        rect->SetParent(previewRoot->get_transform(), false);
        rect->set_anchorMin({0.5f, 0.5f});
        rect->set_anchorMax({0.5f, 0.5f});
        rect->set_anchoredPosition({0, 0});
        rect->set_sizeDelta({35, 26});
        ResetPreview();
        go->SetActive(true);
        Log("PAGE READY");
    }
    void GifTestPage::DidDeactivate(bool, bool) { Stop(); }
    void GifTestPage::Stop() {
        ++runSerial;
        if (preview && preview->m_CachedPtr.m_value) {
            if (auto updater = preview->GetComponent<AnimationStateUpdater*>()) updater->CancelImageLoad();
        }
        Log("Stopped / pending requests cancelled.");
    }
    void GifTestPage::RunOffline() {
        Stop(); Log("Offline suite started.");
        SharedCoroutineStarter::StartCoroutine(Offline(std::make_shared<Run>(Run{this, runSerial})));
    }
    void GifTestPage::RunNetwork() {
        Stop(); Log("Network suite started (about 30 seconds).");
        SharedCoroutineStarter::StartCoroutine(Network(std::make_shared<Run>(Run{this, runSerial})));
    }
    void GifTestPage::Show(std::string path) {
        Stop();
        if (!preview || !preview->m_CachedPtr.m_value) ResetPreview();
        safe_ptr<GifTestPage*, true> self = this;
        auto serial = runSerial;
        Utilities::SetImage(preview, path, loading, {}, true,
            [self, serial] { if (self && self->runSerial == serial) self->Log("Image ready."); },
            [self, serial](auto error) { if (self && self->runSerial == serial) self->Log("Image error: " + std::to_string(int(error))); });
        Log("Loading " + path);
    }
    void GifTestPage::ShowA() { Show(MOD_ID "_test_a_gif"); }
    void GifTestPage::ShowB() { Show(MOD_ID "_test_b_gif"); }
    void GifTestPage::ShowSlowB() { Show(Url("b.gif", true)); }
    void GifTestPage::ShowBad() { Show(MOD_ID "_test_bad_gif"); }
    void GifTestPage::Rebuild() { Stop(); ResetPreview(); Log("ClearContents rebuilt the target."); }
    void GifTestPage::ToggleLoading() { loading = !loading; Log(loading ? "Loading animation ON" : "Loading animation OFF"); }
    void GifTestPage::ToggleActive() {
        Stop();
        if (preview && preview->m_CachedPtr.m_value) {
            auto go = preview->get_gameObject(); go->SetActive(!go->get_activeSelf());
        }
    }
    void GifTestPage::NoFlash() { Log("MANUAL PASS: no flash observed."); }
    void GifTestPage::SawFlash() { Log("MANUAL FAIL: flash observed."); }
    namespace QuestTests {
        void RegisterGifTests() {
            Register::RegisterMainMenu<GifTestPage*>("GIF Tests", "GIF Tests", "On-device image regression tests");
            SharedCoroutineStarter::StartCoroutine(Commands());
        }
    }
}

