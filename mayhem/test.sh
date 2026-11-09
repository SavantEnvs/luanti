#!/usr/bin/env bash
#
# mayhem/test.sh — RUN the glTF loader's known-answer checks (bin/gltf_oracle, built by mayhem/build.sh
# with the normal flags). It loads the devtest glTF/GLB models and compares vertex positions, normals,
# texture coordinates, indices, skin joints and animation keyframes with the values the project's own
# test (src/test/client/test_irr_gltf_mesh_loader.cpp) asserts, and expects the invalid models to be
# rejected. exit 0 = pass.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
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
    "$tool" "$tests" "$passed" "$failed" "$skipped" "$pending" "$other"
  [ "$failed" -eq 0 ]
}

BIN="$SRC/bin/gltf_oracle"
[ -x "$BIN" ] || { echo "missing $BIN: mayhem/build.sh did not produce the oracle" >&2; exit 1; }

OUT=$(mktemp)
timeout 600 "$BIN" "$SRC/games/devtest/mods/gltf/models" > "$OUT" 2>&1
rc=$?
cat "$OUT"

passed=$(grep -c '^PASS ' "$OUT")
failed=$(grep -c '^FAIL ' "$OUT")
# 12 checks run; an oracle that ran fewer (crash, exit, shim) must not pass.
if [ "$rc" -ne 0 ] || [ $(( passed + failed )) -lt 12 ]; then
  echo "oracle ran $(( passed + failed )) of 12 checks (exit status $rc)" >&2
  [ "$failed" -gt 0 ] || failed=1
fi
emit_ctrf "luanti-gltf-oracle" "$passed" "$failed" 0
