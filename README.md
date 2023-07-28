# libyangpush

## Introduction

This is a library used for parsing yang push messages and create schema for yang model and yang push subscription.

## Requirements

cmake >= 3.16.4  
meson >= 1.1.1  
cmocka >= 1.1.5  
libxml2 >= 2.9.10  
libcdada >= 0.4.0

## How to install

```bash
meson setup build
cd build
meson compile
```

A Dockerfile is available under .ci/Dockerfile, which contains all the
necessary dependencies to build the project.

## How to run test

```bash
meson setup build
cd build
meson test
``` 
