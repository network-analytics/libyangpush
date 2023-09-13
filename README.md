# libyangpush

## Introduction

This is a library used for parsing yang push messages to find involved 
YANG model in a subscription and creating schema for yang push subscription.  

The main features include:
- Parse the field datastore-xptah-filter/datastore-subtree-filter
- Find direct/reverse dependency for a module/submodule
- Generate a registration list for registering schemas
- Create post json schema for YANG model


## Requirements

cmake >= 3.16.4  
meson >= 1.1.1  
cmocka >= 1.1.5  
libxml2 >= 2.9.10  
libcdada >= 0.4.0  
jansson >= 2.14  
libcurl >= 8.2.1

## How to install

```bash
meson setup build
cd build
meson compile
meson install
```

A Dockerfile is available under .ci/Dockerfile, which contains all the
necessary dependencies to build the project.

## How to run test
The module set used for unit tests is stored in the /test/resources file
```bash
meson setup build
cd build
meson test
``` 
