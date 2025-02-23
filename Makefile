.PHONY: all install build upload clean compute_version

ARDUINO_CLI = ./bin/arduino-cli --verbose
BOARD = esp32:esp32:XIAO_ESP32C3
PORT = COMx # Replace 'x' with the appropriate COM port number
PROJECT_NAME = VortexEngine/VortexEngine.ino
BUILD_PATH = build
CONFIG_FILE = $(HOME)/.arduino15/arduino-cli.yaml

# The branch/tag suffix for this device
BRANCH_SUFFIX=s

DEFINES=\
	-D VORTEX_VERSION_MAJOR=$(VORTEX_VERSION_MAJOR) \
	-D VORTEX_VERSION_MINOR=$(VORTEX_VERSION_MINOR) \
	-D VORTEX_BUILD_NUMBER=$(VORTEX_BUILD_NUMBER) \
	-D VORTEX_VERSION_NUMBER=$(VORTEX_VERSION_NUMBER) \
	-MMD -c #due to a bug need the -MMD and -c otherwise esp won't build

# Default target
all: build

update-index:
	$(ARDUINO_CLI) core update-index

install:
	sudo apt-get update
	sudo apt-get install -y build-essential
	pip install pyserial
	mkdir -p $(HOME)/.arduino15
	if ! command -v $(ARDUINO_CLI) &> /dev/null ; then \
		curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sudo sh ; \
	fi
	echo 'board_manager: \n  additional_urls: \n    - https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json' | sudo tee $(CONFIG_FILE)
	$(ARDUINO_CLI) lib update-index
	$(ARDUINO_CLI) core update-index --config-file $(CONFIG_FILE)
	$(ARDUINO_CLI) core install esp32:esp32 --config-file $(CONFIG_FILE)
	$(ARDUINO_CLI) lib install FastLED@3.7.6

build: compute_version
	$(ARDUINO_CLI) compile --fqbn $(BOARD) $(PROJECT_NAME) \
		--config-file $(CONFIG_FILE) \
		--build-path $(BUILD_PATH) \
		--build-property compiler.cpp.extra_flags="$(DEFINES)" \
		--build-property compiler.c.extra_flags="$(DEFINES)"
	@echo "== Success building Spark v$(VORTEX_VERSION_NUMBER) =="

upload:
	$(ARDUINO_CLI) upload -p $(PORT) --fqbn $(BOARD) $(PROJECT_NAME) --config-file $(CONFIG_FILE)

core-list:
	$(ARDUINO_CLI) core list

clean:
	rm -rf $(BUILD_PATH)

# calculate the version number of the build
compute_version:
	$(eval LATEST_TAG ?= $(shell git fetch --depth=1 origin +refs/tags/*:refs/tags/* &> /dev/null && git tag --list "*$(BRANCH_SUFFIX)" | sort -V | tail -n1))
	$(eval VORTEX_VERSION_MAJOR ?= $(shell echo $(LATEST_TAG) | cut -d. -f1))
	$(eval VORTEX_VERSION_MINOR ?= $(shell echo $(LATEST_TAG) | sed 's/$(BRANCH_SUFFIX)$$//' | cut -d. -f2))
	$(eval VORTEX_BUILD_NUMBER ?= $(shell git rev-list --count $(LATEST_TAG)..HEAD))
	$(eval VORTEX_VERSION_MAJOR := $(if $(VORTEX_VERSION_MAJOR),$(VORTEX_VERSION_MAJOR),0))
	$(eval VORTEX_VERSION_MINOR := $(if $(VORTEX_VERSION_MINOR),$(VORTEX_VERSION_MINOR),1))
	$(eval VORTEX_BUILD_NUMBER := $(if $(VORTEX_BUILD_NUMBER),$(VORTEX_BUILD_NUMBER),0))
	$(eval VORTEX_VERSION_NUMBER := $(VORTEX_VERSION_MAJOR).$(VORTEX_VERSION_MINOR).$(VORTEX_BUILD_NUMBER))
