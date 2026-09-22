#include "Helpers/utilities.hpp"
#include "ImageLoadRequest.hpp"
#include "BSMLDataCache_internal.hpp"
#include "logging.hpp"

#include "BSML/SharedCoroutineStarter.hpp"
#include "BSML/MainThreadScheduler.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Vector4.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/SpriteMeshType.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Texture.hpp"
#include "UnityEngine/TextureFormat.hpp"
#include "UnityEngine/FilterMode.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/Graphics.hpp"
#include "UnityEngine/RenderTexture.hpp"
#include "UnityEngine/RenderTextureFormat.hpp"
#include "UnityEngine/RenderTextureReadWrite.hpp"
#include "UnityEngine/TextureWrapMode.hpp"
#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "UnityEngine/Networking/DownloadHandler.hpp"
#include "UnityEngine/Networking/UnityWebRequestAsyncOperation.hpp"

#include "BSML/Animations/AnimationStateUpdater.hpp"
#include "BSML/Animations/AnimationController.hpp"
#include "BSML/Animations/AnimationControllerData.hpp"
#include "BSML/Animations/AnimationLoader.hpp"

#include "csscolorparser.hpp"

#include "System/Uri.hpp"
#include "System/String.hpp"
#include "System/StringComparison.hpp"

#include "custom-types/shared/coroutine.hpp"
#include "beatsaber-hook/shared/safeptr.hpp"
#include "beatsaber-hook/shared/stringw.hpp"
#include "beatsaber-hook/shared/byref.hpp"
#include <memory>

#define coro(coroutine) BSML::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

using namespace UnityEngine;
using namespace UnityEngine::Networking;

namespace BSML::Utilities {

    template<typename T, typename U>
    using Dictionary = System::Collections::Generic::Dictionary_2<T, U>;

    safe_ptr<Dictionary<StringW, UnityEngine::Sprite*>*> spriteCache;
    Sprite* FindSpriteCached(StringW name) {
        if (!spriteCache)
            spriteCache.emplace(Dictionary<StringW, UnityEngine::Sprite*>::New_ctor());

        UnityEngine::Sprite* sprite = nullptr;

        if (spriteCache->TryGetValue(name, by_ref(sprite)) && sprite && sprite->m_CachedPtr.m_value)
            return sprite;

        for (auto x : Resources::FindObjectsOfTypeAll<Sprite*>())
        {
            if (x->name.size() == 0)
                continue;
            UnityEngine::Sprite* a = nullptr;
            if(!spriteCache->TryGetValue(x->get_name(), by_ref(a)) || !a)
                spriteCache->Add(x->get_name(), x);

            if (x->get_name() == name)
                sprite = x;
        }

        return sprite;
    }

    safe_ptr<Dictionary<StringW, UnityEngine::Texture*>*> textureCache;
    Texture* FindTextureCached(StringW name) {
        if (!textureCache)
            textureCache.emplace(Dictionary<StringW, UnityEngine::Texture*>::New_ctor());

        UnityEngine::Texture* texture = nullptr;

        if (textureCache->TryGetValue(name, by_ref(texture)) && texture && texture->m_CachedPtr.m_value)
            return texture;

        for (auto x : Resources::FindObjectsOfTypeAll<Texture*>())
        {
            if (x->name.size() == 0)
                continue;
            UnityEngine::Texture* a = nullptr;
            if(!textureCache->TryGetValue(x->get_name(), by_ref(a)) || !a)
                textureCache->Add(x->get_name(), x);

            if (x->get_name() == name)
                texture = x;
        }

        return texture;
    }

    std::optional<UnityEngine::Color> ParseHTMLColorOpt(std::string_view str) {
        std::string val{str};
        bool valid = false;
        auto color = CSSColorParser::parseRGBA(val, valid);
        if (!valid) return std::nullopt;
        return UnityEngine::Color{
            (float)color.r / 255.0f,
            (float)color.g / 255.0f,
            (float)color.b / 255.0f,
            color.a
        };
    }

    UnityEngine::Color ParseHMTMLColor(std::string_view str) {
        return ParseHTMLColorOpt(str).value_or(UnityEngine::Color{1.0, 1.0, 1.0, 1.0});
    }

    std::optional<UnityEngine::Color32> ParseHTMLColor32Opt(std::string_view str) {
        std::string val{str};
        bool valid = false;
        auto color = CSSColorParser::parse(val, valid);
        if (!valid) return std::nullopt;
        return UnityEngine::Color32{
            0,
            color.r,
            color.g,
            color.b,
            static_cast<uint8_t>(color.a * 255)
        };
    }

    UnityEngine::Color32 ParseHTMLColor32(std::string_view str) {
        return ParseHTMLColor32Opt(str).value_or(UnityEngine::Color32{0, 255, 255, 255, 255});
    }

    Texture2D* DownScaleTexture(Texture2D* tex, const ScaleOptions& options) {
        auto originalWidth = tex->get_width();
        auto originalHeight = tex->get_height();

        if (originalWidth + originalHeight <= options.width + options.height)
            return tex;

        auto newWidth = options.width;
        auto newHeight = options.height;
        if (options.maintainRatio) {
            auto ratio = (float)originalWidth / (float)originalHeight;
            auto scale = originalWidth > originalHeight ? originalWidth : originalHeight;

            if (scale * ratio <= originalWidth) {
                originalWidth = scale * ratio;
                originalHeight = scale;
            } else {
                originalWidth = scale;
                originalHeight = scale * ratio;
            }
        }

        auto rect = Rect(0, 0, newWidth, newHeight);
        auto copy = Texture2D::New_ctor(rect.get_width(), rect.get_height(), TextureFormat::RGBA32, false);
        auto currentRT = RenderTexture::get_active();
        auto renderTexture = RenderTexture::GetTemporary(rect.get_width(), rect.get_height(), 32, RenderTextureFormat::Default, RenderTextureReadWrite::Default);
        Graphics::Blit(tex, renderTexture);

        RenderTexture::set_active(renderTexture);
        copy->ReadPixels(rect, 0, 0);
        copy->Apply();
        RenderTexture::set_active(currentRT);
        RenderTexture::ReleaseTemporary(renderTexture);
        return copy;
    }

    UnityEngine::Sprite* DownScaleSprite(Sprite* sprite, const ScaleOptions& options) {
        return LoadSpriteFromTexture(DownScaleTexture(sprite->get_texture(), options));
    }

    custom_types::Helpers::Coroutine DownloadDataCoroutine(StringW uri, std::function<void(ArrayW<uint8_t>)> onFinished) {
        if (!onFinished) {
            ERROR("Can't get data async without a callback to use it with");
            co_return;
        }

        auto www = UnityWebRequest::Get(uri);
        auto req = www->SendWebRequest();
        while (!req->get_isDone()) co_yield nullptr;
        auto error = www->GetError();
        if (error != UnityEngine::Networking::UnityWebRequest::UnityWebRequestError::OK) {
            onFinished(nullptr);
            www->Dispose();
            co_return;
        };

        onFinished(www->get_downloadHandler()->GetData());
        www->Dispose();
        co_return;
    }

    void DownloadData(StringW uri, std::function<void(ArrayW<uint8_t>)> onFinished) {
        INFO("Getting data from uri: {}", uri);
        if (!onFinished) {
            ERROR("Can't get data async without a callback to use it with");
            return;
        }
        coro(DownloadDataCoroutine(uri, onFinished));
    }

    void GetData(StringW key, std::function<void(ArrayW<uint8_t>)> onFinished) {
        if (!onFinished) {
            ERROR("Can't get data from datacache without a callback to use it with");
            return;
        }
        INFO("Getting data from key: {}", key);
        auto entry = DataCache::Get(key);
        if (entry) {
            onFinished(entry->get_data());
        } else {
            ERROR("Could not find entry for key: {}", key);
            onFinished(nullptr);
        }
    }

    bool IsAnimated(StringW str)
    {
        return  str->EndsWith(".gif", System::StringComparison::OrdinalIgnoreCase) ||
                str->EndsWith("_gif", System::StringComparison::OrdinalIgnoreCase) ||
                str->EndsWith(".apng", System::StringComparison::OrdinalIgnoreCase)||
                str->EndsWith("_apng", System::StringComparison::OrdinalIgnoreCase);
    }

    void DefaultImageLoadErrorHandler(ImageLoadError err) {
        if (err != ImageLoadError::None)
            ERROR("Unhandled Load Image error {}, Use the SetImage method that takes an error handler to handle it correctly!", err);
    }

    void SetImage(UnityEngine::UI::Image* image, StringW path) {
        SetImage(image, path, true, ScaleOptions(), true, nullptr, DefaultImageLoadErrorHandler);
    }

    void SetImage(UnityEngine::UI::Image* image, StringW path, bool cached) {
        SetImage(image, path, true, ScaleOptions(), cached, nullptr, DefaultImageLoadErrorHandler);
    }

    void SetImage(UnityEngine::UI::Image* image, StringW path, bool loadingAnimation, ScaleOptions scaleOptions, std::function<void()> onFinished) {
        SetImage(image, path, loadingAnimation, scaleOptions, true, onFinished, DefaultImageLoadErrorHandler);
    }

    Dictionary<StringW, UnityEngine::Sprite*>* get_bsmlSetImageCache() {
        static safe_ptr<Dictionary<StringW, UnityEngine::Sprite*>*> bsmlSetImageCache;
        if (!bsmlSetImageCache) {
            bsmlSetImageCache = Dictionary<StringW, UnityEngine::Sprite*>::New_ctor();
        }
        return bsmlSetImageCache.ptr();
    }

    bool RemoveImage(StringW path) {
        auto cache = get_bsmlSetImageCache();
        UnityEngine::Sprite* img = nullptr;
        if (cache->TryGetValue(path, by_ref(img))) {
            cache->Remove(path);
            if (img && img->m_CachedPtr.m_value) UnityEngine::Object::DestroyImmediate(img);
            return true;
        }
        return false;
    }

    namespace {
        using detail::ImageLoadRequest;
        using detail::ImageDownload;

        custom_types::Helpers::Coroutine DownloadImageData(std::shared_ptr<ImageLoadRequest> request, StringW uri, std::function<void(ArrayW<uint8_t>)> onFinished) {
            if (!request->IsCurrent()) co_return;
            ImageDownload owner{request, UnityWebRequest::Get(uri)};
            request->stateUpdater->imageDownload = owner.download.ptr();
            safe_ptr<UnityWebRequestAsyncOperation*> operation = owner.download->SendWebRequest();
            while (!operation->get_isDone()) {
                if (!request->IsCurrent()) {
                    owner.download->Abort();
                    co_return;
                }
                co_yield nullptr;
            }
            if (!request->IsCurrent()) co_return;
            // Detach before callbacks, which can synchronously start a new request.
            request->stateUpdater->imageDownload = nullptr;
            onFinished(owner.download->GetError() == UnityWebRequest::UnityWebRequestError::OK
                ? owner.download->get_downloadHandler()->GetData() : ArrayW<uint8_t>(nullptr));
        }
    }

    void SetAndLoadImageAnimated(std::shared_ptr<ImageLoadRequest> request, std::pair<bool, System::Uri*> uri, std::function<void()> onFinished, std::function<void(ImageLoadError)> onError) {
        if (!request->IsCurrent()) return;
        safe_ptr<AnimationController*, true> animationController = AnimationController::get_instance();

        auto path = request->path.ptr();

        AnimationControllerData* animationControllerData = nullptr;
        if (animationController->TryGetAnimationControllerData(path, animationControllerData)) {
            if (request->ApplyAnimation(animationControllerData) && onFinished) onFinished();
        } else {
            bool isGif = path->EndsWith("gif", System::StringComparison::OrdinalIgnoreCase) || (uri.first && StringW(uri.second->get_LocalPath())->EndsWith("gif", System::StringComparison::OrdinalIgnoreCase));
            auto animType = isGif ? AnimationLoader::AnimationType::GIF : AnimationLoader::AnimationType::APNG;

            auto errorType = uri.first ? ImageLoadError::NetworkError : ImageLoadError::GetDataError;
            auto onDataFinished = [request, onFinished, onError, errorType, animationController, animType](ArrayW<uint8_t> data){
                if (!request->IsCurrent() || !animationController) return;
                // somehow data was failed to be gotten
                if (!data) {
                    if (onError) onError(errorType);
                    return;
                }

                AnimationLoader::Process(
                    animType,
                    data,
                    [request, onFinished, animationController](auto tex, auto uvs, auto delays){
                        // An in-flight decode may finish after cancellation. It owns
                        // its frames until completion; discard the unused atlas here.
                        if (!request->IsCurrent() || !animationController) {
                            if (tex && tex->m_CachedPtr.m_value) Object::DestroyImmediate(tex);
                            return;
                        }
                        auto controllerData = animationController->Register(request->path.ptr(), tex, uvs, delays);
                        if (request->ApplyAnimation(controllerData) && onFinished) onFinished();
                    },
                    [request, onError](){
                        // Dispatch consistently through the scheduler and recheck request ownership.
                        MainThreadScheduler::Schedule([request, onError]() {
                            if (request->IsCurrent() && onError) onError(ImageLoadError::GifParsingError);
                        });
                    }
                );
            };

            if (uri.first) {
                coro(DownloadImageData(request, uri.second->get_AbsoluteUri(), onDataFinished));
            } else {
                GetData(path, onDataFinished);
            }
        }
    }

    void SetAndLoadImageNonAnimated(std::shared_ptr<ImageLoadRequest> request, ScaleOptions scaleOptions, bool cached, std::pair<bool, System::Uri*> uri, std::function<void()> onFinished, std::function<void(ImageLoadError)> onError) {
        auto errorType = uri.first ? ImageLoadError::NetworkError : ImageLoadError::GetDataError;
        auto onDataFinished = [request, onFinished, onError, errorType, scaleOptions, cached](ArrayW<uint8_t> data) {
            if (!request->IsCurrent()) return;
            // somehow data was failed to be gotten
            if (!data) {
                if (onError) onError(errorType);
                return;
            }

            if (data.size() > 0) {
                auto texture = LoadTextureRaw(data);
                if (!texture) {
                    ERROR("Failed to load texture from data");
                    if (onError) onError(ImageLoadError::ImageParsingError);
                    return;
                }

                if (scaleOptions.shouldScale) {
                    auto scaledTexture = DownScaleTexture(texture, scaleOptions);
                    if (scaledTexture != texture) {
                        Object::DestroyImmediate(texture);
                        texture = scaledTexture;
                    }
                }

                auto sprite = LoadSpriteFromTexture(texture);
                if (!sprite) {
                    Object::DestroyImmediate(texture);
                    ERROR("Failed to load sprite from texture");
                    if (onError) onError(ImageLoadError::ImageParsingError);
                    return;
                }

                if (!request->PrepareStaticImage()) {
                    Object::DestroyImmediate(sprite);
                    Object::DestroyImmediate(texture);
                    return;
                }
                texture->set_wrapMode(TextureWrapMode::Clamp);

                request->image->set_sprite(sprite);
                // Another request may have populated this key while we loaded.
                if (cached) get_bsmlSetImageCache()->TryAdd(request->path.ptr(), sprite);
            }

            if (request->IsCurrent() && onFinished)
                onFinished();
            DEBUG("Done!");
        };

        if (uri.first) {
            coro(DownloadImageData(request, uri.second->get_AbsoluteUri(), onDataFinished));
        } else {
            GetData(request->path.ptr(), onDataFinished);
        }
    }

    void SetImage(UnityEngine::UI::Image* image, StringW path, bool loadingAnimation, ScaleOptions scaleOptions, std::function<void()> onFinished, std::function<void(ImageLoadError)> onError) {
        SetImage(image, path, loadingAnimation, scaleOptions, true, onFinished, onError);
    }

    void SetImage(UnityEngine::UI::Image* image, StringW path, bool loadingAnimation, ScaleOptions scaleOptions, bool cached, std::function<void()> onFinished, std::function<void(ImageLoadError)> onError) {
        if (!image || !image->m_CachedPtr.m_value) {
            ERROR("Can't set null image!");
            return;
        }

        auto animationController = AnimationController::get_instance();
        auto stateUpdater = image->GetComponent<AnimationStateUpdater*>();

        if (!stateUpdater || !stateUpdater->m_CachedPtr.m_value) {
            stateUpdater = image->gameObject->AddComponent<AnimationStateUpdater*>();
        }

        stateUpdater->image = image;
        // Invalidate even synchronous cache/base-game requests before any early return.
        stateUpdater->CancelImageLoad();
        auto request = std::make_shared<ImageLoadRequest>(image, stateUpdater, path);

        if (loadingAnimation && animationController->loadingAnimation && !stateUpdater->get_controllerData()) {
            if (!request->ApplyAnimation(animationController->loadingAnimation)) return;
        }
        // Retain an existing GIF through download and parsing, even when a loading
        // animation was requested. Detaching it can release the displayed atlas.

        if (path.size() > 1 && path[0] == '#') { // it's a base game sprite that is requested
            if (!request->PrepareStaticImage()) return;
            auto imgName = path->Substring(1);
            image->set_sprite(FindSpriteCached(imgName));

            if (!request->IsCurrent()) return;
            if (image->get_sprite() == nullptr)
                ERROR("Could not find base game Sprite with image name {}", imgName);
            return;
        }

        UnityEngine::Sprite* sprite = nullptr;
        if (get_bsmlSetImageCache()->TryGetValue(path, by_ref(sprite)) && sprite && sprite->m_CachedPtr.m_value) {
            // we got a sprite, use it
            if (!request->PrepareStaticImage()) return;

            image->set_sprite(sprite);
            if (request->IsCurrent() && onFinished) onFinished();
            return;
        } else if (sprite) {
            INFO("Removing {} from cache as the attached sprite was invalid", path);
            get_bsmlSetImageCache()->Remove(path);
        }

        System::Uri* uri = nullptr;
        bool isUri = System::Uri::TryCreate(path, System::UriKind::Absolute, by_ref(uri));
        // animated just means ".gif || .apng"
        // TODO: support for animated sprites in the future
        if (IsAnimated(path) || (isUri && IsAnimated(uri->get_LocalPath()))) {
            SetAndLoadImageAnimated(request, {isUri, uri}, onFinished, onError);
        } else { // not animated
            SetAndLoadImageNonAnimated(request, scaleOptions, cached, {isUri, uri}, onFinished, onError);
        }
    }



    UnityEngine::Texture2D* LoadTextureRaw(ArrayW<uint8_t> data) {
        if (data.size() > 0) {
            auto texture = Texture2D::New_ctor(1, 1, TextureFormat::RGBA32, false, false);
            if (ImageConversion::LoadImage(texture, data, false))
                return texture;
            Object::DestroyImmediate(texture);
        }
        ERROR("Failed to load texture from data");
        return nullptr;
    }

    UnityEngine::Sprite* LoadSpriteRaw(ArrayW<uint8_t> data, float pixelsPerUnit) {
        return LoadSpriteFromTexture(LoadTextureRaw(data), pixelsPerUnit);
    }

    UnityEngine::Sprite* LoadSpriteFromTexture(UnityEngine::Texture2D* texture, float pixelsPerUnit) {
        if (!texture) {
            ERROR("Invalid Texture given");
            return nullptr;
        }
        return Sprite::Create(texture, Rect(0.0f, 0.0f, texture->get_width(), texture->get_height()), Vector2(0.5f, 0.5f), pixelsPerUnit, 1u, SpriteMeshType::FullRect, Vector4(0.0f, 0.0f, 0.0f, 0.0f), false);
    }

    bool CheckIfClassIsParentClass(const Il2CppClass* klass, const Il2CppClass* possibleParent) {
        while (klass->parent) {
            if (klass->parent == possibleParent) return true;
            klass = klass->parent;
        }
        return false;
    }

    System::Object* CopyFieldsAndProperties(UnityEngine::Component* comp, UnityEngine::Component* other, Il2CppClass* klass) {
        if (!klass) return comp;
        if (!CheckIfClassIsParentClass(comp->klass, klass) && !CheckIfClassIsParentClass(other->klass, klass)) {
            return nullptr;
        }

        void* myIter = nullptr;
        const PropertyInfo* prop = nullptr;
        while((prop = i2c::functions::class_get_properties(klass, &myIter))) {
            if (prop->get && prop->set) {
                auto getter = i2c::functions::property_get_get_method(prop);
                auto setter = i2c::functions::property_get_set_method(prop);
                if ((getter->token & METHOD_ATTRIBUTE_STATIC) == METHOD_ATTRIBUTE_STATIC) continue;
                if ((setter->token & METHOD_ATTRIBUTE_STATIC) == METHOD_ATTRIBUTE_STATIC) continue;

                std::array<void*, 1> args{nullptr};
                Il2CppException* exp = nullptr;
                auto value = i2c::functions::runtime_invoke(getter, other, args.data(), &exp);
                if (exp) {
                    // handle an exception
                    ERROR("Exception: {}", StringW(exp->message));
                    continue;
                }
                args[0] = value;
                i2c::functions::runtime_invoke(setter, comp, args.data(), &exp);
                if (exp) {
                    // handle an exception
                    ERROR("Exception: {}", StringW(exp->message));
                    continue;
                }
            }
        }

        myIter = nullptr;
        ::FieldInfo* field = nullptr;
        void* value = nullptr;
        uint32_t size = 0;
        while((field = i2c::functions::class_get_fields(klass, &myIter))) {
            if ((field->token & FIELD_ATTRIBUTE_STATIC) == FIELD_ATTRIBUTE_STATIC) continue;
            auto klass = i2c::functions::Class_FromIl2CppType(const_cast<Il2CppType*>(field->type));
            size = klass->instance_size;
            value = realloc(value, size);
            i2c::functions::field_get_value(other, field, value);
            i2c::functions::field_set_value(comp, field, value);
        }
        free(value);

        return CopyFieldsAndProperties(comp, other, klass->parent);
    }

    /// based on https://answers.unity.com/questions/530178/how-to-get-a-component-from-an-object-and-add-it-t.html
    UnityEngine::Component* GetCopyOfComponent(UnityEngine::Component* comp, UnityEngine::Component* other) {
        auto klass = comp->klass;
        if (klass != other->klass) {
            ERROR("Type Mismatch!");
            return nullptr;
        }

        return reinterpret_cast<UnityEngine::Component*>(CopyFieldsAndProperties(comp, other, klass));
    }

    /// end of based on thing

    namespace ImageResources {
        safe_ptr<UnityEngine::Sprite*> blankSprite;
        UnityEngine::Sprite* GetBlankSprite() {
            if (!blankSprite) {
                auto texture = Texture2D::get_blackTexture();
                blankSprite = Sprite::Create(texture, Rect(0.0f, 0.0f, texture->get_width(), texture->get_height()), Vector2(0.5f, 0.5f), 100.0f, 1u, SpriteMeshType::FullRect, Vector4(0.0f, 0.0f, 0.0f, 0.0f), false);
                blankSprite->set_name("BlankSprite");
                Object::DontDestroyOnLoad(blankSprite.ptr());
            }
            return blankSprite.ptr();
        }

        safe_ptr<UnityEngine::Sprite*> whitePixelSprite;
        UnityEngine::Sprite* GetWhitePixel() {
            if (!whitePixelSprite) {
                whitePixelSprite = FindSpriteCached("WhitePixel");
            }
            return whitePixelSprite.ptr();
        }
    }
}
