#include <iostream>
#include "stalloc.hpp"

int main() {
    // stalloc_t of type int
    stalloc_t<4096, int> istk;

    // Allocate, set, and free valid pointer
    int* mem1 = istk.alloc(sizeof(int));
    *mem1 = 42;
    std::cout << pr_inf << "*mem1 = " << *mem1 << std::endl;
    istk.free(mem1);

    // Try to free invalid pointer
    istk.free(mem1 - sizeof(int));

    // Try to allocate more than max size
    mem1 = istk.alloc(4097);

    // stalloc_t of unspecified type (default: void)
    stalloc_t<4096> vstk;

    // Allocate, set, and free valid array
    const size_t mem2_len = 16;
    int* mem2 = static_cast<int*>(vstk.alloc(mem2_len * sizeof(int)));
    for (size_t i = 0; i < mem2_len; i++) mem2[i] = i;
    for (size_t i = 0; i < mem2_len; i++) std::cout << pr_inf << "mem2[i] = " << mem2[i] << std::endl;
    vstk.free(mem2);

    return 0;
}
