ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
PROJECT_DIR := $(ROOT_DIR)/../..

build:
	docker build -t render-studio-builder-image -f Dockerfile.Builder .
	docker run --rm -v $(PROJECT_DIR):/mnt/ --name render-studio-builder-container render-studio-builder-image

deploy:
	rsync -azP --delete $(PROJECT_DIR)/Build/Install/USD/* studio:/opt/rs/
