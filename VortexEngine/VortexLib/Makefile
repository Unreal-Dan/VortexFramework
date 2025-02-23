# clear out all default make targets
.SUFFIXES:

# List all make targets which are not filenames
.PHONY: all tests clean wasm compute_version

# compiler tool definitions
ifdef WASM
CC=em++
AR=emar cru
else
CC=g++
AR=ar cru
endif

MAKE=make
RM=rm -rf
RANLIB=ranlib

CFLAGS=-O2 -Wall

# add -g flag for debug symbols if we're not using wasm
ifndef WASM
CFLAGS += -g
endif

# The branch/tag suffix for this device
BRANCH_SUFFIX=g

# compiler defines
DEFINES=\
	-D VORTEX_LIB \
	-D VORTEX_VERSION_MAJOR=$(VORTEX_VERSION_MAJOR) \
	-D VORTEX_VERSION_MINOR=$(VORTEX_VERSION_MINOR) \
	-D VORTEX_BUILD_NUMBER=$(VORTEX_BUILD_NUMBER) \
	-D VORTEX_VERSION_NUMBER=$(VORTEX_VERSION_NUMBER)

# compiler include paths
INCLUDES=\
	-I ../VortexLib/ \
	-I ../src/ \
	-I ./ \

ifdef WASM
DEFINES += -D WASM
# this was used before but I think only the above is necessary
LDFLAGS += --bind -s EXPORT_ES6=1 -s EXPORT_NAME='VortexLib'
endif

ifdef TESTFRAMEWORK
# This is a lowercase define because it matches the solution name 'VortexTestingFramework'
# because in the windows build a trick is used to put the solution name into the preprocessor
# so that the engine can check which solution it is compiling inside. Because of that we
# need to use the same define here
DEFINES += -D VortexTestingFramework
endif

# only set them if they're not empty to prevent unnecessary whitespace
ifneq ($(DEFINES),)
    CFLAGS+=$(DEFINES)
endif
ifneq ($(INCLUDES),)
    CFLAGS+=$(INCLUDES)
endif

# local NONSTANDARD libraries to link with
# these MUST be exact filenames, cannot be -l syntax
# for example:
#   ../path/to/libname.a
# NOT THIS:
#   -L../path/to -lname
# You should NOT need to add a make target to build
# this library if you have added it correctly.
LLIBS=\

# STANDARD libraries to link with (-l is fine here)
# MUST have LLIBS BEFORE the standard libraries
LIBS=\
	$(LLIBS) \

# source files
# local source files first, other sources after
ifeq ($(OS),Windows_NT)
    SRC = $(shell find ../src/ -type f -name \\*.cpp) VortexLib.cpp
else
    SRC = $(shell find ../src/ -type f -name '*.cpp') VortexLib.cpp
endif

# object files are source files with .c replaced with .o
OBJS=\
	$(SRC:.cpp=.o) \

# dependency files are source files with .c replaced with .d
DFILES=\
	$(SRC:.cpp=.d) \

# target dependencies
# this includes any script generated c/h files,
# the $(LLIBS) list, and the $(OBJS) list
DEPS=\
	$(LLIBS) \
	$(OBJS) \

# unit tests
TESTS=\

# target files
ifdef WASM
TARGETS=VortexLib.js \
	vortex.a
else
TARGETS=vortex.a
endif

# Default target for 'make' command
all: $(TARGETS)

# unit test target
tests: $(TESTS)

# force sub-build of wasm
wasm: FORCE
	env WASM=1 $(MAKE)

# target for vortex lib
vortex.a: compute_version $(DEPS)
	$(AR) $@ $(DEPS)

VortexLib.js: compute_version $(DEPS)
	$(CC) $(LDFLAGS) $(DEPS) -o $@ $(LLIBS)

# catch-all make target to generate .o and .d files
%.o: %.cpp
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# catch-all for static libraries in the form of:
# <directory>/<library.a>
# this expects that the makefile in <directory> has a
# make target named <library>
%.a: FORCE
	$(MAKE) -C $(dir $@) $(notdir $@)

# Empty rule that forces %.a to run all the time
FORCE:

# generic clean target
clean:
	@$(RM) $(DFILES) $(OBJS) $(TARGETS) $(TESTS) VortexLib.js vortex.wasm

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

# Now include our target dependency files
# the hyphen means ignore non-existent files
-include $(DFILES)
