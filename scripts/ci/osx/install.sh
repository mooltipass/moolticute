#!/bin/bash
set -ev

echo "install.sh"

brew update > /dev/null
brew upgrade wget
brew install qt6 jq lftp
networksetup -setv6off Ethernet
