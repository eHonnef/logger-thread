#ifndef DUMMY_H
#define DUMMY_H
#ifdef DUMMY_H // So we don't have "unused macro" for the header safeguard
#endif

class Dummy {
public:
  Dummy() {}
  ~Dummy() {}

  bool doSomething() {
    // Do silly things, using some C++17 features to enforce C++17 builds only.
    constexpr int digits[2] = {0, 1};
    auto [zero, one] = digits;
    return zero + one;
  }
};
#endif

#ifdef ENABLE_DOCTEST_IN_LIBRARY
#include "doctest/doctest.h"
TEST_CASE("we can have tests written here, to test impl. details") { CHECK(true); }
#endif