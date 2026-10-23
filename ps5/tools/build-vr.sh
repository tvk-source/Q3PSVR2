#!/usr/bin/env bash
set -euo pipefail
ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
hui=${PS5_HUI_DIR:-$HOME/dev/ps5-homebrew-ui}
gl=${PS5_OPENGL_PREFIX:-$HOME/dev/ps5-opengl/build/sdk/ps5-opengl-gl46}
sdk=${PS5_PAYLOAD_SDK:-$hui/.deps/native/ps5-payload-sdk}
build=${PS5_VR_BUILD_DIR:-$root/build/ps5-vr}
ninja=$(command -v ninja || true)
[[ -n $ninja ]] || { echo "ninja not found" >&2; exit 2; }

[[ -f $gl/manifest.sha256 ]] || { echo "OpenGL SDK missing: $gl" >&2; exit 2; }
PS5_OPENGL_PREFIX="$gl" make -C "$hui" opengl

# Keep this build tree separate from the original Vulkan one.
if [[ ! -f $build/build.ninja ]]; then
  mkdir -p "$build"
  cmake -S "$ps5" -B "$build" -G Ninja \
    -DCMAKE_MAKE_PROGRAM="$ninja" \
    -DCMAKE_TOOLCHAIN_FILE="$sdk/toolchain/prospero.cmake" \
    -DQUAKE3E_DIR="$root/vendor/Quake3e" \
    -DPS5_HUI_DIR="$hui" \
    -DPS5_OPENGL_PREFIX="$gl" \
    -DPS5OpenGL_DIR="$gl/lib/cmake/PS5OpenGL" \
    -DPS5_PAYLOAD_SDK="$sdk" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=OFF \
    > "$build/configure.log" 2>&1 || { tail -120 "$build/configure.log" >&2; exit 1; }
fi
"$ninja" -C "$build" title
