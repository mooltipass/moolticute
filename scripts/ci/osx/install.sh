#!/bin/bash
set -ev

echo "install.sh"

brew install jq lftp
brew uninstall wget
brew install wget
echo "Current PATH: $PATH"
pip3 install --break-system-packages --upgrade pip
pip3 install --break-system-packages aqtinstall -t /Users/travis/aqt
export PYTHONPATH=$PYTHONPATH:/Users/travis/aqt
/Users/travis/aqt/bin/aqt install-qt mac desktop 6.2.4 clang_64 -m all -O /Users/travis/Qt
networksetup -setv6off Ethernet
