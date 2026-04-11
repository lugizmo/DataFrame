# Filename: Makefile
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

# ====== CONFIGURATION ================================================================================================

# Docker executable used for compose builds.
DOCKER ?= docker

# Compose progress output mode.
COMPOSE_PROGRESS ?= plain

# Optional log directory. Leave empty to disable log file storage.
STORE_DIR ?=

# ====== TARGETS ======================================================================================================

.PHONY: \
	lugizmo_docker_tests \
	lugizmo_docker_tests_gcc \
	lugizmo_docker_tests_llvm \
	lugizmo_docker_clean \
	lugizmo_docker_clean_all

# Run all supported docker test builds.
lugizmo_docker_tests:
	@if [ -n "$(STORE_DIR)" ]; then \
		mkdir -p "$(STORE_DIR)"; \
		$(DOCKER) compose -f compose.yaml --progress $(COMPOSE_PROGRESS) build llvm_debug \
			2>&1 | tee "$(STORE_DIR)/lugizmo_docker_tests_llvm_debug.log"; \
	else \
		$(DOCKER) compose -f compose.yaml --progress $(COMPOSE_PROGRESS) build llvm_debug; \
	fi
	@if [ -n "$(STORE_DIR)" ]; then \
		mkdir -p "$(STORE_DIR)"; \
		$(DOCKER) compose -f compose.yaml --progress $(COMPOSE_PROGRESS) build llvm_release \
			2>&1 | tee "$(STORE_DIR)/lugizmo_docker_tests_llvm_release.log"; \
	else \
		$(DOCKER) compose -f compose.yaml --progress $(COMPOSE_PROGRESS) build llvm_release; \
	fi

	# TODO currently no support for gcc

# Run GCC docker test builds.
lugizmo_docker_tests_gcc:
	@if [ -n "$(STORE_DIR)" ]; then \
		mkdir -p "$(STORE_DIR)"; \
		$(DOCKER) compose -f compose.yaml --progress $(COMPOSE_PROGRESS) build gcc_debug \
			2>&1 | tee "$(STORE_DIR)/lugizmo_docker_tests_gcc_debug.log"; \
	else \
		$(DOCKER) compose -f compose.yaml --progress $(COMPOSE_PROGRESS) build gcc_debug; \
	fi
	@if [ -n "$(STORE_DIR)" ]; then \
		mkdir -p "$(STORE_DIR)"; \
		$(DOCKER) compose -f compose.yaml --progress $(COMPOSE_PROGRESS) build gcc_release \
			2>&1 | tee "$(STORE_DIR)/lugizmo_docker_tests_gcc_release.log"; \
	else \
		$(DOCKER) compose -f compose.yaml --progress $(COMPOSE_PROGRESS) build gcc_release; \
	fi

# Run LLVM docker test builds.
lugizmo_docker_tests_llvm:
	@if [ -n "$(STORE_DIR)" ]; then \
		mkdir -p "$(STORE_DIR)"; \
		$(DOCKER) compose -f compose.yaml --progress $(COMPOSE_PROGRESS) build llvm_debug \
			2>&1 | tee "$(STORE_DIR)/lugizmo_docker_tests_llvm_debug.log"; \
	else \
		$(DOCKER) compose -f compose.yaml --progress $(COMPOSE_PROGRESS) build llvm_debug; \
	fi
	@if [ -n "$(STORE_DIR)" ]; then \
		mkdir -p "$(STORE_DIR)"; \
		$(DOCKER) compose -f compose.yaml --progress $(COMPOSE_PROGRESS) build llvm_release \
			2>&1 | tee "$(STORE_DIR)/lugizmo_docker_tests_llvm_release.log"; \
	else \
		$(DOCKER) compose -f compose.yaml --progress $(COMPOSE_PROGRESS) build llvm_release; \
	fi

# Remove compose containers and networks.
lugizmo_docker_clean:
	$(DOCKER) compose -f compose.yaml down

# Remove compose containers, networks, volumes, and images.
lugizmo_docker_clean_all:
	$(DOCKER) compose -f compose.yaml down -v --rmi all
