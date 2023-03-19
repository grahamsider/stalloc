#pragma once
#include <iostream>

#define pr_inf "inf[" << __func__ << "]: "
#define pr_err "err[" << __func__ << "]: "

constexpr size_t offset(void* ptr, void* base) { return (size_t)ptr - (size_t)base; }

template<size_t MAX_SIZE, typename T = void>
class stalloc_t {
    private:
        char* m_data[MAX_SIZE] = {0};
        size_t m_sizes[MAX_SIZE] = {0};

    public:
        stalloc_t() = default;

        T* alloc(size_t size);
        void free(T* ptr);
};

template<size_t MAX_SIZE, typename T>
T* stalloc_t<MAX_SIZE, T>::alloc(size_t size) {
    T* ptr = nullptr;
    size_t rem = size;

    if (!size || size >= MAX_SIZE)
        goto invld;

    for (size_t i = 0; i < MAX_SIZE; i++) {
        while (m_sizes[i]) {
            rem = size;
            i += m_sizes[i];
        }
        if (!(--rem)) {
            void* vptr = &m_data[i - size + 1];
            ptr = static_cast<T*>(vptr);
            m_sizes[i - size + 1] = size;
            break;
        }
    }

invld:
#ifdef DEBUG
    if (ptr) std::cout << pr_inf << "alloc(" << size << ") at m_data[" << offset(ptr, m_data) << "]"
                       << std::endl;
    else     std::cout << pr_err << "alloc(" << size << ") failed" << std::endl;
#endif

    return ptr;
}

template<size_t MAX_SIZE, typename T>
void stalloc_t<MAX_SIZE, T>::free(T* ptr) {
    size_t off = offset(ptr, m_data);

    if (off > MAX_SIZE || !m_sizes[off]) {
#ifdef DEBUG
        std::cout << pr_err << "misaligned pointer (" << static_cast<void*>(ptr) << ")" << std::endl;
#endif
        return;
    }

#ifdef DEBUG
    std::cout << pr_inf << "free(" << m_sizes[off] << ") at m_data[" << off << "]"
              << std::endl;
#endif

    m_sizes[off] = 0;
}
