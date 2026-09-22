#pragma once

#include "GifStream.hpp"
#include "custom-types/shared/coroutine.hpp"

namespace BSML::detail {
    custom_types::Helpers::Coroutine ProcessGifStreaming(
        ArrayW<uint8_t> data,
        std::function<custom_types::Helpers::Coroutine(std::shared_ptr<GifStream>)> consume,
        std::function<void()> onError);
}
