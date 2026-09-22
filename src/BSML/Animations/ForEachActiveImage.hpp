#pragma once

namespace BSML::detail {
    // Walk the live list backwards without retaining its backing array across
    // callbacks. Removals may shift an already visited image down; assigning the
    // same sprite again is harmless. Appends do not extend this bounded pass:
    // AnimationStateUpdater::OnEnable already applies the current frame.
    template<class Images, class Apply>
    void ForEachActiveImage(Images images, Apply&& apply) {
        for (int index = static_cast<int>(images.size()); index > 0;) {
            const int count = static_cast<int>(images.size());
            if (index > count) index = count;
            if (index == 0) break;

            auto image = images[--index];
            if (!image || !image->m_CachedPtr.m_value) {
                images->RemoveAt(index);
                continue;
            }
            apply(image);
        }
    }
}
