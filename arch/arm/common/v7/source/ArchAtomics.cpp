#include "ArchAtomics.h"

size_t ArchAtomics::test_set_lock(size_t& target, size_t new_value) {
  return ArchAtomics::exchange(target, new_value);
}

void ArchAtomics::fence() {
    asm(
        "dmb sy\n\t"
        ::: "memory"
    );
}
