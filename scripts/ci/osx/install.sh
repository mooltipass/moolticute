#!/bin/bash
set -ev

echo "install.sh"

brew install jq lftp
brew uninstall wget
brew install wget
pip3 install aqtinstall
aqt install-qt mac desktop 6.2.4 clang_64 -m all -O /Users/travis/Qt
networksetup -setv6off Ethernet
