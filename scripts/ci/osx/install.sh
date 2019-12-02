#!/bin/bash
set -ev

echo "install.sh"

brew update > /dev/null
brew upgrade
brew install qt5 jq lftp
