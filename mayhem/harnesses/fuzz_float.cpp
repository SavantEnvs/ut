// Fuzz boost::ut's floating-point value wrappers (_d / _f / _ld and the
// detail::value<floating> constructor), which derive the comparison epsilon
// from the value via math::den_size + math::pow (upstream fix f6cf5f4663:
// "Fix hang when a floating point value is too big for den_size").
#include "fuzz_common.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ut = boost::ut;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  using namespace ut;
  ut::cfg<ut::override>.reset();
  if (size >= sizeof(double)) {
    double d;
    std::memcpy(&d, data, sizeof d);
    expect(_d(d) == d);
    expect(d != _d(d));
    data += sizeof d;
    size -= sizeof d;
  }
  if (size >= sizeof(float)) {
    float f;
    std::memcpy(&f, data, sizeof f);
    expect(_f(f) == f);
    expect(_f(f) <= f);
    data += sizeof f;
    size -= sizeof f;
  }
  if (size >= 10) {  // x86 long double: 80 significant bits
    long double ld{};
    std::memcpy(&ld, data, 10);
    expect(_ld(ld) == ld);
  }
  return 0;
}
