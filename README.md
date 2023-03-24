# stalloc

A C++ Stack Allocation Library

## Implementations

### Implicit List

*Features:*

- Size and type generic
- Bidirectional bounding tags
- Bidirectional immediate coalescing
- Templated first fit or best fit policy

*Runtime:*

- Allocation: Linear in number of free and allocated (total) blocks
- Free: Constant

### Explicit List

*Features:*

- Size and type generic
- Bidirectional bounding tags
- Bidirectional immediate coalescing
- Templated first fit or best fit policy
- Templated LIFO order or address order policy

*Runtime:*

- Allocation: Linear in number of free blocks
- Free: Constant with LIFO ordering, linear in number of free blocks with address ordering

## Example Instantiations

```c++
/* 4KB stack buffer, first fit, LIFO order (for explicit list) */
void* stalloc_t<4096> st;

/* 4KB stack buffer, type int*, first fit, LIFO order (for explicit list) */
int* stalloc_t<4096, int> st;

/* 4KB stack buffer, type char*, best fit, LIFO order (for explicit list) */
char* stalloc_t<4096, int, stalloc_fit_t::best_fit> st;

/* 4KB stack buffer, type char*, first fit, address order (for explicit list) */
char* stalloc_t<4096, int, stalloc_fit_t::first_fit,
                           stalloc_ord_t::addr_order> st;
```

Example usage may be found in the test main.cpp files.

## Build & Run Tests

```bash
git clone git@github.com:grahamsider/stalloc.git && cd stalloc
make
./build/implist/implist_test # run the implicit list tester
./build/implist/explicit_test # run the explicit list tester
```
