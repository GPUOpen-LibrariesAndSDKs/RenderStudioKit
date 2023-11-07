ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
PROJECT_DIR := $(ROOT_DIR)/../..

build:
	docker build -t render-studio-builder-image -f Dockerfile.Builder .
	docker run --rm -v $(PROJECT_DIR):/mnt/ --name render-studio-builder-container render-studio-builder-image

test:
	docker build -t render-studio-tester-image -f Dockerfile.Tester .
	docker run --rm -v $(PROJECT_DIR):/mnt/ -v $(PROJECT_DIR)/SyncthingTest/:/var/lib/AMD/ --name render-studio-tester-container render-studio-tester-image

deploy:
	rsync -azP --delete $(PROJECT_DIR)/Build/Install/USD/* amazon:/opt/rs/
