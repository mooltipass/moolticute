#!/bin/bash
# Shared macOS build environment for Intel and Apple Silicon hosts.

set -e

OSX_SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export MACOS_ARCH="$(uname -m)"

case "$MACOS_ARCH" in
    arm64)
        export GOARCH=arm64
        export MC_CLI_TOOLS_ARCH=arm64
        ;;
    x86_64)
        export GOARCH=amd64
        export MC_CLI_TOOLS_ARCH=amd64
        ;;
    *)
        echo "Unsupported macOS architecture: $MACOS_ARCH"
        exit 1
        ;;
esac

detect_qtdir() {
    if [ -n "${QTDIR:-}" ] && { [ -x "$QTDIR/bin/qmake" ] || [ -x "$QTDIR/bin/qmake6" ]; }; then
        return 0
    fi

    for candidate in \
        "/opt/homebrew/opt/qt" \
        "/usr/local/opt/qt" \
        "/Users/travis/Qt/6.2.4/macos" \
        "$HOME/Qt/6.2.4/macos"
    do
        if [ -x "$candidate/bin/qmake" ] || [ -x "$candidate/bin/qmake6" ]; then
            export QTDIR="$candidate"
            return 0
        fi
    done

    echo "Qt not found. Install with Homebrew (brew install qt) or set QTDIR."
    return 1
}

qmake_bin() {
    if [ -x "$QTDIR/bin/qmake6" ]; then
        echo "$QTDIR/bin/qmake6"
    else
        echo "$QTDIR/bin/qmake"
    fi
}

macdeployqt_bin() {
    if [ -x "$QTDIR/bin/macdeployqt6" ]; then
        echo "$QTDIR/bin/macdeployqt6"
    else
        echo "$QTDIR/bin/macdeployqt"
    fi
}

build_mc_cli_tools() {
    local dest="$1"

    if ! command -v go >/dev/null 2>&1; then
        echo "Go is required to build mc-agent and mc-cli for $MACOS_ARCH"
        return 1
    fi

    mkdir -p "$dest"
    local tmp
    tmp="$(mktemp -d)"

    for tool in mc-agent mc-cli; do
        rm -rf "$tmp/$tool"
        git clone --depth 1 "https://github.com/raoulh/$tool.git" "$tmp/$tool"
        (
            cd "$tmp/$tool"
            GOOS=darwin GOARCH="$GOARCH" go build -o "$dest/$tool" .
        ) || return 1
        [ -f "$dest/$tool" ] || return 1
        chmod +x "$dest/$tool"
    done

    rm -rf "$tmp"
}

bundle_mc_cli_tools() {
    local dest="$1"

    if [ "$MC_CLI_TOOLS_ARCH" = "amd64" ]; then
        # shellcheck source=../funcs.sh
        source "$OSX_SCRIPTDIR/../funcs.sh"
        wget_retry "https://calaos.fr/mooltipass/tools/macos/mc-agent" -O "$dest/mc-agent"
        wget_retry "https://calaos.fr/mooltipass/tools/macos/mc-cli" -O "$dest/mc-cli"
        chmod +x "$dest/mc-agent" "$dest/mc-cli"
        return 0
    fi

    build_mc_cli_tools "$dest"
}
