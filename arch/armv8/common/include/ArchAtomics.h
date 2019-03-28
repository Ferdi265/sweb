#pragma once

#include "types.h"
#include "SpinLock.h"

class ArchAtomics {
public:
  static void initialise();

  template <class T>
  constexpr static bool is_implemented()
  {
    #ifdef VIRTUALIZED_QEMU
      return is_lock_free<T>();
    #else
      return 
        sizeof (T) == 1 ||
        sizeof (T) == 2 ||
        sizeof (T) == 4 ||
        sizeof (T) == 8;
    #endif
  }

  template <class T>
  constexpr static bool is_lock_free()
  {
    #ifdef VIRTUALIZED_QEMU
      return
        sizeof (T) == 1 ||
        sizeof (T) == 2 ||
        sizeof (T) == 4 ||
        sizeof (T) == 8;
    #else
      return false;
    #endif
  }

  template <class T>
  static T load(T& target)
  {
    #ifdef VIRTUALIZED_QEMU
      return impl<T>::load(target);
    #else
      global_atomic_lock.acquire();
      T ret = target;
      global_atomic_lock.release();
      return ret;
    #endif
  }

  template <class T>
  static T load_exclusive(T& target)
  {
    #ifdef VIRTUALIZED_QEMU
      return impl<T>::load_exclusive(target);
    #else
      global_atomic_lock.acquire();
      T ret = target;
      // simulate exclusive load by not unlocking the atomic lock
      // use with care!!
      return ret;
    #endif
  }

  template <class T>
  static void store(T& target, T value)
  {
    #ifdef VIRTUALIZED_QEMU
      impl<T>::store(target, value);
    #else
      global_atomic_lock.acquire();
      target = value;
      global_atomic_lock.release();
    #endif
  }

  template <class T>
  static bool store_exclusive(T& target, T value)
  {
    #ifdef VIRTUALIZED_QEMU
      return impl<T>::store_exclusive(target, value);
    #else
      // simulate exclusive store by not locking the atomic lock
      // use with care!!
      target = value;
      global_atomic_lock.release();
      return true;
    #endif
  }

  template <class T>
  static T exchange(T& target, T value)
  {
    #ifdef VIRTUALIZED_QEMU
      return impl<T>::exchange(target, value);
    #else
      global_atomic_lock.acquire();
      T ret = target;
      target = value;
      global_atomic_lock.release();
      return ret;
    #endif
  }

  template <class T>
  static bool compare_exchange(T& target, T& expected, T desired);

  template <class T>
  static T fetch_add(T& target, T inc) {
    T ret;
    do {
      ret = load_exclusive(target);
    } while (!store_exclusive(target, ret + inc));
    return ret;
  }

  template <class T>
  static T fetch_and(T& target, T inc) {
    T ret;
    do {
      ret = load_exclusive(target);
    } while (!store_exclusive(target, ret & inc));
    return ret;
  }

  template <class T>
  static T fetch_or(T& target, T inc) {
    T ret;
    do {
      ret = load_exclusive(target);
    } while (!store_exclusive(target, ret | inc));
    return ret;
  }

  template <class T>
  static T fetch_xor(T& target, T inc) {
    T ret;
    do {
      ret = load_exclusive(target);
    } while (!store_exclusive(target, ret ^ inc));
    return ret;
  }

  template <class T>
  static T fetch_sub(T& target, T dec)
  {
    return fetch_add(target, -dec);
  }

  template <class T>
  static T add_fetch(T& target, T inc)
  {
    return fetch_add(target, inc) + inc;
  }

  template <class T>
  static T sub_fetch(T& target, T dec)
  {
    return fetch_sub(target, dec) - dec;
  }

  template <class T>
  static T and_fetch(T& target, T mask)
  {
    return fetch_and(target, mask) & mask;
  }

  template <class T>
  static T or_fetch(T& target, T mask)
  {
    return fetch_or(target, mask) | mask;
  }

  template <class T>
  static T xor_fetch(T& target, T mask)
  {
    return fetch_xor(target, mask) ^ mask;
  }

  static void fence();

  static size_t test_set_lock(size_t& lock, size_t new_value);

private:
  #ifndef VIRTUALIZED_QEMU
    static SpinLock global_atomic_lock;
  #endif

  template <class T, size_t sz = sizeof (T)>
  class impl;
};

#ifndef VIRTUALIZED_QEMU
  // explicitly specialize ArchAtomics::store since the Lock class wants to use it
  // this avoids a deadlock when locking the global atomic lock
  template <>
  void ArchAtomics::store<size_t>(size_t& target, size_t value);
#endif

template <class T>
class ArchAtomics::impl<T, 1> {
public:
  static T load(T& target)
  {
    T ret;
    fence();
    __asm__ __volatile__(
      "ldarb %0, %1\n\t"
      : "=r"(ret) : "Q"(target)
    );
    return ret;
  }

  static T load_exclusive(T& target)
  {
    T ret;
    __asm__ __volatile__(
      "ldaxrb %0, %1\n\t"
      : "=r"(ret) : "Q"(target)
    );
    return ret;
  }

  static void store(T& target, T value)
  {
    __asm__ __volatile__(
      "stlrb %0, %1\n\t"
      :: "r"(value), "Q"(target)
    );
    fence();
  }

  static bool store_exclusive(T& target, T value)
  {
    uint32 status;
    __asm__ __volatile__(
      "stlxrb %w0, %1, %2\n\t"
      : "=&r"(status) : "r"(value), "Q"(target)
    );
    return status == 0;
  }

  static T exchange(T& target, T value)
  {
    T ret;
    __asm__ __volatile__(
      "1: ldaxrb %0, %2\n\t"
      "stlxrb w1, %1, %2\n\t"
      "cbnz w1, 1b\n\t"
      : "=&r"(ret) : "r"(value), "Q"(target) : "w1"
    );
    return ret;
  }
};

template <class T>
class ArchAtomics::impl<T, 2> {
public:
  static T load(T& target)
  {
    T ret;
    fence();
    __asm__ __volatile__(
      "ldarh %0, %1\n\t"
      : "=r"(ret) : "Q"(target)
    );
    return ret;
  }

  static T load_exclusive(T& target)
  {
    T ret;
    __asm__ __volatile__(
      "ldaxrh %0, %1\n\t"
      : "=r"(ret) : "Q"(target)
    );
    return ret;
  }

  static void store(T& target, T value)
  {
    __asm__ __volatile__(
      "stlrh %0, %1\n\t"
      :: "r"(value), "Q"(target)
    );
    fence();
  }

  static bool store_exclusive(T& target, T value)
  {
    uint32 status;
    __asm__ __volatile__(
      "stlxrh %w0, %1, %2\n\t"
      : "=&r"(status) : "r"(value), "Q"(target)
    );
    return status == 0;
  }

  static T exchange(T& target, T value)
  {
    T ret;
    __asm__ __volatile__(
      "1: ldaxrh %0, %2\n\t"
      "stlxrh w1, %1, %2\n\t"
      "cbnz w1, 1b\n\t"
      : "=&r"(ret) : "r"(value), "Q"(target) : "w1"
    );
    return ret;
  }
};

template <class T>
class ArchAtomics::impl<T, 4> {
public:
  static T load(T& target)
  {
    T ret;
    fence();
    __asm__ __volatile__(
      "ldar %0, %1\n\t"
      : "=r"(ret) : "Q"(target)
    );
    return ret;
  }

  static T load_exclusive(T& target)
  {
    T ret;
    __asm__ __volatile__(
      "ldaxr %0, %1\n\t"
      : "=r"(ret) : "Q"(target)
    );
    return ret;
  }

  static void store(T& target, T value)
  {
    __asm__ __volatile__(
      "stlr %0, %1\n\t"
      :: "r"(value), "Q"(target)
    );
    fence();
  }

  static bool store_exclusive(T& target, T value)
  {
    uint32 status;
    __asm__ __volatile__(
      "stlxr %w0, %1, %2\n\t"
      : "=&r"(status) : "r"(value), "Q"(target)
    );
    return status == 0;
  }

  static T exchange(T& target, T value)
  {
    T ret;
    __asm__ __volatile__(
      "1: ldaxr %0, %2\n\t"
      "stlxr w1, %1, %2\n\t"
      "cbnz w1, 1b\n\t"
      : "=&r"(ret) : "r"(value), "Q"(target) : "w1"
    );
    return ret;
  }
};

template <class T>
class ArchAtomics::impl<T, 8> {
public:
  static T load(T& target)
  {
    // instruction name is the same for 4 and 8 bytes
    return ArchAtomics::impl<T, 4>::load(target);
  }

  static T load_exclusive(T& target)
  {
    // instruction name is the same for 4 and 8 bytes
    return ArchAtomics::impl<T, 4>::load_exclusive(target);
  }

  static void store(T& target, T value)
  {
    // instruction name is the same for 4 and 8 bytes
    ArchAtomics::impl<T, 4>::store(target, value);
  }

  static bool store_exclusive(T& target, T value)
  {
    // instruction name is the same for 4 and 8 bytes
    return ArchAtomics::impl<T, 4>::store_exclusive(target, value);
  }

  static T exchange(T& target, T value)
  {
    // instruction name is the same for 4 and 8 bytes
    return ArchAtomics::impl<T, 4>::exchange(target, value);
  }
};
