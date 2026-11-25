// Fuzz the boost::ut Gherkin support: bdd::gherkin::parse (feature text:
// tags, tables, doc strings, examples, rules, backgrounds), compile (pickles),
// and the step runner (`steps | text`), which matches steps against
// '{x}' / '*' patterns via utility::match / is_match and lexical_casts the
// captured values. Input = feature file text (as passed to `steps | text` or
// loaded via --feature).
#include "fuzz_common.hpp"

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

namespace ut = boost::ut;

// step definitions must be captureless (steps_t is a function pointer)
static long acc = 0;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  using namespace ut;
  const std::string text(reinterpret_cast<const char*>(data), size);

  // 1) parser + compiler directly
  auto doc = bdd::gherkin::parse(text);
  if (doc.feature) {
    auto pickles = bdd::gherkin::compile(*doc.feature);
    (void)pickles;
  }

  // 2) the step runner, as a test would use it (cf. example/gherkin.cpp)
  ut::cfg<ut::override>.reset();
  acc = 0;
  bdd::gherkin::steps steps = [](auto& s) {
    s.feature("*") = [&] {
      s.rule("*") = [&] {};
      s.scenario("*") = [&] {
        s.given("I have {a} and {b}") = [&](int a, std::string b) {
          acc += a + static_cast<long>(b.size());
        };
        s.given("a value '{v}'") = [&](double v) { acc += v > 0; };
        s.when("I enter {value}") = [&](int v) { acc += v; };
        s.when("I enter the numbers:") =
            [&](const bdd::gherkin::doc_string& d) {
              std::istringstream in{d.content};
              for (int v{}; in >> v;) acc += v;
            };
        s.when("I enter the following values:") =
            [&](const bdd::gherkin::table& t) {
              for (const auto& row : t)
                for (const auto& cell : row) acc += static_cast<long>(cell.size());
            };
        s.when("*") = [&] { ++acc; };
        s.then("I expect {value}") = [&](int r) { expect(that % acc == r); };
        s.step("{x}, {y} and {z}") = [&](float x, unsigned y, char z) {
          acc += static_cast<long>(y) + z + (x > 0);
        };
        s.step("*") = [&] {};
      };
    };
  };
  "gherkin"_test = steps | text;
  return 0;
}
