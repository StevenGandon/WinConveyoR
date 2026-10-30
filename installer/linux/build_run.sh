#!/usr/bin/env bash

set -euo pipefail

DIST="${1:?usage: build_run.sh <dist-dir> <version> <out-dir>}"
VERSION="${2:?missing version}"
OUT="${3:?missing output dir}"

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"

. "$HERE/../components.sh"

DIST="$(cd "$DIST" && pwd)"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"

[ -f "$REPO/LICENSE" ] || { echo "LICENSE not found at repository root" >&2; exit 1; }
command -v patchelf >/dev/null 2>&1 || { echo "patchelf is required (apt-get install patchelf)" >&2; exit 1; }

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$WORK/payload"

cp -a "$DIST/." "$WORK/stage"
DIST="$WORK/stage"

fix_runpath() {
  local f="$1" old entry new=""
  old="$(patchelf --print-rpath "$f" 2>/dev/null || true)"
  local IFS=:

  for entry in '$ORIGIN' '$ORIGIN/winconveyor/lib' $old; do
    case ":$new:" in *":$entry:"*) ;; *) new="${new:+$new:}$entry" ;; esac
  done
  patchelf --set-rpath "$new" "$f"
  echo "RUNPATH of $(basename "$f"): $new"
}
for f in "$DIST"/lib/libwconr*.so*; do
  if [ -f "$f" ] && [ ! -L "$f" ]; then fix_runpath "$f"; fi
done

pack() {
  local name="$1"; shift
  local p
  for p in "$@"; do
    [ -e "$DIST/$p" ] || { echo "missing build output: $DIST/$p" >&2; exit 1; }
  done
  tar -C "$DIST" --owner=0 --group=0 --numeric-owner -czf "$WORK/payload/$name.tar.gz" "$@"
}

files_for() {
  local b
  for b in "$@"; do echo "bin/$b"; done
}

pack cli      $(files_for $CLI_BINS) lib

pack server   $(files_for $SERVER_BINS)

pack devtools $(files_for $DEVTOOLS_BINS) include

cp "$REPO/LICENSE" "$WORK/payload/LICENSE"

RUN="$OUT/WinConveyoR-$VERSION-linux-amd64.run"
sed "s/@VERSION@/$VERSION/g" "$HERE/installer.sh.in" > "$RUN"
tar -C "$WORK/payload" -cf - . >> "$RUN"
chmod 755 "$RUN"
echo "Built $RUN"