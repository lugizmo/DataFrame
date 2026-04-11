# Filename: linux_gcc.Dockerfile
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

FROM gcc:15 AS base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        cmake \
        git \
        ninja-build \
        valgrind && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY . .

FROM base AS debug

RUN cmake -S . -B build/debug -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DLUGIZMO_DF_BUILD_TESTING=ON \
        -DLUGIZMO_DF_BUILD_BENCHMARKS=OFF \
        -DLUGIZMO_DF_ENABLE_WARNINGS=ON \
        -DLUGIZMO_DF_ENABLE_SANITIZERS=ON \
        -DLUGIZMO_DF_ENABLE_OPTIMIZATIONS=OFF \
        -DLUGIZMO_DF_ENABLE_LTO=OFF \
        -DLUGIZMO_DF_STRICT_MODE=ON \
        -DLUGIZMO_DF_USE_CLANG_TIDY=OFF \
        -DLUGIZMO_DF_DOWNLOAD_BENCHMARK=ON \
        -DLUGIZMO_DF_DOWNLOAD_GTEST=ON \
        -DLUGIZMO_DF_ENABLE_ASSERT_TRACE=ON && \
    cmake --build build/debug --parallel && \
    ctest --test-dir build/debug --output-on-failure && \
    valgrind --error-exitcode=1 --leak-check=full --track-origins=yes \
        build/debug/lib/test/LuDataFrameTest

FROM base AS release

RUN cmake -S . -B build/release -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DLUGIZMO_DF_BUILD_TESTING=ON \
        -DLUGIZMO_DF_BUILD_BENCHMARKS=ON \
        -DLUGIZMO_DF_ENABLE_WARNINGS=ON \
        -DLUGIZMO_DF_ENABLE_SANITIZERS=OFF \
        -DLUGIZMO_DF_ENABLE_OPTIMIZATIONS=ON \
        -DLUGIZMO_DF_ENABLE_LTO=ON \
        -DLUGIZMO_DF_STRICT_MODE=OFF \
        -DLUGIZMO_DF_USE_CLANG_TIDY=OFF \
        -DLUGIZMO_DF_DOWNLOAD_BENCHMARK=ON \
        -DLUGIZMO_DF_DOWNLOAD_GTEST=ON \
        -DLUGIZMO_DF_ENABLE_ASSERT=OFF \
        -DLUGIZMO_DF_ENABLE_ASSERT_TRACE=OFF && \
    cmake --build build/release --parallel && \
    ctest --test-dir build/release --output-on-failure && \
    valgrind --error-exitcode=1 --leak-check=full --track-origins=yes \
        build/release/lib/test/LuDataFrameTest && \
    cmake --install build/release --prefix /opt/lugizmo-dataframe && \
    cmake -S docker/package_smoke -B build/package_smoke -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH=/opt/lugizmo-dataframe && \
    cmake --build build/package_smoke --parallel && \
    ctest --test-dir build/package_smoke --output-on-failure && \
    valgrind --error-exitcode=1 --leak-check=full --track-origins=yes \
        build/package_smoke/lugizmo_dataframe_package_smoke
