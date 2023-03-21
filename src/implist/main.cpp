#include <iostream>
#include "stalloc.hpp"

int main() {
    stalloc_t<4096> st;

    int* i = static_cast<int*>(st.alloc(sizeof(int)));
    int* j = static_cast<int*>(st.alloc(sizeof(int)));
    int* k = static_cast<int*>(st.alloc(sizeof(int)));
    st.printb();

    st.free(i);
    st.printb();

    st.free(j);
    st.printb();

    st.free(k);
    st.printb();

    return 0;
}
