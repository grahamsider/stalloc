#include <iostream>
#include <cassert>
#include <chrono>
#include "stalloc.hpp"

#define pr_inf "inf[" << __func__ << "]: "
#define pr_err "err[" << __func__ << "]: "

int main() {
    stalloc_t<4096, int, stalloc_fit_t::best_fit> st;

    int* i = nullptr;
    int* j = nullptr;
    int* k = nullptr;

    /* Allocate and free three 16B blocks */
    std::cout << std::endl << pr_inf << "allocating three 16B blocks" << std::endl;
    i = st.alloc(4 * sizeof(int));
    j = st.alloc(4 * sizeof(int));
    k = st.alloc(4 * sizeof(int));
    st.printb();
    assert(i && j && k);

    std::cout << std::endl << pr_inf << "freeing i (" << i << ")" << std::endl;
    st.free(i);
    st.printb();
    i = nullptr;

    std::cout << std::endl << pr_inf << "freeing j (" << j << ")" << std::endl;
    st.free(j);
    st.printb();
    j = nullptr;

    std::cout << std::endl << pr_inf << "freeing k (" << k << ")" << std::endl;
    st.free(k);
    st.printb();
    k = nullptr;

    /* Allocate and free max size (4064B) */
    std::cout << std::endl << pr_inf << "allocating block of max size" << std::endl;
    i = st.alloc(1016 * sizeof(int));
    st.printb();
    assert(i);

    std::cout << std::endl << pr_inf << "freeing i (" << i << ")" << std::endl;
    st.free(i);
    st.printb();
    i = nullptr;

    /* Try to allocate more than max size */
    std::cout << std::endl << pr_inf << "trying to allocate block greater than max size" << std::endl;
    i = st.alloc(1024 * sizeof(int));
    st.printb();
    assert(!i);

    /* Allocate just under max size (not enough leftover for another block) */
    std::cout << std::endl << pr_inf << "allocating block just under max size" << std::endl;
    i = st.alloc(1012 * sizeof(int));
    st.printb();
    assert(i);

    /* Try to allocate another block (previous allocation should fill space) */
    std::cout << std::endl << pr_inf << "trying to allocate another block" << std::endl;
    j = st.alloc(4 * sizeof(int));
    st.printb();
    assert(!j);

    std::cout << std::endl << pr_inf << "freeing i (" << i << ")" << std::endl;
    st.free(i);
    st.printb();
    i = nullptr;

    std::cout << std::endl;

    /* Allocate 126 16B blocks. Boundary tags will make each buffer 32B total.
     * 126 * 32B = 4032B => 4080B - 4032B = 48B leftover. */
    std::cout << pr_inf << "allocating 126 16B blocks" << std::endl;
    int* abuf[126] = {nullptr};
    for (int idx = 0; idx < 126; idx++) {
        abuf[idx] = st.alloc(4 * sizeof(int));
        assert(abuf[idx]);
    }

    /* Try to allocate a 48B buffer. This should fail due to inadequate space for boundary tags */
    std::cout << pr_inf << "trying to allocate another 48B block" << std::endl;
    i = st.alloc(12 * sizeof(int));
    assert(!i);

    /* Try to allocate a 32B buffer. This should be a valid allocation */
    std::cout << pr_inf << "allocating another 32B block" << std::endl;
    i = st.alloc(8 * sizeof(int));
    assert(i);

    /* Free every second block in the 126 block array */
    std::cout << pr_inf << "freeing every second block in the 126 block array" << std::endl;
    for (int idx = 1; idx < 126; idx += 2) {
        st.free(abuf[idx]);
        abuf[idx] = nullptr;
    }

    /* Free the rest of the blocks from last to first */
    std::cout << pr_inf << "freeing the rest of the blocks from last to first" << std::endl;
    st.free(i);
    i = nullptr;
    for (int idx = 124; idx >= 0; idx -= 2) {
        st.free(abuf[idx]);
        abuf[idx] = nullptr;
    }
    st.printb();

    /* Allocate seven blocks of decreasing size (256B -> 64B) */
    std::cout << std::endl << pr_inf << "allocating seven blocks of decreasing size" << std::endl;
    int* bbuf[7];
    for (int idx = 0; idx < 7; idx++) {
        bbuf[idx] = st.alloc(256 - (32 * idx));
        assert(bbuf[idx]);
    }
    st.printb();

    /* Free every second block in the seven block array */
    std::cout << std::endl << pr_inf << "freeing every second block in the seven block array" << std::endl;
    for (int idx = 1; idx < 7; idx += 2) {
        st.free(bbuf[idx]);
        bbuf[idx] = nullptr;
    }
    st.printb();

    /* Allocate new blocks the same size as the previously freed ones
     * This should result in the same block allocations when using best fit,
     * but a different (less ideal) block allocation when using first fit */
    std::cout << std::endl << pr_inf << "re-allocating the previously freed blocks backwards" << std::endl;
    for (int idx = 5; idx > 0; idx -= 2) {
        bbuf[idx] = st.alloc(256 - (32 * idx));
        assert(bbuf[idx]);
    }
    st.printb();

    /* Free all blocks from first to last */
    std::cout << std::endl << pr_inf << "freeing blocks from first to last" << std::endl;
    for (int idx = 0; idx < 7; idx++) {
        st.free(bbuf[idx]);
        bbuf[idx] = nullptr;
    }
    st.printb();

    /* Allocate and free entire buffer many times */
    std::cout << std::endl << pr_inf << "running performance test (65,536 loops)..." << std::endl;;
    auto start_time = std::chrono::high_resolution_clock::now();
    for (int l = 0; l < 65536; l++) {
        for (int idx = 0; idx < 126; idx++) {
            abuf[idx] = st.alloc(4 * sizeof(int));
            assert(abuf[idx]);
        }
        i = st.alloc(8 * sizeof(int));
        assert(i);
        for (int idx = 1; idx < 126; idx += 2) {
            st.free(abuf[idx]);
            abuf[idx] = nullptr;
        }
        st.free(i);
        i = nullptr;
        for (int idx = 124; idx >= 0; idx -= 2) {
            st.free(abuf[idx]);
            abuf[idx] = nullptr;
        }
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto dur_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << pr_inf << "performance test done [" << dur_time.count() / 1000. << "s]" << std::endl;

    return 0;
}
