#pragma once
#include <atomic>
#include <memory>
#include <vector>

// Shared ownership models reachability for host tests, not Unity GC timing.
template<class T>
class ArrayW {
    struct Storage {
        std::vector<T> values;
        explicit Storage(size_t count) : values(count) {
            auto live = ++liveBuffers;
            auto peak = peakBuffers.load();
            while (peak < live && !peakBuffers.compare_exchange_weak(peak, live)) {}
        }
        ~Storage() { --liveBuffers; }
    };
    std::shared_ptr<Storage> storage;
public:
    inline static std::atomic<size_t> liveBuffers{0}, peakBuffers{0};
    explicit ArrayW(size_t count) : storage(std::make_shared<Storage>(count)) {}
    T* begin() const { return storage->values.data(); }
    size_t size() const { return storage->values.size(); }
    bool operator!=(std::nullptr_t) const { return bool(storage); }
};
