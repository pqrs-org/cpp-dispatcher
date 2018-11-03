[![Build Status](https://travis-ci.org/pqrs-org/cpp-dispatcher.svg?branch=master)](https://travis-ci.org/pqrs-org/cpp-dispatcher)
[![License](https://img.shields.io/badge/license-Boost%20Software%20License-blue.svg)](https://github.com/pqrs-org/cpp-dispatcher/blob/master/LICENSE.md)

# cpp-dispatcher

A C++14 implementation of header-only single threaded dispatcher.
The main aim of this library is to avoid calling function with a released object.

## Requirements

cpp-dispatcher depends [pqrs::thread_wait](https://github.com/pqrs-org/cpp-thread_wait).

You can install them by using [cget](https://github.com/pfultz2/cget).

```shell
cget install pqrs-org/cpp-thread_wait@v1.0.0 --cmake header
cget install pqrs-org/cpp-dispatcher@v1.11.0 --cmake header
```
