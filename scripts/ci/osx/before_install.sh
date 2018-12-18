#!/bin/sh

#try to fix travis osx issue: https://github.com/travis-ci/travis-ci/issues/6522
if [[ "${TRAVIS_OS_NAME}" == "osx"  ]]; then
    rvm get head || true
fi

rm -rf ~/.nvm && git clone https://github.com/creationix/nvm.git ~/.nvm && (cd ~/.nvm && git checkout `git describe --abbrev=0 --tags`) && source ~/.nvm/nvm.sh && nvm install6

npm install npm
mv node_modules npm
npm/.bin/npm --version
nvm install v6.12.3
nvm use 6.12.3
npm/.bin/npm --version
node --version

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPTDIR/../funcs.sh

osx_setup_netrc $HOME

#create certificate from env
echo $CODESIGN_OSX_CERT| base64 -D > $HOME/cert.p12
