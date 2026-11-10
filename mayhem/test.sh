#!/usr/bin/env bash
#
# mayhem/test.sh — RUN pocketsphinx's OWN functional tests (already built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
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

TEST_DIR="$SRC/build/tests"
[ -x "$TEST_DIR/pocketsphinx" ] || { echo "missing $TEST_DIR/pocketsphinx: build.sh did not build the tests" >&2; exit 2; }
export POCKETSPHINX_PATH="$SRC/model"
M="$SRC/model/en-us"
D="$SRC/test/data"

passed=0; failed=0; skipped=0

# 1) The project's ctest suite (unit tests + regression scripts). test-main.sh and test_vad shell out to
#    sox, which the image does not ship, so they are excluded and counted as skipped.
log=$(mktemp)
(cd "$TEST_DIR" && timeout 1500 ctest -j"$MAYHEM_JOBS" -E '^(test-main\.sh|test_vad)$' 2>&1) | tee "$log" | tail -15
summary=$(sed -n 's/.*tests passed, \([0-9]*\) tests failed out of \([0-9]*\).*/\1 \2/p' "$log" | tail -1)
if [ -z "$summary" ]; then
  echo "ctest produced no summary" >&2; failed=$((failed + 1))
else
  set -- $summary
  ctest_failed=$1; ctest_total=$2
  failed=$((failed + ctest_failed)); passed=$((passed + ctest_total - ctest_failed))
fi
rm -f "$log"
skipped=$((skipped + 2))

# 2) Known-answer probes through the built binaries (dynamically linked, so a no-op'd program cannot pass).
probe() { # <name> <expected> <actual>
  if [ "$2" = "$3" ]; then passed=$((passed + 1)); echo "PASS probe $1"
  else failed=$((failed + 1)); echo "FAIL probe $1: expected [$2] got [$3]"; fi
}
decode() {
  "$TEST_DIR/pocketsphinx" -loglevel FATAL -hmm "$M/en-us" -lm "$M/en-us.lm.bin" \
      -dict "$M/cmudict-en-us.dict" -samprate 16000 single "$1" 2>/dev/null |
    sed -n 's/^{"b":[^{]*"t":"\([^"]*\)","w":.*/\1/p' | head -1
}
probe decode-goforward "go forward ten meters" "$(decode "$D/goforward.raw")"
probe decode-numbers "thirty three four or six ninety two" "$(decode "$D/numbers.raw")"
probe decode-something "go somewhere and do something" "$(decode "$D/something.raw")"

arpa=$(mktemp --suffix=.arpa)
"$TEST_DIR/pocketsphinx_lm_convert" -i "$D/turtle.lm.bin" -o "$arpa" >/dev/null 2>&1
probe lm-convert-counts "ngram 1=91 ngram 2=212 ngram 3=177" "$(grep '^ngram ' "$arpa" | tr '\n' ' ' | sed 's/ $//')"
probe lm-convert-unigram "-0.9129	</s>	-0.3010" "$(sed -n '/^\\1-grams:/{n;p}' "$arpa")"
rm -f "$arpa"

fsg=$("$TEST_DIR/pocketsphinx_jsgf2fsg" -jsgf "$D/goforward.gram" 2>/dev/null | grep -E '^(NUM_STATES|TRANSITION)' | tr '\n' ';')
probe jsgf2fsg "NUM_STATES 6;TRANSITION 0 2 1.000000 go;TRANSITION 2 3 1.000000 forward;TRANSITION 3 4 1.000000 ten;TRANSITION 4 5 1.000000 meters;TRANSITION 5 1 1.000000 ;" "$fsg"

emit_ctrf "ctest+kat" "$passed" "$failed" "$skipped"
