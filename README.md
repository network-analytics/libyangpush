# Initial repo structure

## Introduction
This is a initial repo structure for libyangpush

## Requirements
meson
cmocka

## How to run
```
$ meson build -Dbsanitize=address,undefined
$ ninja -C build
```
## How to run test
```
$ ninja -C build test
$ ./build/run_tests
``` 