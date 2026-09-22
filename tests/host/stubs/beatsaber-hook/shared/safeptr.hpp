#pragma once
// GC rooting is verified by the Quest integration checks, not by this stub.
template<class T, bool Unity = false>
struct safe_ptr {
    T value;
    safe_ptr(T value) : value(value) {}
    T ptr() const { return value; }
    T operator->() const { return value; }
    explicit operator bool() const {
        if constexpr (Unity) return value && value->m_CachedPtr.m_value;
        else return value != nullptr;
    }
};
