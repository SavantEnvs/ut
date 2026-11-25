// Fuzz the boost::ut test-binary command line: detail::cfg::parse (option
// table lookup, std::stoull of numeric options, query pattern -> regex),
// utility::regex_match / is_match of test names and tags, the runner's
// tag/skip/query selection, --list-tests / --list-tags / --list-test-names-only,
// and the console and junit (-r junit) reporters.
//
// This is a FILE-INPUT target (AFL-instrumented, Mayhemfile `afl: true`),
// not libFuzzer: upstream's parser calls std::exit() on unknown options,
// --help, --libidentify, --list-reporters and --abort, which is its normal
// behavior for a test binary, so each input runs in its own process.
//
// Input: the file named by argv[1]; split on '\n' into the test binary's
// argv[1..] (argv[0] is fixed). The program then does exactly what a test
// binary does (cf. example/run.cpp): cfg<>.run({.argc, .argv}).
//
// One deliberate deviation: the argument of `-o/--out <filename>` (the
// option table entry upstream uses to open an output std::ofstream) is
// replaced by a fixed relative name, so an input cannot make the target
// write to arbitrary paths. Upstream's own find_arg() decides which argv
// entry is that option, so parsing is unchanged otherwise.
#include <boost/ut.hpp>

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace ut = boost::ut;

ut::suite<"calculator"> calc = [] {
  using namespace ut;
  "add"_test = [] { expect(1 + 1 == 2_i); };
  "nested"_test = [] {
    "inner pass"_test = [] { expect(eq(2, 2)); };
    "inner fail"_test = [] { expect(eq(1, 2)) << "deliberately failing"; };
  };
  tag("fast") / "tagged"_test = [] { expect(true); };
  tag("slow") / tag("io") / "multi tag"_test = [] { expect(3_u == 3u); };
  skip / "skipped"_test = [] { expect(false); };
};

ut::suite<"strings"> strs = [] {
  using namespace ut;
  "string.eq"_test = [] { expect(std::string{"a"} == std::string{"a"}); };
  "throws"_test = [] { expect(throws([] { throw 1; })); };
  "exception"_test = [] { throw std::runtime_error("boom"); };
  "floats"_test = [] { expect(_d(1.5) == 1.5_d); };
};

int main(int argc, char** argv) {
  if (argc < 2) return 1;
  std::ifstream in(argv[1], std::ios::binary);
  if (!in) return 1;
  std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

  std::vector<std::string> args{"fuzz_cli"};
  std::string cur;
  for (char c : data) {
    if (c == '\n') {
      args.push_back(cur);
      cur.clear();
    } else {
      cur += c;
    }
  }
  if (!cur.empty()) args.push_back(cur);

  // Neutralize the -o/--out filename (see header), mirroring parse()'s
  // argument consumption: an option with an argument consumes the next entry.
  for (std::size_t i = 1; i < args.size(); ++i) {
    auto opt = ut::detail::cfg::find_arg(args[i]);
    if (!opt) continue;
    const auto& ref = std::get<ut::detail::cfg::value_ref>(*opt);
    if (std::get<1>(*opt).empty() &&
        std::holds_alternative<std::reference_wrapper<bool>>(ref))
      continue;
    if (i + 1 < args.size()) {
      if (std::get<0>(*opt) == "-o, --out") args[i + 1] = "ut-fuzz-out.xml";
      ++i;
    }
  }

  std::vector<const char*> cargv;
  for (const auto& a : args) cargv.push_back(a.c_str());
  cargv.push_back(nullptr);
  const int n = static_cast<int>(args.size());
  (void)ut::cfg<>.run({.argc = n, .argv = cargv.data()});
  return 0;
}
