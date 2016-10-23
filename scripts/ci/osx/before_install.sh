#!/bin/sh

#try to fix travis osx issue: https://github.com/travis-ci/travis-ci/issues/6522
if [[ "${TRAVIS_OS_NAME}" == "osx"  ]]; then
    rvm get head || true
fi

rm -rf ~/.nvm && git clone https://github.com/creationix/nvm.git ~/.nvm && (cd ~/.nvm && git checkout `git describe --abbrev=0 --tags`) && source ~/.nvm/nvm.sh && nvm install6
npm install npm
mv node_modules npm
npm/.bin/npm --version
nvm install v6.4.0
nvm use 6.4.0
npm/.bin/npm --version
node --version

#openssl aes-256-cbc \
#    -K $encrypted_7917762619ed_key \
#    -iv $encrypted_7917762619ed_iv \
#    -in scripts/ci/osx/developerID_application.cer.enc \
#    -out scripts/ci/osx/developerID_application.cer \
#    -d
