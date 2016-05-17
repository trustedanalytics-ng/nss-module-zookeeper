# Name Service Switch module for zookeeper

## HGM

Hadoop Groups Mapping is rest server to manage users stored in zookeeper which is compatible with NSSZK data format.

## NSSZK and Hadoop

Module can be deployed within standalone Hadoop cluster and allow to:

* Easily manage HDFS and Hive permissions for groups
* Spawn jobs on YARN from users stored in zookeeper (YARN checks if user exists on local machine before spawning job)
* Manage YARN queue privileges, which allow you to limit YARN resources for groups

## Data structure

NSSZK map data stored in zookeeper to unix groups and users.

## Data format

Users are stored in format: `<base znode>/<user>/<group>`. NSSZK use data stored in this znodes to generate user and group information.

### User

* Username - user znode name
* UID - parsed value of user znode
* Home directory - base path is hardcoded. User home directory will be `/home/<user>`
* Shell - `/sbin/nologin`
* Groups - all child znodes of user znode
* Default group - First created znode (sorted by creation time)

### Group

* Group name - group znode name
* GID - parsed value of group znode

# Installation

## Zookeeper instalation

To prevent infinity loop in zookeeper, you have to reconfigure zookeeper library before installing NSSZK:

* Download zookeeper release `http://www-eu.apache.org/dist/zookeeper/`
* Unpack zookeeper and go to c library directory `src/c`
* Run `./configure`. This will generate `config.h`.
* Remove `HAVE_GETLOGIN`, `HAVE_GETPWUID_R` and `HAVE_GETUID` defines from `config.h`.

## NSS compilation

To compile NSS type `make compile`. 

# Configuration

NSSZK use 2 configurations in java property format:
* System defined configuration (via system environments or system config file)
* Cloudera HGM gateway generated configuration

## Options

### Default system config file location

> Default value: `/etc/zk_nss_config.properties` <br />
> Env name: `NSS_CONFIG_FILE`

### Default HGM config file location

> Default value: `/etc/hadoop-groups-mapping/conf/hadoop-groups-mapping.properties` <br />
> Env name: `NSS_HGM_CONFIG_FILE`

### Zookeeper quorum

> Default value: `localhost:2181` <br />
> File config option: `zkcluster` <br />
> Env name: `ZK_NSS_QUORUM`

### Zookeeper base znode

> Default value: `/user_management` <br />
> File config option: `rootNode` <br />
> Env name: `ZK_NSS_ZNODE`

### Zookeeper base znode

> Default value: `/user_management` <br />
> File config option: `rootNode` <br />
> Env name: `ZK_NSS_ZNODE`

### Zookeeper auth scheme 

Specify which zookeeper auth scheme NSSZK should use while connecting to zookeeper. Default is without auth. Other possibilities:

* `digest` - while using this method zookeeper user and password have to be not null

> Default value: `null` <br />
> File config option: `authScheme` <br />
> Env name: `ZK_NSS_AUTH`

### Zookeeper user

> Default value: `null` <br />
> File config option: `authUser` <br />
> Env name: `ZK_NSS_USER`

### Zookeeper password

> Default value: `null` <br />
> File config option: `authPass` <br />
> Env name: `ZK_NSS_PASS`

### Debug

To enable debug set this option as `true`

> Default value: `null` <br />
> File config option: `debug` <br />
> Env name: `ZK_NSS_DEBUG`
