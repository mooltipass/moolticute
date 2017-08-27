SHELL := /bin/bash
PROJECT_NAME := moolticute

TRAVIS_BUILD_DIR := $(shell pwd)

BUILD_TAG := $(shell git tag --points-at HEAD | tail -n 1)

CONTAINER_NAME := $(PROJECT_NAME)
DOCKER_EXEC_ENV ?=
DOCKER_EXEC = docker exec $(DOCKER_EXEC_ENV) $(CONTAINER_NAME) /bin/bash -c

DOCKER_COMPOSE_CONFIG := -f docker-compose.base.yml
ifeq ($(ENV),dev)
DOCKER_COMPOSE_CONFIG +=  -f docker-compose.dev.yml
else
DOCKER_COMPOSE_CONFIG +=  -f docker-compose.yml
endif

BUILD_DIR := $(TRAVIS_BUILD_DIR)/build-linux
DEB_VERSION := $(shell echo $(BUILD_TAG) | tr 'v' ' ' | xargs)

USER_EMAIL ?= limpkin@limpkin.fr

GITHUB_REPO ?= $(shell echo "$(TRAVIS_REPO_SLUG)" | rev | cut -d "/" -f1 | rev)
GITHUB_ACCOUNT ?= $(shell echo "$(TRAVIS_REPO_SLUG)" | rev | cut -d "/" -f2 | rev)

export

# Debug mode
ifeq ($(DEBUG),1)
$(info TRAVIS_REPO_SLUG = [$(TRAVIS_REPO_SLUG)])
$(info PROJECT_NAME = [$(PROJECT_NAME)])
$(info TRAVIS_BUILD_DIR = [$(TRAVIS_BUILD_DIR)])
$(info BUILD_TAG = [$(BUILD_TAG)])
$(info CONTAINER_NAME = [$(CONTAINER_NAME)])
$(info DOCKER_EXEC = [$(DOCKER_EXEC)])
$(info BUILD_DIR = [$(BUILD_DIR)])
$(info USER_EMAIL = [$(USER_EMAIL)])
$(info GITHUB_ACCOUNT = [$(GITHUB_ACCOUNT)])
$(info GITHUB_REPO = [$(GITHUB_REPO)])
$(info GITHUB_LOGIN = [$(GITHUB_LOGIN)])
$(info GITHUB_TOKEN = [$(GITHUB_TOKEN)])
$(info DEB_VERSION = [$(DEB_VERSION)])
endif

.PHONY: build docker_prepare docker_image github_upload git_setup deb_init deb_clean deb_changelog deb_package debian

# Builds
build: docker_prepare
	$(DOCKER_EXEC) "cd build-linux && qmake /app/Moolticute.pro && make"

# Docker
docker_prepare:
	-mkdir -p $(BUILD_DIR)/deb
	docker-compose $(DOCKER_COMPOSE_CONFIG) up --force-recreate -d
ifeq ($(DEBUG),1)
	-docker inspect $(CONTAINER_NAME)
	-$(DOCKER_EXEC) "cat ~/.netrc"
endif

docker_image:
	docker build -t $(PROJECT_NAME) ./docker

docker_image_windows:
	docker build -t $(PROJECT_NAME).windows -f docker/Dockerfile.windows .

# GitHub
github_upload:
ifneq ($(BUILD_TAG),)
	$(DOCKER_EXEC) "source docker/tools.sh && \
	PROJECT_NAME=$(PROJECT_NAME) TRAVIS_OS_NAME=$(TRAVIS_OS_NAME) \
	create_github_release $(BUILD_TAG)"
endif

github_access_test:
	$(DOCKER_EXEC) "cd /app/build-linux/deb && . /usr/local/bin/tools.sh && ok.sh list_releases $(GITHUB_ACCOUNT) $(GITHUB_REPO)"

# Git
git_setup:
	$(DOCKER_EXEC) "git config --global user.email '$(USER_EMAIL)'"
	$(DOCKER_EXEC) "git config --global user.name '$(USER)'"

# Debian packages
deb_init:
	$(DOCKER_EXEC) "dch --create --distribution trusty --package \"moolticute\" \
	--newversion $(BUILD_TAG) \"Initial package\""

deb_clean:
	$(DOCKER_EXEC) "dh_clean"

deb_changelog: git_setup
	@echo "Generating changelog for tag $(BUILD_TAG) [$(TRAVIS_COMMIT)]"
	$(DOCKER_EXEC) 'gbp dch --debian-tag="%(version)s" --new-version=$(DEB_VERSION) --debian-branch $(TRAVIS_BRANCH) \
	 --git-author --distribution="stable" --force-distribution \
	 --since=$(TRAVIS_COMMIT) --release --spawn-editor=never --ignore-branch'

deb_package:
	$(DOCKER_EXEC) "cp -f README.md debian/README"
	$(DOCKER_EXEC) "dpkg-buildpackage -b -us -uc && mkdir -p build-linux/deb && cp ../*.deb build-linux/deb"

# Misc
if_should_build:
	. docker/tools.sh && is_build_of_tag $(BUILD_TAG)

# Build a complete Debian package
debian: build deb_changelog deb_package