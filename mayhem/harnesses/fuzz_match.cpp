// Fuzz boost::ut's pattern utilities that every test name, tag, filter and
// gherkin step goes through: utility::match ('{x}' capture extraction used
// by gherkin step definitions), utility::is_match ('*'/'?' glob used by
// --filter / tags / steps), utility::split, and the runner's filter/tag
// selection (cfg<override> = {.filter, .tag}). Input: pattern '\n' text.
#include "fuzz_common.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace ut = boost::ut;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  using namespace ut;
  const std::string in(reinterpret_cast<const char*>(data), size);
  const auto nl = in.find('\n');
  const std::string pattern = in.substr(0, nl);
  const std::string text = nl == std::string::npos ? std::string{} : in.substr(nl + 1);

  auto groups = utility::match(pattern, text);
  (void)groups;
  (void)utility::is_match(text, pattern);
  auto parts = utility::split(text, pattern);
  (void)parts;
  auto sparts = utility::split<std::string>(text, std::string{"."});
  (void)sparts;

  // runner filter + tag selection over nested tests
  ut::cfg<ut::override>.reset();
  ut::cfg<ut::override> = ut::options{.filter = pattern, .tag = {text}};
  int ran = 0;
  "a"_test = [&] {
    ++ran;
    "b"_test = [&] { ++ran; };
    tag(text) / "c"_test = [&] { ++ran; };
  };
  tag("x") / tag("skip") / "d"_test = [&] { ++ran; };
  ut::cfg<ut::override> = ut::options{};
  return 0;
}
