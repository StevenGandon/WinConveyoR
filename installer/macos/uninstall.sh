#!/bin/bash

set -u
ROOT=/usr/local/winconveyor

if [ "$(id -u)" -ne 0 ]; then
  echo "Please run with sudo: sudo $0" >&2
  exit 1
fi

if [ "${1:-}" != "--yes" ]; then
  printf 'Remove WinConveyoR from this Mac? [y/N] '
  read -r answer || answer=n
  case "$answer" in [Yy]*) ;; *) echo "Aborted."; exit 1 ;; esac
fi

# Remove the symlinks we created in the system directories.
for dir in /usr/local/bin /usr/local/lib /usr/local/include; do
  [ -d "$dir" ] || continue
  find "$dir" -maxdepth 1 -type l | while read -r link; do
    case "$(readlink "$link")" in
      *winconveyor/*) rm -f "$link" ;;
    esac
  done
done

rm -rf "$ROOT"

for id in com.winconveyor.cli com.winconveyor.server com.winconveyor.devtools; do
  pkgutil --forget "$id" >/dev/null 2>&1 || true
done
echo "WinConveyoR has been removed."
