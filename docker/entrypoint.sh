#!/usr/bin/env bash

sed -i "s/<username>/${GITHUB_LOGIN}/g" $HOME/.netrc
sed -i "s/<token>/${GITHUB_TOKEN}/g" $HOME/.netrc

chmod 600 ${HOME}/.netrc

bash