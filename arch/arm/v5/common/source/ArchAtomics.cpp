#include "ArchAtomics.h"

size_t ArchAtomics::test_set_lock(size_t& target, size_t new_value) {
  return ArchAtomics::exchange(target, new_value);
}

void ArchAtomics::fence() {
    asm(
        "mcr p15, 0, %0, c7, c10, 4\n\t"
        :: "r"(0) : "memory"
    );
}
