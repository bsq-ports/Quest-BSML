#include "ForEachActiveImage.hpp"
#include "BSML/Animations/AnimationStateUpdater.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <new>

namespace {
    size_t allocations = 0;
}

void* operator new(size_t size) {
    if (auto result = std::malloc(size ? size : 1)) {
        ++allocations;
        return result;
    }
    throw std::bad_alloc();
}
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, size_t) noexcept { std::free(pointer); }
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer, size_t) noexcept { std::free(pointer); }

using UnityEngine::UI::Image;
using UnityEngine::Sprite;
using BSML::detail::ForEachActiveImage;

// Simulate ListW's borrowed list handle and replaceable backing array. Bounds
// assertions catch reads through an index invalidated by a setter callback.
struct ImageList {
    std::vector<Image*> values;
    size_t reads = 0;
    void RemoveAt(int index) {
        assert(index >= 0 && index < static_cast<int>(values.size()));
        values.erase(values.begin() + index);
    }
    void Remove(Image* image) {
        auto it = std::find(values.begin(), values.end(), image);
        assert(it != values.end());
        values.erase(it);
    }
};
struct ListView {
    ImageList* list;
    size_t size() const { return list->values.size(); }
    Image* operator[](int index) const {
        ++list->reads;
        return list->values.at(index);
    }
    ImageList* operator->() const { return list; }
};

void Apply(ImageList& list, Sprite* sprite) {
    ForEachActiveImage(ListView{&list}, [sprite](Image* image) { image->set_sprite(sprite); });
}

void NoAllocationsAndLinearWork() {
    std::array<Image, 256> images;
    std::array<Sprite, 2> frames;
    ImageList list;
    for (auto& image : images) list.values.push_back(&image);
    const auto before = allocations;
    for (int frame = 0; frame < 1000; ++frame) Apply(list, &frames[frame % 2]);
    assert(allocations == before);
    assert(list.reads == 1000 * images.size());
    for (auto& image : images) assert(image.sprite == &frames[1]);
    list.values.clear();
    Apply(list, &frames[0]);
    assert(allocations == before);
}

void PruneDestroyedImages() {
    Image first, second, dead;
    dead.m_CachedPtr.m_value = 0;
    Sprite frame;
    ImageList list{{nullptr, &first, &dead, &second, nullptr, &dead}};
    Apply(list, &frame);
    assert((list.values == std::vector<Image*>{&first, &second}));
    assert(first.sprite == &frame && second.sprite == &frame);
    assert(dead.sprite == nullptr);
}

void RemovalDuringSetter() {
    // Remove self, an earlier image, or an already visited image. A replacement
    // sprite on a detached target must survive the rest of the old pass.
    for (int source = 0; source < 4; ++source) {
        for (int target = 0; target < 4; ++target) {
            std::array<Image, 4> images;
            Sprite frame, replacement;
            ImageList list;
            for (auto& image : images) list.values.push_back(&image);
            bool fired = false;
            images[source].dirty = [&] {
                if (fired) return;
                fired = true;
                list.Remove(&images[target]);
                images[target].sprite = &replacement;
            };
            Apply(list, &frame);
            assert(fired && images[target].sprite == &replacement);
            for (auto image : list.values) assert(image->sprite == &frame);
            assert(list.reads <= images.size());
        }
    }
}

void ClearShrinkAndDestroyDuringSetter() {
    for (int mode = 0; mode < 3; ++mode) {
        std::array<Image, 5> images;
        Sprite frame;
        ImageList list;
        for (auto& image : images) list.values.push_back(&image);
        images.back().dirty = [&] {
            if (mode == 0) list.values.clear();
            else if (mode == 1) list.values.resize(1);
            else images[2].m_CachedPtr.m_value = 0;
        };
        Apply(list, &frame);
        if (mode == 0) assert(images[0].sprite == nullptr && list.values.empty());
        if (mode == 1) assert(images[0].sprite == &frame && list.values.size() == 1);
        if (mode == 2) assert(images[2].sprite == nullptr && list.values.size() == 4);
        assert(list.reads <= images.size());
    }
}

void AppendAndNestedPass() {
    for (bool nested : {false, true}) {
        std::array<Image, 4> images;
        std::array<Image, 128> added;
        Sprite frame;
        ImageList list;
        for (auto& image : images) list.values.push_back(&image);
        bool fired = false;
        images.back().dirty = [&] {
            if (fired) return;
            fired = true;
            if (nested) {
                list.Remove(&images.back());
                Apply(list, &frame);
            } else {
                for (auto& image : added) {
                    list.values.push_back(&image); // Forces backing-array growth.
                    image.set_sprite(&frame); // OnEnable applies the frame now.
                }
            }
        };
        Apply(list, &frame);
        assert(fired);
        for (auto image : list.values) assert(image->sprite == &frame);
        if (!nested) assert(list.reads == images.size());
    }
}

int main() {
    NoAllocationsAndLinearWork();
    PruneDestroyedImages();
    RemovalDuringSetter();
    ClearShrinkAndDestroyDuringSetter();
    AppendAndNestedPass();
    std::cout << "PASS: allocation-free linear playback, pruning, mutation, and reentry\n";
}
