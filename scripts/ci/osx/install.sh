#!/bin/bash
set -ev

echo "install.sh"

brew update > /dev/null
brew upgrade wget
brew install jq lftp
pip install aqtinstall
aqt install-qt mac desktop 6.2.4 clang_64 -m all -O /opt/Qt
networksetup -setv6off Ethernet
