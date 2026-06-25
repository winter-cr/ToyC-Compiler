BUILD_DIR ?= build
BUILD_TYPE ?= Debug
GENERATOR ?= Ninja
CMAKE ?= cmake
CTEST ?= ctest

.DEFAULT_GOAL := all

.PHONY: all configure build test e2e run cli clean rebuild help

all: build

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	$(CMAKE) --build $(BUILD_DIR)

test: build
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure

e2e: build
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure \
		-R backend_rv32_e2e

run: build
	$(BUILD_DIR)/backend_demo

cli:
	@$(MAKE) --no-print-directory build 1>&2
	@$(BUILD_DIR)/toyc_backend_cli $(ARGS)

clean:
	$(CMAKE) -E remove_directory $(BUILD_DIR)

rebuild: clean all

help:
	@echo "ToyC backend Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make          Configure and build the backend"
	@echo "  make test     Build and run all backend tests"
	@echo "  make e2e      Run the RV32/QEMU end-to-end test"
	@echo "  make run      Build and run backend_demo"
	@echo "  make cli      Read textual IR from stdin and emit assembly"
	@echo "  make clean    Remove the build directory"
	@echo "  make rebuild  Clean and rebuild"
	@echo "  make help     Show this help"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_DIR=<dir>       Build directory (default: build)"
	@echo "  BUILD_TYPE=<type>     CMake build type (default: Debug)"
	@echo "  GENERATOR=<generator> CMake generator (default: Ninja)"
	@echo ""
	@echo "Example:"
	@echo "  make BUILD_TYPE=Release"
	@echo "  make cli ARGS=-opt < tests/data/factorial.ir > factorial.s"
