PROJECT_NAME := moolticute

TRAVIS_BUILD_DIR := $(shell pwd)
TRAVIS_TAG := $(shell git describe --tags --abbrev=0)
TRAVIS_COMMIT ?= $(shell git log -1 $(TRAVIS_TAG) | head -n 1 | awk '{ print $$2 }')

CONTAINER_NAME := $(PROJECT_NAME)
DOCKER_EXEC := docker exec $(CONTAINER_NAME) /bin/bash -c

BUILD_DIR := $(TRAVIS_BUILD_DIR)/build-linux

DEB_VERSION := $(shell echo $(TRAVIS_TAG) | tr 'v' ' ' | xargs)
DEB_NAME := $(PROJECT_NAME)_$(DEB_VERSION)_amd64.deb
DEB_FILE := /app/build-linux/deb/$(DEB_NAME)
DEB_MIME := application/vnd.debian.binary-package

USER_EMAIL ?= limpkin@limpkin.fr

export GITHUB_REPO := $(shell echo "$(TRAVIS_REPO_SLUG)" | rev | cut -d "/" -f1 | rev)

.PHONY: build docker_prepare docker_image github_upload git_setup deb_init deb_clean deb_changelog deb_package debian

# Builds
build: docker_prepare
	$(DOCKER_EXEC) "cd build-linux && qmake /app/Moolticute.pro && make"

# Docker
docker_prepare:
	echo "Build directory: $(TRAVIS_BUILD_DIR)"
	-mkdir -p $(BUILD_DIR)/deb
	docker-compose up -d

docker_image:
	docker build -t $(IMAGE_NAME) ./docker

# GitHub
github_upload:
	$(DOCKER_EXEC) "cd /app/build-linux/deb && . /usr/local/bin/tools.sh && create_release_and_upload_asset $(TRAVIS_TAG) $(DEB_NAME) $(DEB_MIME)"

# Git
git_setup:
	$(DOCKER_EXEC) "git config --global user.email '$(USER_EMAIL)'"
	$(DOCKER_EXEC) "git config --global user.name '$(USER)'"

# Debian packages
deb_init:
	$(DOCKER_EXEC) "dch --create --distribution trusty --package \"moolticute\" \
	--newversion $(TRAVIS_TAG) \"Initial package\""

deb_clean:
	$(DOCKER_EXEC) "dh_clean"

deb_changelog: git_setup
	$(DOCKER_EXEC) 'gbp dch --debian-tag="%(version)s" --new-version=$(DEB_VERSION) --debian-branch $(TRAVIS_BRANCH) \
	 --git-author --distribution="stable" --force-distribution \
	 --commit --since=$(TRAVIS_COMMIT) --release --spawn-editor=never --ignore-branch'

deb_package:
	$(DOCKER_EXEC) "cp -f README.md debian/README"
	$(DOCKER_EXEC) "dpkg-buildpackage -b -us -uc && mkdir -p build-linux/deb && cp ../*.deb build-linux/deb"

# Build a complete Debian package
debian: build deb_changelog deb_package