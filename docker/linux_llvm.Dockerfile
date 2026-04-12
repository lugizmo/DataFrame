# Filename: linux_llvm.Dockerfile
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

FROM debian:trixie-slim AS base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        git \
        gnupg \
        lsb-release \
        ninja-build \
        valgrind \
        wget && \
    rm -rf /var/lib/apt/lists/*

RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 19 all && \
    rm -f llvm.sh

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        clang-19 \
        clang-tidy-19 \
        clang-tools-19 \
        cppcheck \
        libc++-19-dev \
        libc++abi-19-dev \
        lld-19 && \
    rm -rf /var/lib/apt/lists/*

ENV CC="clang-19"
ENV CXX="clang++-19"
ENV LD="ld.lld-19"
ENV CXXFLAGS="-stdlib=libc++"
ENV LDFLAGS="-stdlib=libc++"

WORKDIR /src

COPY . .

# ======= DEBUG BUILD CHECKS ===========================================================================================
FROM base AS debug

# build and run tests (with sanitizers)
RUN cmake --preset debug
RUN cmake --build --preset debug --parallel
RUN ctest --preset debug

# run valgrind tests
RUN valgrind --error-exitcode=1 --leak-check=full --track-origins=yes build/presets/debug/lib/test/LuDataFrameTest

# run clang-tidy tests with a focused checkset to keep Docker logs usable
# TODO after refactoring the code activate and check again
#RUN find lib/test -type f -name '*.cpp' -print0 | \
#    xargs -0 clang-tidy-19 \
#        -quiet \
#        -p build/presets/debug \
#        -header-filter='^/src/lib/(include|test)/' \
#        -checks='-*,clang-analyzer-*,bugprone-*,performance-*'

# run cpp check
# TODO with this setup reports a lot also of dependencies and valid pattern
#RUN cppcheck --project=build/presets/debug/compile_commands.json \
#    --quiet \
#    --enable=warning,performance,portability \
#    --error-exitcode=1 \
#    --inline-suppr \
#    --suppress=missingIncludeSystem \
#    -i/src/build/presets/debug/_deps \
#    -i/src/build/presets/debug

# ======= RELEASE BUILD CHECKS =========================================================================================
FROM base AS release

# build and run tests (with sanitizers)
RUN cmake --preset release
RUN cmake --build --preset release --parallel
RUN ctest --preset release

# run valgrind tests
RUN valgrind --error-exitcode=1 --leak-check=full --track-origins=yes build/presets/release/lib/test/LuDataFrameTest

# build and run library packing test
RUN cmake --install build/presets/release --prefix /opt/lugizmo-dataframe
RUN cmake -S docker/package_smoke -B build/package_smoke -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/lugizmo-dataframe

RUN cmake --build build/package_smoke --parallel
RUN ctest --test-dir build/package_smoke --output-on-failure

# run valgrind on packed project
RUN valgrind --error-exitcode=1 --leak-check=full --track-origins=yes build/package_smoke/lugizmo_dataframe_package_smoke
