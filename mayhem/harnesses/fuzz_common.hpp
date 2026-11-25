// Shared fuzz-harness glue for boost::ut (header-only test framework).
// The framework registers tests into a global runner (cfg<override>) and
// reports to a console/junit reporter that buffers output for the whole
// process. For an in-process fuzz loop we specialize cfg<override> (the
// framework's documented customization point) with a runner whose reporter
// discards events, so each input's result depends only on that input and
// memory does not grow across iterations.
#pragma once
#include <boost/ut.hpp>

namespace fuzz {
struct quiet_reporter {
  quiet_reporter() = default;
  explicit(false) quiet_reporter(const boost::ut::colors&) {}
  template <class... Ts>
  auto on(const Ts&...) -> void {}
};

struct runner : boost::ut::runner<quiet_reporter> {
  using boost::ut::runner<quiet_reporter>::operator=;
  runner() { run_ = true; }  // tests run eagerly; nothing deferred to exit
  auto reset() -> void {
    fails_ = 0;
    level_ = 0;
  }
};
}  // namespace fuzz

template <>
auto boost::ut::cfg<boost::ut::override> = fuzz::runner{};
