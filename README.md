[![Build Status](https://github.com/pqrs-org/cpp-dispatcher/workflows/CI/badge.svg)](https://github.com/pqrs-org/cpp-dispatcher/actions)
[![License](https://img.shields.io/badge/license-Boost%20Software%20License-blue.svg)](https://github.com/pqrs-org/cpp-dispatcher/blob/main/LICENSE.md)

# cpp-dispatcher

A C++ implementation of header-only single threaded dispatcher.
The main aim of this library is to avoid calling function with a released object.

## Requirements

cpp-dispatcher depends [pqrs::thread_wait](https://github.com/pqrs-org/cpp-thread_wait).

## Install

Copy `include/pqrs` and `vendor/vendor/include` directories into your include directory.
