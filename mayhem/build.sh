#!/usr/bin/env bash
#
# mayhem/build.sh — builds Luanti's dedicated server twice (normal flags for the unit-test oracle,
# sanitized for the fuzz harnesses) and links one libFuzzer binary + one -standalone reproducer per
# harness in mayhem/harnesses/. Runs inside the commit image as `mayhem` in /mayhem; re-runnable offline.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

TARGETS="fuzz_mapblock fuzz_nodemeta fuzz_defs fuzz_schematic fuzz_settings fuzz_serialization"

# The compilers come from the exported CC/CXX only on the first configure: passing -DCMAKE_C_COMPILER
# would make a re-run (PATH without the image build's ccache wrappers) look like a changed compiler,
# and CMake then wipes the cache and re-configures with the defaults.

# Server-only configuration shared by both builds: no client/irrlicht/SDL, no network deps (curl,
# gettext, sound...), bundled Lua/jsoncpp/gmp. Only zlib, zstd and sqlite3 come from apt.
CMAKE_COMMON=(-G Ninja -DCMAKE_BUILD_TYPE=Release
	-DBUILD_CLIENT=0 -DBUILD_SERVER=1 -DBUILD_UNITTESTS=1 -DBUILD_BENCHMARKS=0 -DBUILD_DOCUMENTATION=0
	-DENABLE_CURL=0 -DENABLE_CURSES=0 -DENABLE_GETTEXT=0 -DENABLE_LEVELDB=0 -DENABLE_LUAJIT=0
	-DENABLE_OPENSSL=0 -DENABLE_POSTGRESQL=0 -DENABLE_PROMETHEUS=0 -DENABLE_REDIS=0 -DENABLE_SOUND=0
	-DENABLE_SPATIAL=0 -DENABLE_UPDATE_CHECKER=0 -DENABLE_LTO=0)

# Both trees link their luantiserver into $SRC/bin/ (Luanti's CMake hardcodes that for native builds), so
# the sanitized tree is built FIRST and the normal-flags oracle tree LAST: bin/luantiserver, which
# test.sh runs, is then always the unsanitized one, including on re-runs where both trees are up to date.

# 1) Sanitized build of the whole engine; the harnesses are linked from its object files.
#    -fsanitize=fuzzer-no-link adds SanCov to all engine code so libFuzzer sees the fuzzed edges.
FUZZ_FLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link"
cmake -S . -B build/fuzz "${CMAKE_COMMON[@]}" -DRUN_IN_PLACE=1 \
	-DCMAKE_C_FLAGS="$FUZZ_FLAGS" -DCMAKE_CXX_FLAGS="$FUZZ_FLAGS"
cmake --build build/fuzz -j"$MAYHEM_JOBS" --target luantiserver

# 2) Test/oracle build: the project's NORMAL flags. Output: $SRC/bin/luantiserver (gitignored).
cmake -S . -B build/test "${CMAKE_COMMON[@]}" -DRUN_IN_PLACE=1 \
	-DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS"
cmake --build build/test -j"$MAYHEM_JOBS" --target luantiserver

# 3) Harnesses. Reuse the build system's own compile and link lines (include paths, defines, the
#    object list, static libs) from ninja so nothing is hand-copied; only main.cpp.o and the output
#    are swapped for the harness.
cd build/fuzz
ninja -t commands luantiserver > /tmp/luanti-commands.txt
COMPILE_LINE=$(grep -E ' -c [^ ]*/src/main\.cpp$' /tmp/luanti-commands.txt | head -n1)
LINK_LINE=$(grep -E '^: && .* -o [^ ]*/luantiserver ' /tmp/luanti-commands.txt | head -n1)
[ -n "$COMPILE_LINE" ] && [ -n "$LINK_LINE" ]

read -r -a CT <<< "$COMPILE_LINE"
COMPILE=()
i=0
while [ $i -lt ${#CT[@]} ]; do
	case "${CT[$i]}" in
		-MD) ;;
		-MT|-MF|-o|-c) i=$((i+1)) ;;
		*) COMPILE+=("${CT[$i]}") ;;
	esac
	i=$((i+1))
done

read -r -a LT <<< "$LINK_LINE"
LINK=()
i=0
while [ $i -lt ${#LT[@]} ]; do
	case "${LT[$i]}" in
		:|'&&') ;;
		-o) i=$((i+1)) ;;
		-Xlinker) i=$((i+1)) ;;
		*/main.cpp.o) ;;
		*) LINK+=("${LT[$i]}") ;;
	esac
	i=$((i+1))
done

# Standalone driver as a C object so its LLVMFuzzerTestOneInput reference keeps C linkage.
"$CC" $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o /tmp/standalone_main.o
"$CXX" $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o /tmp/lsan_off.o

for t in $TARGETS; do
	"${COMPILE[@]}" -I"$SRC/mayhem/harnesses" -c "$SRC/mayhem/harnesses/$t.cpp" -o "/tmp/$t.o"
	"${LINK[@]}" "/tmp/$t.o" /tmp/lsan_off.o $LIB_FUZZING_ENGINE -o "$SRC/$t"
	"${LINK[@]}" "/tmp/$t.o" /tmp/lsan_off.o /tmp/standalone_main.o -o "$SRC/$t-standalone"
done
