[![Build Status](https://travis-ci.org/pqrs-org/cpp-dispatcher.svg?branch=master)](https://travis-ci.org/pqrs-org/cpp-dispatcher)
[![License](https://img.shields.io/badge/license-Boost%20Software%20License-blue.svg)](https://github.com/pqrs-org/cpp-dispatcher/blob/master/LICENSE.md)

# cpp-dispatcher

A C++14 implementation of header-only single threaded dispatcher.
The main aim of this library is to avoid calling function with a released object.

## Requirements

cpp-dispatcher depends [pqrs::thread_wait](https://github.com/pqrs-org/cpp-thread_wait).

## Install

### Manual install

Copy `include/pqrs` directory into your include directory.

### Using package manager

You can also install `include/pqrs` by using [cget](https://github.com/pfultz2/cget).

```shell
cget install pqrs-org/cpp-thread_wait@v1.2.0 --cmake header
cget install pqrs-org/cpp-dispatcher@v2.1.0 --cmake header
```
