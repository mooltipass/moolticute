#!/bin/bash
set -ev

echo "install.sh"

brew update > /dev/null
brew install qt5
brew install curl

