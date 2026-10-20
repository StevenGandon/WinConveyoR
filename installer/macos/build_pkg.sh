set -euo pipefail

DIST="${1:?usage: build_pkg.sh <dist-dir> <version> <out-dir>}"
VERSION="${2:?missing version}"
OUT="${3:?missing output dir}"

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"

. "$HERE/../components.sh"

DIST="$(cd "$DIST" && pwd)"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
NUMERIC="${VERSION%%-*}"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
HOME_DIR="usr/local/winconveyor"

[ -f "$REPO/LICENSE" ] || { echo "LICENSE not found at repository root" >&2; exit 1; }

new_root() {
  local r="$WORK/$1"
  mkdir -p "$r/usr/local/bin" "$r/$HOME_DIR/bin"
  echo "$r"
}

add_bins() {
  local r="$1"; shift
  local b
  for b in "$@"; do
    [ -f "$DIST/bin/$b" ] || { echo "missing build output: $DIST/bin/$b" >&2; exit 1; }
    install -m 755 "$DIST/bin/$b" "$r/$HOME_DIR/bin/$b"
    ln -s "../winconveyor/bin/$b" "$r/usr/local/bin/$b"
  done
}

CLI="$(new_root cli)"

add_bins "$CLI" $CLI_BINS
mkdir -p "$CLI/$HOME_DIR/lib" "$CLI/usr/local/lib"
cp -R "$DIST/lib/." "$CLI/$HOME_DIR/lib/"
for f in "$CLI/$HOME_DIR/lib/libwconr"*.dylib; do
  if [ -e "$f" ]; then
    ln -s "../winconveyor/lib/$(basename "$f")" "$CLI/usr/local/lib/$(basename "$f")"
  fi
done
install -m 755 "$HERE/uninstall.sh" "$CLI/$HOME_DIR/uninstall.sh"
install -m 644 "$REPO/LICENSE" "$CLI/$HOME_DIR/LICENSE.txt"

SRV="$(new_root server)"

add_bins "$SRV" $SERVER_BINS

DEV="$(new_root devtools)"

add_bins "$DEV" $DEVTOOLS_BINS
mkdir -p "$DEV/$HOME_DIR/include" "$DEV/usr/local/include"
cp -R "$DIST/include/." "$DEV/$HOME_DIR/include/"
for e in "$DIST/include/"*; do
  ln -s "../winconveyor/include/$(basename "$e")" "$DEV/usr/local/include/$(basename "$e")"
done

find "$WORK" -name .DS_Store -delete
find "$WORK" -type d -exec chmod 755 {} +

mkdir -p "$WORK/pkgs"
for c in cli server devtools; do
  pkgbuild \
    --root "$WORK/$c" \
    --identifier "com.winconveyor.$c" \
    --version "$NUMERIC" \
    --ownership recommended \
    --install-location / \
    "$WORK/pkgs/$c.pkg"
done

mkdir -p "$WORK/resources"
for h in "$HERE"/resources/*.html; do
  sed -e "s/@VERSION@/$VERSION/g" "$h" > "$WORK/resources/$(basename "$h")"
done
cp "$REPO/LICENSE" "$WORK/resources/license.txt"
sed -e "s/@VERSION@/$VERSION/g" -e "s/@NUMERIC@/$NUMERIC/g" \
  "$HERE/distribution.xml" > "$WORK/distribution.xml"

UNSIGNED="$WORK/product.pkg"
productbuild \
  --distribution "$WORK/distribution.xml" \
  --resources "$WORK/resources" \
  --package-path "$WORK/pkgs" \
  "$UNSIGNED"

FINAL="$OUT/WinConveyoR-$VERSION-macos-arm64.pkg"
if [ -n "${SIGN_IDENTITY:-}" ]; then
  productsign --sign "$SIGN_IDENTITY" "$UNSIGNED" "$FINAL"
else
  echo "SIGN_IDENTITY not set: producing an UNSIGNED installer"
  cp "$UNSIGNED" "$FINAL"
fi
echo "Built $FINAL"
