# libyangpush

## Introduction

This is a library used for parsing yang push messages to find involved 
YANG model in a subscription and creating schema for yang push subscription.  

The features included in the main branch are:
- Parse the field datastore-xptah-filter/datastore-subtree-filter
- Find direct/reverse dependency for a module/submodule


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

## Branches
The main branch is the stable branch that contains code which has been validated. The feature/register_schema branch contains code for the following functionalities that has not been reviewed yet:  
- Generate a registration list for registering schemas
- Create post json schema for YANG model

The code in the feature/register_schema branch is the most up-to-date. It will be merged to main after testing.

## Contribution
This library is done by Zhuoyao Lin, a 2nd year master student in Ecole Polytechnique during her internship at Huawei Ireland Research Center, as an individual contributor.