#pragma once
#include <iostream>

#define pr_inf "inf[" << __func__ << "]: "
#define pr_err "err[" << __func__ << "]: "

constexpr size_t offset(void* ptr, void* base) { return (size_t)ptr - (size_t)base; }

template<size_t STSIZE_MAX, typename T = void>
class stalloc_t {
    private:
        char* m_data[STSIZE_MAX] = {0};

    public:
        stalloc_t() = default;

        T* alloc(size_t size);
        void free(T* ptr);
};

template<size_t STSIZE_MAX, typename T>
T* stalloc_t<STSIZE_MAX, T>::alloc(size_t size) {
    T* ptr = nullptr;
    return ptr;
}

template<size_t STSIZE_MAX, typename T>
void stalloc_t<STSIZE_MAX, T>::free(T* ptr) {
    return;
}
