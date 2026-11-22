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
chmod -R u+w "$WORK/stage"
DIST="$WORK/stage"

scrub_runpath() {
  local f="$1" old entry new="" had_abs=0 forced=0
  shift
  old="$(patchelf --print-rpath "$f" 2>/dev/null)" || return 0
  [ "$#" -gt 0 ] && forced=1
  local IFS=:

  for entry in "$@" $old; do
    case "$entry" in
      '$ORIGIN'*|'${ORIGIN}'*) ;;
      /*) had_abs=1; continue ;;
      *) continue ;;
    esac
    case ":$new:" in *":$entry:"*) ;; *) new="${new:+$new:}$entry" ;; esac
  done
  if [ "$had_abs" -eq 0 ] && [ "$forced" -eq 0 ]; then return 0; fi
  if [ -z "$new" ]; then

    case "$(basename "$(dirname "$f")")" in
      lib) new='$ORIGIN' ;;
      *)   new='$ORIGIN/../lib' ;;
    esac
  fi
  patchelf --set-rpath "$new" "$f"
  echo "RUNPATH of $(basename "$f"): $new"
}

for f in "$DIST"/lib/*.so*; do
  [ -f "$f" ] && [ ! -L "$f" ] || continue
  case "$(basename "$f")" in
    libwconr*) scrub_runpath "$f" '$ORIGIN' '$ORIGIN/winconveyor/lib' ;;
    *)         scrub_runpath "$f" ;;
  esac
done

for b in $CLI_BINS $DEVTOOLS_BINS; do
  [ -f "$DIST/bin/$b" ] && scrub_runpath "$DIST/bin/$b"
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

TEMPLATE="$HERE/installer.sh.in"
CLEAN="$WORK/template.sh"

tr -d '\r' < "$TEMPLATE" > "$CLEAN"

if ! head -n 1 "$CLEAN" | grep -q '^#!/bin/sh'; then
  echo "error: $TEMPLATE must start with '#!/bin/sh'. First lines are:" >&2
  head -n 3 "$CLEAN" >&2
  exit 1
fi
if ! grep -q '^VERSION="@VERSION@"' "$CLEAN" || ! grep -qx '__ARCHIVE__' "$CLEAN"; then
  echo "error: $TEMPLATE is not the WinConveyoR installer template" >&2
  echo "       (missing the VERSION=\"@VERSION@\" line or the __ARCHIVE__ marker)." >&2
  echo "       Re-copy installer/linux/installer.sh.in from the delivered files." >&2
  exit 1
fi
if grep -q 'BASH_SOURCE' "$CLEAN"; then
  echo "error: $TEMPLATE contains bash-only code; it must run under plain sh (dash)." >&2
  exit 1
fi

HEADER="$WORK/header.sh"
sed "s/@VERSION@/$VERSION/g" "$CLEAN" > "$HEADER"
sh -n "$HEADER" || { echo "error: generated installer header has a syntax error" >&2; exit 1; }

RUN="$OUT/WinConveyoR-$VERSION-linux-amd64.run"
cat "$HEADER" > "$RUN"
tar -C "$WORK/payload" -cf - . >> "$RUN"
chmod 755 "$RUN"
echo "Built $RUN"
head -n 2 "$RUN"