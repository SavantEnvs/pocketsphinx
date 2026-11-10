#!/usr/bin/env bash
#
# mayhem/build.sh — build the PocketSphinx fuzz harnesses and the project's own test suite.
# Runs inside the commit image as `mayhem` in /mayhem (see mayhem/Dockerfile). Re-runnable and offline:
# everything comes from the checkout and apt packages baked into the image.
set -euo pipefail

# clang rejects SOURCE_DATE_EPOCH='' (empty) — it must be unset or a valid integer.
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

# Both build trees live under build/, which upstream's .gitignore covers (so `git clean -ffdX` removes them).
FUZZ_DIR="$SRC/build/fuzz"
TEST_DIR="$SRC/build/tests"

# Instrument the library for coverage (fuzzer-no-link) unconditionally, even with the sanitizers off.
# -fno-sanitize=shift-base: relaxed library-wide (UBSan flags left shifts of negative values in upstream's
# fixed-point code; ASan and every other UBSan check stay halting).
# mayhem/webrtc_ubsan.cmake additionally drops signed-integer-overflow for the vendored WebRTC DSP only.
LIB_CFLAGS="-O2 $SANITIZER_FLAGS -fno-sanitize=shift-base -fsanitize=fuzzer-no-link $DEBUG_FLAGS"

# 1) The project itself, sanitized + coverage-instrumented, DWARF 3.
cmake -S . -B "$FUZZ_DIR" -G Ninja \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_C_FLAGS="$LIB_CFLAGS" \
    -DCMAKE_EXE_LINKER_FLAGS="$SANITIZER_FLAGS" \
    -DCMAKE_PROJECT_INCLUDE="$SRC/mayhem/webrtc_ubsan.cmake" \
    -DBUILD_TESTING=OFF
cmake --build "$FUZZ_DIR" --target pocketsphinx -j"$MAYHEM_JOBS"

# 2) Each harness twice: libFuzzer binary + standalone run-once reproducer. LeakSanitizer is switched
#    off at build time by lsan_off.cc (ASan and UBSan stay fully active).
mkdir -p "$FUZZ_DIR/obj"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c -x c "$STANDALONE_FUZZ_MAIN" -o "$FUZZ_DIR/obj/standalone_main.o"
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$FUZZ_DIR/obj/lsan_off.o"

HARNESS_INC="-I$SRC/include -I$FUZZ_DIR/include -I$SRC/src -I$FUZZ_DIR -I$SRC/mayhem/harnesses"
TARGETS="fuzz_decode_raw fuzz_ngram_read fuzz_jsgf fuzz_fsg_read fuzz_lattice_read fuzz_endpointer \
fuzz_config_json fuzz_soundfile fuzz_mllr fuzz_mdef fuzz_dict fuzz_senscr"
for t in $TARGETS; do
    src="$SRC/mayhem/harnesses/$t.c"
    $CC -O2 $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE $HARNESS_INC -DMODELDIR="\"$SRC/model\"" \
        "$src" "$FUZZ_DIR/obj/lsan_off.o" "$FUZZ_DIR/libpocketsphinx.a" -lm -o "$SRC/$t"
    $CC -O2 $SANITIZER_FLAGS $DEBUG_FLAGS $HARNESS_INC -DMODELDIR="\"$SRC/model\"" \
        "$src" "$FUZZ_DIR/obj/standalone_main.o" "$FUZZ_DIR/obj/lsan_off.o" \
        "$FUZZ_DIR/libpocketsphinx.a" -lm -o "$SRC/$t-standalone"
done

# 3) The project's own test suite, with its NORMAL flags (no sanitizers, no -gdwarf-3), so test.sh only runs it.
cmake -S . -B "$TEST_DIR" -G Ninja \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" \
    -DCMAKE_EXE_LINKER_FLAGS="$COVERAGE_FLAGS"
# The unit-test executables are EXCLUDE_FROM_ALL; build every executable target ninja knows about.
exe_targets=$(ninja -C "$TEST_DIR" -t targets all | sed -n 's/^\([A-Za-z0-9_]*\): C\(XX\)\{0,1\}_EXECUTABLE_LINKER.*/\1/p')
# shellcheck disable=SC2086
cmake --build "$TEST_DIR" -j"$MAYHEM_JOBS" --target $exe_targets
