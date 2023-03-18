#include <iostream>

#define DEBUG
#define pr_inf "inf[" << __func__ << "]: "
#define pr_err "err[" << __func__ << "]: "

constexpr size_t offset(void* ptr, void* base) { return (size_t)ptr - (size_t)base; }

template<typename T, size_t MAX_SIZE>
class StackAllocator {
    private:
        char* m_Data[MAX_SIZE] = {0};
        size_t m_Sizes[MAX_SIZE] = {0};
#if 0
        size_t m_FreeBlocks[MAX_SIZE] = {0};
#endif

    public:
        StackAllocator() { m_FreeBlocks[0] = MAX_SIZE; };

        T* alloc(size_t size);
        void free(T* ptr);
};

template <typename T, size_t MAX_SIZE>
T* StackAllocator<T, MAX_SIZE>::alloc(size_t size) {
    T* ptr = nullptr;
    size_t rem = size;

    if (!size || size >= MAX_SIZE)
        goto invld;

#if 0
    size_t off = 0;
    while (off < MAX_SIZE) {
        while (off < MAX_SIZE && m_Sizes[off])
            off += m_Sizes[off];
        if (m_FreeBlocks[off] <= size) {
            void* vptr = &m_Data[off];
            ptr = static_cast<T*>(vptr);
            m_Sizes[off] = size;
            m_FreeBlocks[off + size] = m_FreeBlocks[off] - size;
            m_FreeBlocks[off] = 0;
            break;
        }
    }
#endif

    for (size_t i = 0; i < MAX_SIZE; i++) {
        while (m_Sizes[i]) {
            rem = size;
            i += m_Sizes[i];
        }
        if (!(--rem)) {
            void* vptr = &m_Data[i - size + 1];
            ptr = static_cast<T*>(vptr);
            m_Sizes[i - size + 1] = size;
            break;
        }
    }

invld:
#ifdef DEBUG
    if (ptr) std::cout << pr_inf << "alloc(" << size << ") at m_Data[" << offset(ptr, m_Data) << "]"
                       << std::endl;
    else     std::cout << pr_err << "alloc(" << size << ") failed" << std::endl;
#endif

    return ptr;
}

template <typename T, size_t MAX_SIZE>
void StackAllocator<T, MAX_SIZE>::free(T* ptr) {
    size_t off = offset(ptr, m_Data);

    if (off > MAX_SIZE || !m_Sizes[off]) {
        std::cout << pr_err << "misaligned pointer (" << static_cast<void*>(ptr) << ")" << std::endl;
        return;
    }

#ifdef DEBUG
    std::cout << pr_inf << "free(" << m_Sizes[off] << ") at m_Data[" << off << "]"
              << std::endl;
#endif

    m_Sizes[off] = 0;
}

int main() {
    StackAllocator<int, 4096> stk;

    // Allocate, set, and free valid pointer
    int* mem1 = stk.alloc(sizeof(int));
    *mem1 = 42;
    std::cout << pr_inf << "*mem1 = " << *mem1 << std::endl;
    stk.free(mem1);

    // Try to free invalid pointer
    stk.free(mem1 - sizeof(int));

    // Try to allocate more than max size
    mem1 = stk.alloc(4097);

    return 0;
}
