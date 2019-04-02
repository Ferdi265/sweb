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
      fence();
      T ret = (volatile T&)target;
      fence();
      return ret;
    #else
      global_atomic_lock.acquire();
      T ret = target;
      global_atomic_lock.release();
      return ret;
    #endif
  }

  template <class T>
  static void store(T& target, T value)
  {
    #ifdef VIRTUALIZED_QEMU
      fence();
      ((volatile T&)target) = value;
      fence();
    #else
      global_atomic_lock.acquire();
      target = value;
      global_atomic_lock.release();
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
  static bool compare_exchange(T& target, T& expected, T desired)
  {
    #ifdef VIRTUALIZED_QEMU
      return impl<T>::compare_exchange(target, expected, desired);
    #else
      bool ret;
      global_atomic_lock.acquire();
      if (target == expected) {
        target = desired;
        ret = true;
      } else {
        expected = target;
        ret = false;
      }
      global_atomic_lock.release();
      return ret;
    #endif
  }

  template <class T>
  static T fetch_add(T& target, T inc) {
    T t = load(target);
    while (!compare_exchange(target, ret, ret + inc));
    return t;
  }

  template <class T>
  static T fetch_and(T& target, T mask)
  {
    T t = load(target);
    while (!compare_exchange(target, t, t & mask));
    return t;
  }

  template <class T>
  static T fetch_or(T& target, T mask)
  {
    T t = load(target);
    while (!compare_exchange(target, t, t | mask));
    return t;
  }

  template <class T>
  static T fetch_xor(T& target, T mask)
  {
    T t = load(target);
    while (!compare_exchange(target, t, t ^ mask));
    return t;
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
  static T exchange(T& target, T value)
  {
    T ret;
    __asm__ __volatile__(
      "1: ldaxrb %w0, %2\n\t"
      "stlxrb w1, %w1, %2\n\t"
      "cbnz w1, 1b\n\t"
      : "=&r"(ret) : "r"(value), "Q"(target) : "w1", "memory"
    );
    return ret;
  }

  static bool compare_exchange(T& target, T& expected, T desired)
  {
    bool ret;
    T actual;
    __asm__ __volatile__(
      "1: ldaxrb %w1, %4\n\t"
      "cmp %w1, %w2\n\t"
      "bne 1f\n\t"
      "stlxrb w1, %w3, %4\n\t"
      "cbnz w1, 1b\n\t"
      "1: cset %0, eq\n\t"
      : "=&r"(ret), "=r"(actual) : "r"(expected), "r"(desired), "Q"(target) : "w1", "memory"
    );
    if (!ret) expected = actual;
    return ret;
  }
};

template <class T>
class ArchAtomics::impl<T, 2> {
public:
  static T exchange(T& target, T value)
  {
    T ret;
    __asm__ __volatile__(
      "1: ldaxrh %w0, %2\n\t"
      "stlxrh w1, %w1, %2\n\t"
      "cbnz w1, 1b\n\t"
      : "=&r"(ret) : "r"(value), "Q"(target) : "w1", "memory"
    );
    return ret;
  }

  static bool compare_exchange(T& target, T& expected, T desired)
  {
    bool ret;
    T actual;
    __asm__ __volatile__(
      "1: ldaxrh %w1, %4\n\t"
      "cmp %w1, %w2\n\t"
      "bne 1f\n\t"
      "stlxrh w1, %w3, %4\n\t"
      "cbnz w1, 1b\n\t"
      "1: cset %0, eq\n\t"
      : "=&r"(ret), "=r"(actual) : "r"(expected), "r"(desired), "Q"(target) : "w1", "memory"
    );
    if (!ret) expected = actual;
    return ret;
  }
};

template <class T>
class ArchAtomics::impl<T, 4> {
public:
  static T exchange(T& target, T value)
  {
    T ret;
    __asm__ __volatile__(
      "1: ldaxr %w0, %2\n\t"
      "stlxr w1, %w1, %2\n\t"
      "cbnz w1, 1b\n\t"
      : "=&r"(ret) : "r"(value), "Q"(target) : "w1", "memory"
    );
    return ret;
  }

  static bool compare_exchange(T& target, T& expected, T desired)
  {
    bool ret;
    T actual;
    __asm__ __volatile__(
      "1: ldaxr %w1, %4\n\t"
      "cmp %w1, %w2\n\t"
      "bne 1f\n\t"
      "stlxr w1, %w3, %4\n\t"
      "cbnz w1, 1b\n\t"
      "1: cset %0, eq\n\t"
      : "=&r"(ret), "=r"(actual) : "r"(expected), "r"(desired), "Q"(target) : "w1", "memory"
    );
    if (!ret) expected = actual;
    return ret;
  }
};

template <class T>
class ArchAtomics::impl<T, 8> {
public:
  static T exchange(T& target, T value)
  {
    T ret;
    __asm__ __volatile__(
      "1: ldaxr %0, %2\n\t"
      "stlxr w1, %1, %2\n\t"
      "cbnz w1, 1b\n\t"
      : "=&r"(ret) : "r"(value), "Q"(target) : "w1", "memory"
    );
    return ret;
  }

  static bool compare_exchange(T& target, T& expected, T desired)
  {
    bool ret;
    T actual;
    __asm__ __volatile__(
      "1: ldaxr %1, %4\n\t"
      "cmp %1, %2\n\t"
      "bne 1f\n\t"
      "stlxr w1, %3, %4\n\t"
      "cbnz w1, 1b\n\t"
      "1: cset %0, eq\n\t"
      : "=&r"(ret), "=r"(actual) : "r"(expected), "r"(desired), "Q"(target) : "w1", "memory"
    );
    if (!ret) expected = actual;
    return ret;
  }
};
