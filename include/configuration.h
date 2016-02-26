#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//configuration files
#define ENV_HGM_CONFIG_FILE_LOC "NSS_HGM_CONFIG_FILE"
#define DEFAULT_HGM_CONFIG_FILE_LOC "/etc/hadoop-groups-mapping/conf/hadoop-groups-mapping.properties"
#define GET_HGM_CONFIG_FILE_LOC() zk_getenv(ENV_HGM_CONFIG_FILE_LOC, DEFAULT_HGM_CONFIG_FILE_LOC)

#define ENV_CONFIG_FILE_LOC "NSS_CONFIG_FILE"
#define DEFAULT_CONFIG_FILE_LOC "/etc/zk_nss_config.properties"
#define GET_CONFIG_FILE_LOC() zk_getenv(ENV_CONFIG_FILE_LOC, DEFAULT_CONFIG_FILE_LOC)

#define ENV_CONFIG_QOURUM_ADRESS "ZK_NSS_QUORUM"
#define FILE_CONFIG_QUORUM_ADRESS "zkcluster"
#define DEFAULT_CONFIG_QUORUM_ADRESS "localhost:2181"
#define GET_CONFIG_QUORUM_ADRESS() zk_getconfig(ENV_CONFIG_QOURUM_ADRESS, FILE_CONFIG_QUORUM_ADRESS, DEFAULT_CONFIG_QUORUM_ADRESS, 0)

#define ENV_CONFIG_ZNODE_ADRESS "ZK_NSS_ZNODE"
#define FILE_CONFIG_ZNODE_ADRESS "rootNode"
#define DEFAULT_CONFIG_ZNODE_ADRESS "/user_management"
#define GET_CONFIG_ZNODE_ADRESS() zk_getconfig(ENV_CONFIG_ZNODE_ADRESS, FILE_CONFIG_ZNODE_ADRESS, DEFAULT_CONFIG_ZNODE_ADRESS, 0)

#define ENV_AUTH_SCHEME "ZK_NSS_AUTH"
#define FILE_AUTH_SCHEME "authScheme"
#define GET_AUTH_SCHEME() zk_getconfig(ENV_AUTH_SCHEME, FILE_AUTH_SCHEME, 0, 1)

#define ENV_AUTH_USER "ZK_NSS_USER"
#define FILE_AUTH_USER "authUser"
#define GET_AUTH_USER() zk_getconfig(ENV_AUTH_USER, FILE_AUTH_USER, 0, 1)

#define ENV_AUTH_PASS "ZK_NSS_PASS"
#define FILE_AUTH_PASS "authPass"
#define GET_AUTH_PASS() zk_getconfig(ENV_AUTH_PASS, FILE_AUTH_PASS, 0, 1)

#define ENV_DEBUG "ZK_NSS_DEBUG"
#define FILE_DEBUG "debug"
#define GET_DEBUG() zk_getconfig(ENV_DEBUG, FILE_DEBUG, 0, 1)

#define ZK_USER_HOME "/home"
#define ZK_USER_SHELL "/sbin/nologin"

const char *zk_getenv(const char *name, const char *defaultvalue);

const char *zk_getfileconfig(const char *file, const char *name, const char *defaultvalue);

const char *zk_getfileconfig_n(const char *file1, const char *file2, const char *name, const char *defaultvalue);

const char *zk_getfileconfig_a(const char *name, const char *defaultvalue, int rootdef);

const char *zk_getconfig(const char *envname, const char *filename, const char *defaultvalue, int rootdef);

#endif
