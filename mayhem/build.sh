#!/usr/bin/env bash
# mayhem/build.sh — build boost::ut's fuzz harnesses and its own test suite.
# boost::ut is header-only (include/boost/ut.hpp, C++20): there is no library to
# build, so each harness TU instantiates the framework with $SANITIZER_FLAGS.
# Outputs (all under gitignored build-* dirs, recreated on every run):
#   build-fuzz/fuzz_<name>            libFuzzer targets (Mayhemfiles)
#   build-fuzz/fuzz_<name>-standalone run-once reproducers
#   build-fuzz/fuzz_cli                AFL file-input target (CLI)
#   build-tests/{ut_test,ft,gherkin,bdd}  the test suite with normal flags (test.sh runs them)
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"
FUZZ=build-fuzz
TESTS=build-tests
mkdir -p "$FUZZ" "$TESTS"
CXXSTD="-std=c++20 -Iinclude"

# 1+2) harnesses (the framework is header-only, so the harness TU IS the project build)
# shellcheck disable=SC2086
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.cc -o "$FUZZ/lsan_off.o"
# shellcheck disable=SC2086
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$FUZZ/standalone_main.o"

build_harness() {
  local name="$1"
  # shellcheck disable=SC2086
  $CXX $CXXSTD -O1 $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link \
      -c "mayhem/harnesses/$name.cpp" -o "$FUZZ/$name.o"
  # shellcheck disable=SC2086
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$FUZZ/$name.o" "$FUZZ/lsan_off.o" -o "$FUZZ/$name"
  # shellcheck disable=SC2086
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "$FUZZ/$name.o" "$FUZZ/standalone_main.o" "$FUZZ/lsan_off.o" \
      -o "$FUZZ/$name-standalone"
}
pids=()
for h in fuzz_gherkin fuzz_float fuzz_match; do
  build_harness "$h" & pids+=($!)
done
# fuzz_cli: file-input AFL target (upstream's CLI parser std::exit()s on bad
# options, so one process per input); its own reproducer, no -standalone.
: "${AFL_CXX:=afl-clang-fast++}"
# shellcheck disable=SC2086
AFL_QUIET=1 $AFL_CXX $CXXSTD -O1 $SANITIZER_FLAGS $DEBUG_FLAGS \
    mayhem/harnesses/fuzz_cli.cpp "$FUZZ/lsan_off.o" -o "$FUZZ/fuzz_cli" & pids+=($!)

# 3) upstream's own tests, NORMAL flags (no sanitizers): test/ut (framework
#    self-test), test/ft (functional suites), and the gherkin/BDD examples.
build_test() {
  local out="$1"; shift
  # shellcheck disable=SC2086
  $CXX $CXXSTD -O1 $COVERAGE_FLAGS "$@" -o "$TESTS/$out"
}
build_test ut_test test/ut/ut.cpp & pids+=($!)
build_test ft -Itest/ft/include test/ft/main.cpp test/ft/test_suite_1.cpp test/ft/test_suite_2.cpp test/ft/test_suite_3.cpp & pids+=($!)
build_test gherkin example/gherkin.cpp & pids+=($!)
build_test bdd example/BDD.cpp & pids+=($!)

rc=0
for p in "${pids[@]}"; do wait "$p" || rc=1; done
exit $rc
