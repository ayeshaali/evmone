# evmone: Fast Ethereum Virtual Machine implementation
# Copyright 2018 The evmone Authors.
# SPDX-License-Identifier: Apache-2.0

include(hunter_cmake_args)

hunter_cmake_args(
    ethash
    CMAKE_ARGS -DETHASH_BUILD_ETHASH=OFF
)

hunter_config(
    libff
    VERSION 0.2.1
    URL https://github.com/scipr-lab/libff/archive/refs/tags/v0.2.1.tar.gz
    SHA1 7736613185125ceb542927aa81b426207efb7ec6
    CMAKE_ARGS OPENSSL_ROOT_DIR=/usr/local/opt/openssl WITH_PROCPS=OFF
)
