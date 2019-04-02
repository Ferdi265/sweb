#pragma once

#include "types.h"

class ArchAtomics {
public:
  template <class T>
  constexpr static bool is_implemented()
  {
    return is_lock_free<T>();
  }

  template <class T>
  constexpr static bool is_lock_free()
  {
    return
      sizeof (T) == 1 ||
      sizeof (T) == 2 ||
      sizeof (T) == 4;
  }

  template <class T>
  static T load(T& target)
  {
    fence();
    T ret = (volatile T&)target;
    fence();
    return ret;
  }

  template <class T>
  static void store(T& target, T value)
  {
    fence();
    ((volatile T&)target) = value;
    fence();
  }

  template <class T>
  static T exchange(T& target, T value)
  {
    return impl<T>::exchange(target, value);
  }

  template <class T>
  static bool compare_exchange(T& target, T& expected, T desired)
  {
    return impl<T>::compare_exchange(target, expected, desired);
  }

  template <class T>
  static T fetch_add(T& target, T inc) {
    T t = load(target);
    while (!compare_exchange(target, t, t + inc));
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

  static size_t test_set_lock(size_t& target, size_t new_value);

  static void fence();

private:
  template <class T, size_t sz = sizeof (T)>
  class impl;
};

template <class T>
class ArchAtomics::impl<T, 1> {
public:
  static T exchange(T& target, T value) {
    T ret;
    __asm__ __volatile__(
      "1: ldrexb %0, %2\n\t"
      "strexb r1, %1, %2\n\t"
      "tst r1, #0\n\t"
      "bne 1b\n\t"
      : "=&r"(ret) : "r"(value), "Q"(target) : "r1", "memory"
    );
    return ret;
  }

  static bool compare_exchange(T& target, T& expected, T desired) {
    bool ret;
    T actual;
    __asm__ __volatile__(
      "1: ldrexb %1, %4\n\t"
      "cmp %1, %2\n\t"
      "bne 1f\n\t"
      "strexb r1, %3, %4\n\t"
      "tst r1, #0\n\t"
      "bne 1b\n\t"
      "1: cmp %1, %2\n\t"
      "moveq %0, #1\n\t"
      "movne %0, #0\n\t"
      : "=&r"(ret), "=r"(actual) : "r"(expected), "r"(desired), "Q"(target) : "r1", "memory"
    );
    if (!ret) expected = actual;
    return ret;
  }
};

template <class T>
class ArchAtomics::impl<T, 2> {
public:
  static T exchange(T& target, T value) {
    T ret;
    __asm__ __volatile__(
      "1: ldrexh %0, %2\n\t"
      "strexh r1, %1, %2\n\t"
      "tst r1, #0\n\t"
      "bne 1b\n\t"
      : "=&r"(ret) : "r"(value), "Q"(target) : "r1", "memory"
    );
    return ret;
  }

  static bool compare_exchange(T& target, T& expected, T desired) {
    bool ret;
    T actual;
    __asm__ __volatile__(
      "1: ldrexh %1, %4\n\t"
      "cmp %1, %2\n\t"
      "bne 1f\n\t"
      "strexh r1, %3, %4\n\t"
      "tst r1, #0\n\t"
      "bne 1b\n\t"
      "1: cmp %1, %2\n\t"
      "moveq %0, #1\n\t"
      "movne %0, #0\n\t"
      : "=&r"(ret), "=r"(actual) : "r"(expected), "r"(desired), "Q"(target) : "r1", "memory"
    );
    if (!ret) expected = actual;
    return ret;
  }
};

template <class T>
class ArchAtomics::impl<T, 4> {
public:
  static T exchange(T& target, T value) {
    T ret;
    __asm__ __volatile__(
      "1: ldrex %0, %2\n\t"
      "strex r1, %1, %2\n\t"
      "tst r1, #0\n\t"
      "bne 1b\n\t"
      : "=&r"(ret) : "r"(value), "Q"(target) : "r1", "memory"
    );
    return ret;
  }

  static bool compare_exchange(T& target, T& expected, T desired) {
    bool ret;
    T actual;
    __asm__ __volatile__(
      "1: ldrex %1, %4\n\t"
      "cmp %1, %2\n\t"
      "bne 1f\n\t"
      "strex r1, %3, %4\n\t"
      "tst r1, #0\n\t"
      "bne 1b\n\t"
      "1: cmp %1, %2\n\t"
      "moveq %0, #1\n\t"
      "movne %0, #0\n\t"
      : "=&r"(ret), "=r"(actual) : "r"(expected), "r"(desired), "Q"(target) : "r1", "memory"
    );
    if (!ret) expected = actual;
    return ret;
  }
};
