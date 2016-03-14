#include "zk_nss.h"

static pthread_mutex_t NSS_ZK_MUTEX = PTHREAD_MUTEX_INITIALIZER;
#define NSS_ZK_LOCK()    do { pthread_mutex_lock(&NSS_ZK_MUTEX); } while (0)
#define NSS_ZK_UNLOCK()  do { pthread_mutex_unlock(&NSS_ZK_MUTEX); } while (0)

static zhandle_t *zk_handler = 0;
static zk_list *zk_nss_list = 0;
static int zk_nss_index = 0;


enum nss_status _nss_zk_setpwent_lock(void) {
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
    int ret = 0;
    zk_handler = init_zk();
    if (!zk_handler) DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_setpwent_lock :: Unable to init zookeeper client\n"))

    if (zk_get_users(zk_handler, &zk_nss_list) != ZOK) DRETURN(NSS_STATUS_NOTFOUND,
                                                               printf("_nss_zk_setpwent_lock :: Unable to list users\n"))

    zk_nss_index = 0;

    DRETURN(NSS_STATUS_SUCCESS, printf("_nss_zk_setpwent_lock :: OK\n"))

    clean:
    return ret;
}

enum nss_status _nss_zk_setpwent(void) {
    enum nss_status ret;
    PD(printf("_nss_zk_setpwent :: Got request\n"));
    NSS_ZK_LOCK();
    ret = _nss_zk_setpwent_lock();
    NSS_ZK_UNLOCK();
    PD(printf("_nss_zk_setpwent :: Request res %d\n", ret));
    return ret;
}

enum nss_status _nss_zk_endpwent_lock(void) {
    if (zk_handler)
        destroy_zk(zk_handler);
    zk_handler = 0;
    return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_zk_endpwent(void) {
    enum nss_status ret;
    PD(printf("_nss_zk_endpwent :: Got request\n"));
    NSS_ZK_LOCK();
    ret = _nss_zk_endpwent_lock();
    NSS_ZK_UNLOCK();
    PD(printf("_nss_zk_endpwent :: Request res %d\n", ret));
    return ret;
}

static char *get_user_home(const char *user) {
    int len1 = strlen(user);
    int len2 = strlen(ZK_USER_HOME);
    char *ret = malloc(len1 + len2 + 2);
    if (!ret)
        return 0;

    memcpy(ret, ZK_USER_HOME, len2);
    ret[len2] = '/';
    memcpy(ret + len2 + 1, user, len1 + 1);
    return ret;
}

static int pack_passwd_zk(const char *nodeValue, struct passwd *result, char *buffer, size_t buflen,
                          zhandle_t *zk_handler) {
    char *next_buff = buffer;
    size_t bufleft = buflen;
    int i = 0;

    int ID = 0;
    int UID = 0;
    int64_t UCT = -1; // set max long long

    if (bufleft <= strlen(nodeValue)) return -2;
    result->pw_name = strncpy(next_buff, nodeValue, bufleft);
    next_buff += strlen(result->pw_name) + 1;
    bufleft -= strlen(result->pw_name) + 1;

    if (bufleft <= 1) return -2;
    result->pw_passwd = strncpy(next_buff, "", bufleft);
    next_buff++;
    bufleft--;

    if (bufleft <= 1) return -2;
    result->pw_gecos = strncpy(next_buff, nodeValue, bufleft);
    next_buff += strlen(result->pw_gecos) + 1;
    bufleft -= strlen(result->pw_gecos) + 1;

    char *user_home = get_user_home(nodeValue);
    if (!user_home) return -2;

    if (bufleft <= strlen(user_home)) {
        free(user_home);
        return -2;
    }
    result->pw_dir = strncpy(next_buff, user_home, bufleft);
    free(user_home);
    next_buff += strlen(result->pw_dir) + 1;
    bufleft -= strlen(result->pw_dir) + 1;

    if (bufleft <= strlen(ZK_USER_SHELL))return -2;

    result->pw_shell = strncpy(next_buff, ZK_USER_SHELL, bufleft);
    next_buff += strlen(result->pw_shell) + 1;
    bufleft -= strlen(result->pw_shell) + 1;

    int ret = zk_get_user_id(zk_handler, nodeValue, &ID, 0);
    if (ret != ZOK) {
        return -1;
    }

    result->pw_uid = ID;

    zk_list *groups = 0;
    zk_stat zks;

    if (zk_get_user_groups(zk_handler, nodeValue, &groups) != ZOK || groups->count <= 0) {
        return -1;
    }

    ID = 0;
    for (i = 0; i < groups->count; i++) {
        if (zk_get_user_group_id(zk_handler, nodeValue, groups->data[i], &ID, &zks) != ZOK) {
            zk_delete_list(groups);
            return -1;
        }
        if (UCT == -1 || zks.ctime < UCT) {
            UCT = zks.ctime;
            UID = ID;
        }
        ID = 0;
    }

    zk_delete_list(groups);

    result->pw_gid = UID;
    return 0;
}

enum nss_status _nss_zk_getpwent_lock(struct passwd *result, char *buffer, size_t buflen, int *errnop) {
    enum nss_status ret = NSS_STATUS_SUCCESS;

    if (!zk_handler || !zk_nss_list)
        ret = _nss_zk_setpwent_lock();

    if (ret != NSS_STATUS_SUCCESS) DRETURN(ret, printf("_nss_zk_getpwent_lock :: set pw failed\n"))

    if (zk_nss_index >= zk_nss_list->count) {
        *errnop = 2;
        DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getpwent_lock :: unable to find any user\n"))
    }

    int pack_result = pack_passwd_zk(zk_nss_list->data[zk_nss_index++], result, buffer, buflen, zk_handler);

    if (pack_result == -1) {
        *errnop = 2;
        DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getpwent_lock :: pack failed\n"))
    }

    if (pack_result == -2) {
        *errnop = 34;
        DRETURN(NSS_STATUS_TRYAGAIN, printf("_nss_zk_getpwent_lock :: pack failed - retry\n"))
    }

    DRETURN(NSS_STATUS_SUCCESS, printf("_nss_zk_getpwent_lock :: OK\n"))

    clean:
    return ret;
}

enum nss_status _nss_zk_getpwent_r(struct passwd *result, char *buffer, size_t buflen, int *errnop) {
    enum nss_status ret;
    PD(printf("_nss_zk_getpwent_r :: Got request\n"));
    NSS_ZK_LOCK();
    ret = _nss_zk_getpwent_lock(result, buffer, buflen, errnop);
    NSS_ZK_UNLOCK();
    PD(printf("_nss_zk_getpwent_r :: Request res %d\n", ret));
    return ret;
}

enum nss_status _nss_zk_getpwuid_lock(uid_t uid, struct passwd *result, char *buffer, size_t buflen, int *errnop) {

    zhandle_t *zk_handler = init_zk();
    int i = 0;
    int ID = 0;
    int ret = 0;

    zk_list *users = 0;
    if (!zk_handler) DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getpwuid_lock :: Unable to init zookeeper client\n"))


    ret = zk_get_users(zk_handler, &users);
    if (ret != ZOK) {
        DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getpwuid_lock :: Unable to list users2\n"))
    }

    for (i = 0; i < users->count; i++) {
        if (zk_get_user_id(zk_handler, users->data[i], &ID, 0) == ZOK && ID == uid) {
            int pack_result = pack_passwd_zk(users->data[i], result, buffer, buflen, zk_handler);

            if (pack_result == -1) {
                *errnop = 2;
                DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getpwuid_lock :: pack failed\n"))
            }

            if (pack_result == -2) {
                *errnop = 34;
                DRETURN(NSS_STATUS_TRYAGAIN, printf("_nss_zk_getpwuid_lock :: pack failed - retry\n"))
            }

            DRETURN(NSS_STATUS_SUCCESS, printf("_nss_zk_getpwuid_lock :: OK\n"))
        }
    }


    clean:
    if(users) zk_delete_list(users);
    if(zk_handler) destroy_zk(zk_handler);
    return ret;
}

enum nss_status _nss_zk_getpwuid_r(uid_t uid, struct passwd *result, char *buffer, size_t buflen, int *errnop) {
    enum nss_status ret;
    PD(printf("_nss_zk_getpwuid_r :: Got request\n"));
    NSS_ZK_LOCK();
    ret = _nss_zk_getpwuid_lock(uid, result, buffer, buflen, errnop);
    NSS_ZK_UNLOCK();
    PD(printf("_nss_zk_getpwuid_r :: Request res %d\n", ret));
    return ret;
}

enum nss_status _nss_zk_getpwnam_lock(const char *name, struct passwd *result, char *buffer, size_t buflen,
                                      int *errnop) {

    zhandle_t *zk_handler = init_zk();
    int ret = 0;
    if (!zk_handler) DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getpwnam_lock :: Unable to init zookeeper client\n"))

    if (zk_get_user(zk_handler, name) != ZOK) DRETURN(NSS_STATUS_NOTFOUND,
                                                      printf("_nss_zk_getpwnam_lock :: Cant find user %s\n", name))

    int pack_result = pack_passwd_zk(name, result, buffer, buflen, zk_handler);


    if (pack_result == -1) {
        *errnop = 2;
        DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getpwnam_lock :: pack failed\n"))
    }

    if (pack_result == -2) {
        *errnop = 34;
        DRETURN(NSS_STATUS_TRYAGAIN, printf("_nss_zk_getpwnam_lock :: pack failed - retry\n"))
    }

    DRETURN(NSS_STATUS_SUCCESS, printf("_nss_zk_getpwnam_lock :: OK\n"))

    clean:
    destroy_zk(zk_handler);
    return ret;
}

enum nss_status _nss_zk_getpwnam_r(const char *name, struct passwd *result, char *buffer, size_t buflen,
                                   int *errnop) {

    enum nss_status ret;
    PD(printf("_nss_zk_getpwnam_r :: Got request\n"));
    NSS_ZK_LOCK();
    ret = _nss_zk_getpwnam_lock(name, result, buffer, buflen, errnop);
    NSS_ZK_UNLOCK();
    PD(printf("_nss_zk_getpwnam_r :: Request res %d\n", ret));
    return ret;
}
