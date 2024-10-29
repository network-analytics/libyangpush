# Docker instruction for netopeer2, with the augmentedby feature implemented
This folder contains the Dockerfile for building netopeer2, with augmentedby feature implemented. The augmentedby feature is implemented in [sysrepo](https://github.com/Zephyre777/sysrepo.git) and [libyang](https://github.com/Zephyre777/libyang.git) by Zhuoyao Lin during IETF190 hackathon.
This Dockerfile is not yet complete and some configurations in bash need to be done manually. Please see the following instruction for how to configure and launch netopeer2-server and netopeer2-cli.

## build docker image
```
./build_docker_image.sh
```

## Manually configuration for netopeer2
```
cd netopeer2/scripts

export NP2_MODULE_DIR=/usr/share/yang/modules/netopeer2
export NP2_MODULE_PERMS=600
export LN2_MODULE_DIR=/usr/share/yang/modules/libnetconf2/

./setup.sh
./merge_hostkey.sh
./merge_config.sh
```

## Run netopeer2 docker container  
```
docker run -dit --name [your_container_name] -p [port_from_your_machine]:830 -it netopeer2_ietf120
```

## Configure password for the docker container
```
docker exec -itu 0 [your_container_name] pass_wd
```  
Then type in your new password, which is later going to be used as the netopeer2-cli ssh connection password. 

## Run netopeer2-server
```
netopeer2-server
```

## RUN netopeer2-cli
```
netopeer2-cli

> connect
```
Type in the password you set to connect as the root user for netopeer2

## Test with netopeer2

Get the yanglib content to check is the augmentedby feature is correctly implemented

```
> get --filter-xpath /ietf-yang-library:yang-library/module-set/module/augmented-by
DATA
<data xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">
  <yang-library xmlns="urn:ietf:params:xml:ns:yang:ietf-yang-library">
    <module-set>
      <name>complete</name>
      <module>
        <name>ietf-factory-default</name>
        <augmented-by xmlns="urn:ietf:params:xml:ns:yang:ietf-yang-library-augmentedby">sysrepo-factory-default</augmented-by>
      </module>
      <module>
        <name>ietf-yang-library</name>
        <augmented-by xmlns="urn:ietf:params:xml:ns:yang:ietf-yang-library-augmentedby">ietf-yang-library-augmentedby</augmented-by>
      </module>
      <module>
        <name>ietf-netconf</name>
        <augmented-by xmlns="urn:ietf:params:xml:ns:yang:ietf-yang-library-augmentedby">ietf-netconf-with-defaults</augmented-by>
        <augmented-by xmlns="urn:ietf:params:xml:ns:yang:ietf-yang-library-augmentedby">ietf-netconf-nmda</augmented-by>
      </module>
      <module>
        <name>ietf-interfaces</name>
        <augmented-by xmlns="urn:ietf:params:xml:ns:yang:ietf-yang-library-augmentedby">ietf-ip</augmented-by>
        <augmented-by xmlns="urn:ietf:params:xml:ns:yang:ietf-yang-library-augmentedby">ietf-network-instance</augmented-by>
      </module>
      <module>
        <name>ietf-subscribed-notifications</name>
        <augmented-by xmlns="urn:ietf:params:xml:ns:yang:ietf-yang-library-augmentedby">ietf-yang-push</augmented-by>
      </module>
      <module>
        <name>ietf-netconf-server</name>
        <augmented-by xmlns="urn:ietf:params:xml:ns:yang:ietf-yang-library-augmentedby">libnetconf2-netconf-server</augmented-by>
      </module>
    </module-set>
  </yang-library>
</data>
```
If the netopee2 retrun the above content, your netopeer2 with augmented-by feature is correctly installed

