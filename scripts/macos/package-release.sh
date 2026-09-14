#!/bin/bash
# Build a distributable macOS .dmg and .zip from build/Moolticute.app
#
# Usage: ./scripts/macos/package-release.sh [version]
# Example: ./scripts/macos/package-release.sh v1.04.0-arm64

set -eo pipefail

SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPTDIR/../.." && pwd)"
APP=Moolticute
ARCH="$(uname -m)"

if [ -n "${1:-}" ]; then
    VERSION="$1"
else
    VERSION="$(git -C "$REPO_ROOT" describe --tags --abbrev=0 2>/dev/null || echo "dev")"
fi
VERSION="${VERSION#v}"

APP_PATH="$REPO_ROOT/build/${APP}.app"
if [ ! -d "$APP_PATH" ]; then
    echo "Missing $APP_PATH — run ./scripts/macos/build-local.sh --package first"
    exit 1
fi

# Ad-hoc sign so local Gatekeeper may allow after user approval (not notarized).
codesign --force --deep --sign - "$APP_PATH" 2>/dev/null || true

BASENAME="${APP}-${VERSION}-macos-${ARCH}"
DMG_PATH="$REPO_ROOT/build/${BASENAME}.dmg"
ZIP_PATH="$REPO_ROOT/build/${BASENAME}.zip"

rm -f "$DMG_PATH" "$ZIP_PATH"

echo "Creating $DMG_PATH"
hdiutil create \
    -volname "$APP" \
    -srcfolder "$APP_PATH" \
    -ov \
    -format UDZO \
    "$DMG_PATH"

echo "Creating $ZIP_PATH"
ditto -c -k --sequesterRsrc --keepParent "$APP_PATH" "$ZIP_PATH"

echo "Artifacts:"
ls -lh "$DMG_PATH" "$ZIP_PATH"
file "$APP_PATH/Contents/MacOS/moolticute"
