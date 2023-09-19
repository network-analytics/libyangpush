# libyangpush

## Introduction

This is a library used for parsing yang push messages to find involved 
YANG model in a subscription and creating schema for yang push subscription.  

The main features include:
- Parse the field datastore-xptah-filter/datastore-subtree-filter
- Find direct/reverse dependency for a module/submodule
- Generate a registration list for registering schemas
- Create post json schema for YANG model

The code in this branch is the most up-to-date with functionalities to create and register schemas.

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
meson setup --libdir=/usr/local/lib build
cd build
meson compile
meson install
```
Setting ```--libdir=/usr/local/lib``` will install the library .so into /usr/lib.

A Dockerfile is available under .ci/Dockerfile, which contains all the
necessary dependencies to build the project.

## How to run test
The module set used for unit tests is stored in the /test/resources file
```bash
meson setup build
cd build
meson test
``` 

## Example program
This branch provides an example program to use this library. To run the program, the user needs to configure a NETCONF device and a schema registry and specifies their ip address and port in the example.h file. For NETCONF server, user can use either public key or password authentication method. For the schema registry, user can specify the prefix they want for each schema. A folder called modules should be created in the example folder for storing the schemas fetched from NETCONF server.  
The program uses the push-update.xml in the resources folder as a fake YANG push notification message. User can define theirs and use it by specifiying the path in example.h file.  
To run the example program, use the following commands：
```bash
cd example
mkdir modules
meson setup build
cd build
meson compile
./example_program
```
