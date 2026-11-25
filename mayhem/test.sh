#!/usr/bin/env bash
# mayhem/test.sh — run boost::ut's own tests (built by mayhem/build.sh) and
# assert their behavior: exit status AND the framework's own result lines.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=/mayhem}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

T=build-tests
for b in ut_test ft gherkin bdd; do
  [ -x "$T/$b" ] || { echo "missing $T/$b — run mayhem/build.sh first" >&2; exit 1; }
done

passed=0 failed=0
# check <name> <expected exit> <expected stdout line (grep -F -x, after stripping colors)> -- cmd...
check() {
  local name="$1" want_rc="$2" want_line="$3"; shift 4
  local out rc
  out=$("$@" 2>&1); rc=$?
  out=$(printf '%s\n' "$out" | sed 's/\x1b\[[0-9;]*m//g')
  if [ "$rc" -eq "$want_rc" ] && { [ -z "$want_line" ] || printf '%s\n' "$out" | grep -qxF "$want_line"; }; then
    echo "PASS $name"; passed=$((passed + 1))
  else
    echo "FAIL $name (rc=$rc, want rc=$want_rc and line '$want_line')"; printf '%s\n' "$out" | tail -20
    failed=$((failed + 1))
  fi
}

# test/ut/ut.cpp: hundreds of test_assert()s against a fake runner; aborts on the first failure.
# Its exit status is the oracle (std::abort on any failed assertion).
check ut_test 0 "" -- "$T/ut_test"
# test/ft: functional suites through the real runner; known totals.
check ft 0 "All tests passed (6 asserts in 4 tests)" -- "$T/ft"
# example/gherkin.cpp: the Gherkin parser + step runner over an embedded feature and a feature file.
check gherkin_embedded 0 "Suite 'unnamed suite': all tests passed (4 asserts in 1 tests); 1 tests skipped" -- "$T/gherkin" -r console
check gherkin_feature_file 0 "Suite 'unnamed suite': all tests passed (10 asserts in 2 tests); 1 tests skipped" -- "$T/gherkin" -r console --feature example/gherkin.feature
# example/BDD.cpp
check bdd 0 "Suite 'global': all tests passed (3 asserts in 2 tests)" -- "$T/bdd" -r console
# --list-test-names: CLI option parsing + test enumeration
check gherkin_list 0 "UT starts =====================================================================Calculator" -- "$T/gherkin" -r console --list-test-names

emit_ctrf "boost-ut" "$passed" "$failed" 0
