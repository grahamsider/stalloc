#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <type_traits>
#include <bit>

enum stalloc_fit_t { first_fit, best_fit };
enum stalloc_ord_t { lifo_order, addr_order };

template<size_t MaxSize, typename T = void, stalloc_fit_t F = stalloc_fit_t::best_fit,
                                            stalloc_ord_t O = stalloc_ord_t::lifo_order>
class stalloc_t {
    /* Word and double-word sizes, architecture dependant (bytes) */
    /* Note: On 64-bit architectures, alignment (DSIZE) is 16 bytes */
    static constexpr size_t WSIZE = sizeof(void*);
    static constexpr size_t DSIZE = 2 * WSIZE;

    /* Pack size and alloc bit into a word for header/footer */
    /* Note: size is assumed to be DSIZE aligned */
    static constexpr uintptr_t PACK(size_t size, bool alloc) { return (size | alloc); }

    /* Read and write a word at address p */
    static constexpr uintptr_t GET(void* p) { return *(uintptr_t*)p; }
    static constexpr void PUT(void* p, uintptr_t v) { *(uintptr_t*)p = v; }

    /* Read size and alloc fields from address p */
    static constexpr size_t GET_SIZE(void* p) { return GET(p) & ~(DSIZE - 1); }
    static constexpr size_t GET_ALLOC(void* p) { return GET(p) & 0x1; }

    /* Get header/footer address from block pointer */
    static constexpr void* HDRP(void* bp) { return (void*)((size_t)bp - WSIZE); }
    static constexpr void* FTRP(void* bp) { return (void*)((size_t)bp + GET_SIZE(HDRP(bp)) - DSIZE); }

    /* Get next/previous blocks from block pointer */
    static constexpr void* NEXT_BLKP(void* bp) { return (void*)((size_t)bp + GET_SIZE((void*)((size_t)bp - WSIZE))); }
    static constexpr void* PREV_BLKP(void* bp) { return (void*)((size_t)bp - GET_SIZE((void*)((size_t)bp - DSIZE))); }

    /* Check if next/previous blocks exist (i.e. if current block is at boundary) */
    static constexpr bool PREV_EXIST(void* bp) { return GET((void*)((size_t)bp - DSIZE)); }
    static constexpr bool NEXT_EXIST(void* bp) { return GET((void*)((size_t)bp + GET_SIZE(HDRP(bp)) - WSIZE)); }

    /* Calculate offset between two pointers */
    static constexpr size_t OFFSET(void* p, void* b) { return (size_t)p - (size_t)b; }

    /* Alignment helpers */
    static constexpr size_t ALIGN_MASK(size_t x, size_t m) { return (x + m) & (~m); }
    static constexpr size_t ALIGN_UP(size_t x) { return ALIGN_MASK(x, DSIZE-1); }
    static constexpr size_t ALIGN_SIZE(size_t x) { return (x > DSIZE) ? ALIGN_UP(x) + DSIZE : 2 * DSIZE; }

    /* Segregated freelist range helper and number of ranges constant */
    static constexpr size_t FLIST_RANGE(size_t x) { return std::bit_width(x) - std::bit_width(DSIZE); }
    static constexpr size_t FLIST_NRANGES = FLIST_RANGE(MaxSize);

    /* Ensure T is a trivially copyable type (or void) */
    static_assert(std::is_trivially_copyable_v<T> || std::is_void_v<T>);

    /* Ensure MaxSize is double-word aligned and can fit at least one block */
    static_assert(((MaxSize & (DSIZE-1)) == 0) && (MaxSize >= 3 * DSIZE));

    /* Best fit with address ordering is pointless -- disallow */
    static_assert(!(F == stalloc_fit_t::best_fit && O == stalloc_ord_t::addr_order),
            "stalloc_fit_t::best_fit with stalloc_ord_t::addr_order not allowed");

    /* Freelist type for explicit free linked list */
    struct fl_t {
        fl_t* prev;
        fl_t* next;
    };

    private:
        unsigned char m_data[MaxSize] = {0};
        void* const m_listp = m_data + DSIZE;
        fl_t* m_flistp[FLIST_NRANGES] = {nullptr};

        void* find_fit(const size_t asize);
        void place(void* const bp, size_t asize);
        void coalesce(void* const bp);

        void fl_insert(void* const bp);
        void fl_remove(void* const bp);

    public:
        stalloc_t() {
            /* First and last words are reserved */
            PUT(m_data + WSIZE, PACK(MaxSize - DSIZE, false));
            PUT(FTRP(m_data + DSIZE), PACK(MaxSize - DSIZE, false));

            /* Freelist starts as a single node of the largest range */
            m_flistp[FLIST_NRANGES-1] = (fl_t*)(m_data + DSIZE);
            m_flistp[FLIST_NRANGES-1]->prev = nullptr;
            m_flistp[FLIST_NRANGES-1]->next = nullptr;
        };

        [[nodiscard]] T* alloc(const size_t size);
        void free(T* const bp);

        /* Debug */
        void printb();
};

/**
 * stalloc_t::printb()
 *
 * Print a formatted representation of the instantiated stack
 * allocator's block list.
 */
template<size_t MaxSize, typename T, stalloc_fit_t F, stalloc_ord_t O>
void stalloc_t<MaxSize, T, F, O>::printb() {
    printf("+------------------------------------------------+\n"
           "|                      Stack                     |\n"
           "+-------+----------------+--------------+--------+\n"
           "| Block |     Address    |     Size     | Status |\n"
           "+-------+----------------+--------------+--------+\n");

    int i = 0;
    for (void* bp = m_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp), i++) {
        printf("| %-6d| %p | %-13ld|   %c    |\n"
               "+-------+----------------+--------------+--------+\n",
                i, bp, GET_SIZE(HDRP(bp)), (GET_ALLOC(HDRP(bp)) ? 'A' : 'F'));
    }
}

/**
 * stalloc_t::fl_insert()
 *
 * Insert block into freelist.
 *
 * Order algorithm may be chosen at compile time/instantiation
 * via the stalloc_ord_t type template parameter. Defaults to
 * stalloc_ord_t::lifo_order.
 */
template<size_t MaxSize, typename T, stalloc_fit_t F, stalloc_ord_t O>
void stalloc_t<MaxSize, T, F, O>::fl_insert(void* const bp) {
    const size_t fl_ridx = FLIST_RANGE(GET_SIZE(HDRP(bp)));
    fl_t* const fbp = static_cast<fl_t*>(bp);

    /* Ignore invalid requests */
    if (!fbp)
        return;

    /* If freelist is empty, fbp is new start of freelist */
    if (!m_flistp[fl_ridx]) {
        m_flistp[fl_ridx] = fbp;
        m_flistp[fl_ridx]->prev = nullptr;
        m_flistp[fl_ridx]->next = nullptr;
        return;
    }

    /* LIFO Ordering */
    if constexpr (O == stalloc_ord_t::lifo_order) {
        fbp->prev = nullptr;
        fbp->next = m_flistp[fl_ridx];
        m_flistp[fl_ridx]->prev = fbp;
        m_flistp[fl_ridx] = fbp;
    }
    /* Address Ordering */
    if constexpr (O == stalloc_ord_t::addr_order) {
        if (fbp < m_flistp[fl_ridx]) {
            fbp->prev = nullptr;
            fbp->next = m_flistp[fl_ridx];
            m_flistp[fl_ridx]->prev = fbp;
            m_flistp[fl_ridx] = fbp;
            return;
        }

        fl_t* flp = m_flistp[fl_ridx];
        while (flp->next && flp < fbp)
            flp = flp->next;

        if (flp < fbp) {
            flp->next = fbp;
            fbp->prev = flp;
            fbp->next = nullptr;
        } else {
            fbp->prev = flp->prev;
            fbp->next = flp;
            flp->prev = fbp;
            fbp->prev->next = fbp;
        }
    }
}

/**
 * stalloc_t::fl_remove()
 *
 * Remove block from freelist.
 */
template<size_t MaxSize, typename T, stalloc_fit_t F, stalloc_ord_t O>
void stalloc_t<MaxSize, T, F, O>::fl_remove(void* const bp) {
    const size_t fl_ridx = FLIST_RANGE(GET_SIZE(HDRP(bp)));
    fl_t* const fbp = static_cast<fl_t*>(bp);

    /* Ignore invalid requests */
    if (!fbp)
        return;

    /* Only block in freelist */
    if (!fbp->prev && !fbp->next) {
        m_flistp[fl_ridx] = nullptr;
    }
    /* Located at head of freelist */
    else if (!fbp->prev) {
        m_flistp[fl_ridx] = fbp->next;
        m_flistp[fl_ridx]->prev = nullptr;
        fbp->next = nullptr;
    }
    /* Located at tail of freelist */
    else if (!fbp->next) {
        fbp->prev->next = nullptr;
        fbp->prev = nullptr;
    }
    /* Located in middle of freelist */
    else {
        fbp->prev->next = fbp->next;
        fbp->next->prev = fbp->prev;
        fbp->prev = nullptr;
        fbp->next = nullptr;
    }
}

/**
 * stalloc_t::find_fit()
 *
 * Free block fit finder. Returns pointer to the allotted
 * block if fit is found. Otherwise returns nullptr.
 *
 * Fit algorithm may be chosen at compile time/instantiation
 * via the stalloc_fit_t type template parameter. Defaults to
 * stalloc_type_t::first_fit.
 */
template<size_t MaxSize, typename T, stalloc_fit_t F, stalloc_ord_t O>
void* stalloc_t<MaxSize, T, F, O>::find_fit(const size_t asize) {
    size_t fl_ridx = FLIST_RANGE(asize);

    for (; fl_ridx < FLIST_NRANGES; fl_ridx++) {
        fl_t* flp = m_flistp[fl_ridx];

        /* First Fit */
        if constexpr (F == stalloc_fit_t::first_fit) {
            for (; flp; flp = flp->next)
                if (asize <= GET_SIZE(HDRP(flp)))
                    return static_cast<void*>(flp);
        }

        /* Best Fit */
        if constexpr (F == stalloc_fit_t::best_fit) {
            fl_t* bp = nullptr;
            size_t bp_size = ~((size_t)0);

            for (; flp; flp = flp->next) {
                const size_t flp_size = GET_SIZE(HDRP(flp));
                if (asize <= flp_size && flp_size < bp_size) {
                    bp = flp;
                    bp_size = flp_size;
                }
            }
            if (bp) return static_cast<void*>(bp);
        }
    }

    return nullptr;
}

/**
 * stalloc_t::place()
 *
 * Sets the header and footer of the allotted block and leftover
 * block (when applicable) to (total_size | 1) to complete allocation.
 * The size placed in the header/footer includes that of the header and
 * footer themselves.
 */
template<size_t MaxSize, typename T, stalloc_fit_t F, stalloc_ord_t O>
void stalloc_t<MaxSize, T, F, O>::place(void* const bp, size_t asize) {
    /* Get current (free) block size and leftover block size */
    const size_t fsize = GET_SIZE(HDRP(bp));
    const size_t lsize = fsize - asize;

    /* If leftover size is too small for another block, use all of free
     * block size. Otherwise set leftover block size accordingly */
    if (lsize < 2 * DSIZE) {
        asize = fsize;
    } else {
        PUT(HDRP((void*)((size_t)bp + asize)), PACK(lsize, false));
        PUT(FTRP((void*)((size_t)bp + asize)), PACK(lsize, false));
        fl_insert((void*)((size_t)bp + asize));
    }

    fl_remove(bp);

    /* Write header and footer for newly allocated block */
    PUT(HDRP(bp), PACK(asize, true));
    PUT(FTRP(bp), PACK(asize, true));
}

/**
 * stalloc_t::alloc()
 *
 * Public facing allocation subroutine. Attempts to find a free
 * block of adequate size for the request. Returns a pointer to
 * the start of that free block on success. Returns nullptr on
 * failure.
 *
 * The block header is one word prior to the start of the newly
 * allotted block whose address is returned to the user. Similarly,
 * the block footer is located directly after the end of the aligned
 * block.
 *
 * The start address of the newly allotted block is always double-
 * word aligned, as is the size of the block.
 */
template<size_t MaxSize, typename T, stalloc_fit_t F, stalloc_ord_t O>
T* stalloc_t<MaxSize, T, F, O>::alloc(const size_t size) {
    /* Ignore zero-sized and known-too-large requests */
    if (!size || size > MaxSize - (2 * DSIZE))
        return nullptr;

    void* bp = nullptr;
    const size_t asize = ALIGN_SIZE(size);

    if ((bp = find_fit(asize)))
        place(bp, asize);

    return static_cast<T*>(bp);
}

/**
 * stalloc_t::free()
 *
 * Public facing de-allocation subroutine. Attempts to free the
 * given block whose pointer is provided by the user. Silently
 * fails if given an invalid request.
 *
 * On success attempts to coalesce adjacent free blocks.
 */
template<size_t MaxSize, typename T, stalloc_fit_t F, stalloc_ord_t O>
void stalloc_t<MaxSize, T, F, O>::free(T* const bp) {
    void* const vbp = static_cast<void*>(bp);

    /* Ignore invalid requests */
    if (!vbp || !GET_ALLOC(HDRP(vbp)))
        return;

    const size_t size = GET_SIZE(HDRP(vbp));
    PUT(HDRP(vbp), PACK(size, false));
    PUT(FTRP(vbp), PACK(size, false));

    fl_insert(vbp);
    coalesce(vbp);
}

/**
 * stalloc_t::coalesce()
 *
 * Attempt to coalesce adjacent free blocks. In order to coalesce,
 * adjacent block must both exist (i.e. given block pointer is not
 * at a boundary) and have its alloc flag set to false.
 */
template<size_t MaxSize, typename T, stalloc_fit_t F, stalloc_ord_t O>
void stalloc_t<MaxSize, T, F, O>::coalesce(void* const bp) {
    const bool prev = PREV_EXIST(bp) && !GET_ALLOC(HDRP(PREV_BLKP(bp)));
    const bool next = NEXT_EXIST(bp) && !GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    void* prev_hdrp = nullptr;
    void* prev_ftrp = nullptr;
    void* next_hdrp = nullptr;
    void* next_ftrp = nullptr;
    void* new_bp = nullptr;
    size_t size = GET_SIZE(HDRP(bp));

    if (prev) {
        prev_hdrp = HDRP(PREV_BLKP(bp));
        prev_ftrp = FTRP(PREV_BLKP(bp));
        size += GET_SIZE(prev_hdrp);
    }
    if (next) {
        next_hdrp = HDRP(NEXT_BLKP(bp));
        next_ftrp = FTRP(NEXT_BLKP(bp));
        size += GET_SIZE(next_hdrp);
    }

    if (prev && next) {
        new_bp = PREV_BLKP(bp);
        fl_remove(NEXT_BLKP(bp));
        fl_remove(bp);
        fl_remove(new_bp);

        PUT(prev_ftrp, 0);
        PUT(prev_hdrp, PACK(size, false));

        PUT(next_ftrp, PACK(size, false));
        PUT(next_hdrp, 0);

        PUT(FTRP(bp), 0);
        PUT(HDRP(bp), 0);
    } else if (prev) {
        new_bp = PREV_BLKP(bp);
        fl_remove(bp);
        fl_remove(new_bp);

        PUT(prev_ftrp, 0);
        PUT(prev_hdrp, PACK(size, false));

        PUT(FTRP(bp), PACK(size, false));
        PUT(HDRP(bp), 0);
    } else if (next) {
        new_bp = bp;
        fl_remove(NEXT_BLKP(bp));
        fl_remove(new_bp);

        PUT(next_ftrp, PACK(size, false));
        PUT(next_hdrp, 0);

        PUT(FTRP(bp), 0);
        PUT(HDRP(bp), PACK(size, false));
    }

    if (new_bp) fl_insert(new_bp);
}
