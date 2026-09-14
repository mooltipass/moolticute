#!/bin/bash
set -ev

echo "install.sh"

brew upgrade openssl
brew install jq lftp go
brew uninstall wget || true
brew install wget

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/env.sh

echo "Current PATH: $PATH"
echo "Target architecture: $MACOS_ARCH"

if [ "$MACOS_ARCH" = "arm64" ]; then
    brew install qt
    detect_qtdir
    echo "Using Homebrew Qt at $QTDIR for Apple Silicon"
else
    pip3 install --upgrade pip
    pip3 install aqtinstall -t /Users/travis/aqt
    export PYTHONPATH=$PYTHONPATH:/Users/travis/aqt
    /Users/travis/aqt/bin/aqt install-qt mac desktop 6.2.4 clang_64 -m all -O /Users/travis/Qt
    export QTDIR="/Users/travis/Qt/6.2.4/macos"
    networksetup -setv6off Ethernet || true
fi
