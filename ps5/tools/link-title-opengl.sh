#!/usr/bin/env bash
set -euo pipefail

work=$1 hui=$2 gl=$3 sdk=$4
shift 4
objects=("$@")
ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
param="$ps5/sce_sys/param.json"
native="$hui/tooling/native"
tool="$hui/build/host/ps5-native-tool"
export PS5_CLANG=${PS5_CLANG:-$(command -v clang-18 || command -v clang || true)}

[[ -x $sdk/bin/prospero-lld ]] || { echo "missing SDK: $sdk" >&2; exit 2; }
[[ -f $gl/manifest.sha256 ]] || { echo "missing OpenGL SDK: $gl" >&2; exit 2; }
(cd "$gl" && sha256sum --check --strict --quiet manifest.sha256)

# Build/prepare the same native tooling and OpenGL link group used by ps5-homebrew-ui.
PS5_OPENGL_PREFIX="$gl" make -C "$hui" opengl >/dev/null
bash "$hui/tools/build-host-tools.sh"
[[ -f $hui/runtime/libc.prx ]] || bash "$hui/tools/rebuild-libc.sh"

mkdir -p "$work/link/obj" "$work/link/stubs"
cc(){ PS5_PAYLOAD_SDK="$sdk" PS5_CLANG="$PS5_CLANG" sh "$hui/tooling/prospero-clang18" "$@"; }

for n in app_crt app_cpp_runtime; do
  cc -std=c++20 -O2 -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections \
     -c "$native/$n.cpp" -o "$work/link/obj/$n.o"
done

# Existing Quake platform code uses the console mouse library; keep its import stub.
cc -std=c11 -O2 -fPIC -c "$ps5/stubs/libSceMouse_stub.c" -o "$work/link/obj/libSceMouse_stub.o"
"$sdk/bin/prospero-lld" --shared -soname libSceMouse.prx \
  -o "$work/link/stubs/libSceMouse.so" "$work/link/obj/libSceMouse_stub.o"

ogl_group="$hui/.deps/ps5-opengl/libps5opengl-group.a"
[[ -f $ogl_group ]] || { echo "missing OpenGL link group: $ogl_group" >&2; exit 2; }

wrap=(--wrap=malloc --wrap=calloc --wrap=realloc --wrap=free --wrap=posix_memalign --wrap=malloc_usable_size)

# The PS5's normal libc directory calls cannot reliably enumerate /app0.
# Quake's filesystem uses opendir/readdir to discover baseq3/*.pk3, so bind
# the complete DIR API to the implementations in libps5platform.a.
platform_defs=(
  --defsym=opendir=ps5_opendir
  --defsym=fdopendir=ps5_fdopendir
  --defsym=readdir=ps5_readdir
  --defsym=rewinddir=ps5_rewinddir
  --defsym=dirfd=ps5_dirfd
  --defsym=closedir=ps5_closedir
)

# Keep the replacement libc names internal to the executable.
cat > "$work/link/ps5platform-local.map" <<'MAP'
{
    local:
        opendir;
        fdopendir;
        readdir;
        rewinddir;
        dirfd;
        closedir;
};
MAP
"$sdk/bin/prospero-lld" -T "$native/ps5-pie.ld" --eh-frame-hdr \
  "${wrap[@]}" "${platform_defs[@]}" \
  --version-script "$native/app-symbols.map" \
  --version-script "$work/link/ps5platform-local.map" --exclude-libs=ALL \
  -e _start -o "$work/link/llvm-pie.elf" \
  "$work/link/obj/app_crt.o" "$work/link/obj/app_cpp_runtime.o" \
  "${objects[@]}" "$sdk/target/lib/libps5platform.a" "$ogl_group" \
  --as-needed "$gl/lib/libSceAgc.so" "$gl/lib/libSceAgcDriver.so" \
  "$work/link/stubs/libSceMouse.so" "$sdk"/target/lib/*.so

"$tool" link --in "$work/link/llvm-pie.elf" --out "$work/eboot.elf.new" \
  --stub-dir "$sdk/target/lib" \
  --stub "$gl/lib/libSceAgc.so" --stub "$gl/lib/libSceAgcDriver.so" \
  --stub "$work/link/stubs/libSceMouse.so" \
  --module-sdk 0x02000009 --companion-sdk 0x08050001 --file-name eboot.elf

title_id=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["titleId"])' "$param")
app="$root/dist/$title_id"
mkdir -p "$app/sce_sys" "$app/sce_module" "$app/baseq3"
"$tool" self --sign --in "$work/eboot.elf.new" --out "$app/eboot.bin" --magic 0x1D3D154F
"$tool" self --inspect --file "$app/eboot.bin" >/dev/null
cp "$param" "$app/sce_sys/param.json"
for asset in icon0.png pic0.dds pic1.dds snd0.at9; do
  [[ -f $ps5/sce_sys/$asset ]] && cp "$ps5/sce_sys/$asset" "$app/sce_sys/$asset"
done
cp "$hui/runtime/libc.prx" "$app/sce_module/libc.prx"
cp "$root/vendor/Quake3e/COPYING.txt" "$app/Quake3e-COPYING.txt"
mv "$work/eboot.elf.new" "$work/eboot.elf"
printf '==> PSVR2/OpenGL title: %s (eboot.bin %s bytes)\n' "$app" "$(stat -c %s "$app/eboot.bin")"
