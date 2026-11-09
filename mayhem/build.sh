#!/usr/bin/env bash
#
# mayhem/build.sh — builds Luanti's glTF/GLB mesh loader (irr/src/CGLTFMeshFileLoader.cpp and the few
# Irrlicht sources it needs: SkinnedMesh, WeightBuffer, memory file, logger, os; plus tiniergltf, jsoncpp
# and util/base64) twice: sanitized into the libFuzzer harness + a -standalone reproducer, and with the
# project's normal flags into the known-answer oracle that test.sh runs. The loader is compiled directly,
# not through CMake: Irrlicht is only part of the client build, which needs SDL/OpenGL. Runs inside the
# commit image as `mayhem` in /mayhem; re-runnable offline (no downloads).
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS

cd "$SRC"

INC=(-Iirr/include -Iirr/src -Ilib/tiniergltf -Ilib/jsoncpp -Isrc)
# No _IRR_COMPILE_WITH_SDL_DEVICE_: irr/src/os.cpp would then include SDL's endian header.
DEFS=(-D_IRR_POSIX_API_)
SOURCES=(irr/src/CGLTFMeshFileLoader.cpp irr/src/SkinnedMesh.cpp irr/src/WeightBuffer.cpp
	irr/src/CMemoryFile.cpp irr/src/HWBuffer.cpp irr/src/CLogger.cpp irr/src/os.cpp
	src/util/base64.cpp lib/jsoncpp/jsoncpp.cpp)

OBJ=/tmp/luanti-gltf-obj
rm -rf "$OBJ"
mkdir -p "$OBJ/fuzz" "$OBJ/test" "$SRC/bin"

compile() { # <outdir> <flags> <source> ...
	local out="$1" flags="$2"; shift 2
	local s
	for s in "$@"; do
		# shellcheck disable=SC2086
		"$CXX" -std=c++17 -O1 $flags "${DEFS[@]}" "${INC[@]}" -c "$s" -o "$out/$(basename "$s").o" &
		while [ "$(jobs -rp | wc -l)" -ge "$MAYHEM_JOBS" ]; do wait -n || true; done
	done
	wait || true
	# a failed compile leaves no object behind
	for s in "$@"; do [ -s "$out/$(basename "$s").o" ] || { echo "compiling $s failed" >&2; exit 1; }; done
}

# 1) Sanitized build. -fsanitize=fuzzer-no-link adds SanCov to all the loader code.
FUZZ_FLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link"
compile "$OBJ/fuzz" "$FUZZ_FLAGS" "${SOURCES[@]}" mayhem/harnesses/fuzz_gltf.cpp

# Standalone driver as a C object so its LLVMFuzzerTestOneInput reference keeps C linkage.
"$CC" $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OBJ/standalone_main.o"
"$CXX" $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.cc -o "$OBJ/lsan_off.o"

LIBOBJS=()
for s in "${SOURCES[@]}"; do LIBOBJS+=("$OBJ/fuzz/$(basename "$s").o"); done
"$CXX" $SANITIZER_FLAGS "$OBJ/fuzz/fuzz_gltf.cpp.o" "${LIBOBJS[@]}" "$OBJ/lsan_off.o" $LIB_FUZZING_ENGINE -o "$SRC/fuzz_gltf"
"$CXX" $SANITIZER_FLAGS "$OBJ/fuzz/fuzz_gltf.cpp.o" "${LIBOBJS[@]}" "$OBJ/lsan_off.o" "$OBJ/standalone_main.o" -o "$SRC/fuzz_gltf-standalone"

# 2) Test/oracle build: the project's NORMAL flags. Output: $SRC/bin/gltf_oracle (gitignored).
compile "$OBJ/test" "" "${SOURCES[@]}" mayhem/harnesses/gltf_oracle.cpp
TESTOBJS=()
for s in "${SOURCES[@]}"; do TESTOBJS+=("$OBJ/test/$(basename "$s").o"); done
"$CXX" "$OBJ/test/gltf_oracle.cpp.o" "${TESTOBJS[@]}" -o "$SRC/bin/gltf_oracle"
