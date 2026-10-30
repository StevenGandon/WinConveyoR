#!/usr/bin/env bash

set -euo pipefail

STAGE="${1:?usage: relocate_dylibs.sh <stage-dir> <bin>...}"
shift
BINS=("$@")
INSTALL_LIB="${INSTALL_LIB:-/usr/local/winconveyor/lib}"

is_macho() { file "$1" | grep -q 'Mach-O'; }

deps_of() {
  if [[ "$1" == *.dylib ]]; then
    otool -L "$1" | tail -n +3 | awk '{print $1}'
  else
    otool -L "$1" | tail -n +2 | awk '{print $1}'
  fi
}

resolve_bundled() {
  local base stripped c
  base="${1##*/}"
  stripped="$(printf '%s' "$base" | sed -E 's/\.[0-9]+(\.[0-9]+)*\.dylib$/.dylib/')"
  for c in "$base" "$stripped"; do
    if [ -f "$STAGE/lib/$c" ]; then echo "$c"; return 0; fi
  done
  return 0
}

fix_one() {
  local f="$1" dep name target want current changed=0
  is_macho "$f" || return 0

  if [[ "$f" == *.dylib ]]; then
    want="$INSTALL_LIB/${f##*/}"
    current="$(otool -D "$f" | tail -n +2)"
    if [ "$current" != "$want" ]; then
      install_name_tool -id "$want" "$f"
      changed=1
    fi
  fi

  while read -r dep; do
    [ -n "$dep" ] || continue
    case "$dep" in /usr/lib/*|/System/*) continue ;; esac

    name="$(resolve_bundled "$dep")"
    if [ -z "$name" ]; then
      if [[ "$dep" == /* ]] && [ -f "$dep" ]; then
        name="${dep##*/}"
        echo "  bundling external dependency: $dep"
        cp -L "$dep" "$STAGE/lib/$name"
        chmod u+w "$STAGE/lib/$name"
      else
        echo "error: ${f#"$STAGE"/} depends on '$dep', which is neither a system library nor in lib/" >&2
        echo "       lib/ contains: $(ls "$STAGE/lib" | tr '\n' ' ')" >&2
        exit 1
      fi
    fi

    target="$INSTALL_LIB/$name"
    if [ "$dep" != "$target" ]; then
      install_name_tool -change "$dep" "$target" "$f"
      changed=1
    fi
  done < <(deps_of "$f")

  if [ "$changed" -eq 1 ]; then
    codesign --force --sign - "$f"
    echo "  relocated ${f#"$STAGE"/}"
  fi
}

process_all() {
  local f b
  for f in "$STAGE"/lib/*.dylib; do
    if [ -f "$f" ] && [ ! -L "$f" ]; then fix_one "$f"; fi
  done
  for b in "${BINS[@]}"; do
    if [ -f "$STAGE/bin/$b" ]; then fix_one "$STAGE/bin/$b"; fi
  done
}

for _ in 1 2 3 4 5 6 7 8; do
  before="$(ls "$STAGE/lib" | wc -l)"
  process_all
  after="$(ls "$STAGE/lib" | wc -l)"
  if [ "$before" -eq "$after" ]; then break; fi
done

status=0
verify_one() {
  local f="$1" dep
  is_macho "$f" || return 0
  while read -r dep; do
    [ -n "$dep" ] || continue
    case "$dep" in
      /usr/lib/*|/System/*) ;;
      "$INSTALL_LIB"/*)
        if [ ! -f "$STAGE/lib/${dep##*/}" ]; then
          echo "error: ${f#"$STAGE"/} -> $dep is not in lib/" >&2; status=1
        fi ;;
      *) echo "error: ${f#"$STAGE"/} still depends on '$dep'" >&2; status=1 ;;
    esac
  done < <(deps_of "$f")
}
for f in "$STAGE"/lib/*.dylib; do
  if [ -f "$f" ] && [ ! -L "$f" ]; then verify_one "$f"; fi
done
for b in "${BINS[@]}"; do
  if [ -f "$STAGE/bin/$b" ]; then verify_one "$STAGE/bin/$b"; fi
done
if [ "$status" -ne 0 ]; then exit 1; fi
echo "All Mach-O dependencies resolve inside $INSTALL_LIB or the system."