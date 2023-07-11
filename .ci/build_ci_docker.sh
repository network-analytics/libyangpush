#! /bin/bash

docker build --build-arg HTTP_PROXY=$HTTP_PROXY --build-arg HTTPS_PROXY=$HTTPS_PROXY --build-arg http_proxy=$http_proxy --build-arg https_proxy=$https_proxy -t registry-eu.inhuawei.com:5050/libyangpush-ci:v1.3.0 .
