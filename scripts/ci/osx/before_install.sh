#!/bin/sh

openssl aes-256-cbc \
    -K $encrypted_7917762619ed_key \
    -iv $encrypted_7917762619ed_iv \
    -in scripts/ci/osx/developerID_application.cer.enc \
    -out scripts/ci/osx/developerID_application.cer \
    -d
