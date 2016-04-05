/*
  Name Service Switch module for zookeeper
  Copyright (C) 2016, Intel Corporation.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
  USA */

#include "zk_nss.h"

static pthread_mutex_t NSS_ZK_MUTEX = PTHREAD_MUTEX_INITIALIZER;
#define NSS_ZK_LOCK()    do { pthread_mutex_lock(&NSS_ZK_MUTEX); } while (0)
#define NSS_ZK_UNLOCK()  do { pthread_mutex_unlock(&NSS_ZK_MUTEX); } while (0)


static int
pack_group_struct(zk_group_pack *data, struct group *result, char *buffer, size_t buflen) {
    char *nbuff = buffer;
    size_t bufleft = buflen;
    int ret = 0;

    if (!data) DRETURN(NSS_STATUS_NOTFOUND, printf("pack_group_struct :: data is null\n"))

    if (!data->name) DRETURN(NSS_STATUS_NOTFOUND, printf("pack_group_struct :: data->name is null\n"))
    if (bufleft <= strlen(data->name)) DRETURN(NSS_STATUS_TRYAGAIN,
                                               printf("pack_group_struct :: not enought buffer for data->name\n"))
    result->gr_name = strncpy(nbuff, data->name, strlen(data->name) + 1);
    nbuff += strlen(result->gr_name) + 1;
    bufleft -= strlen(result->gr_name) + 1;

    result->gr_gid = data->gid;

    result->gr_passwd = strncpy(nbuff, "", 1);
    nbuff += 1;
    bufleft -= 1;


    result->gr_mem = (char **) nbuff;
    nbuff += (data->users_len + 1) * sizeof(char *);
    bufleft -= (data->users_len + 1) * sizeof(char *);
    int m = 0;
    for (m = 0; m < data->users_len; m++) {
        if (!data->users[m]) DRETURN(NSS_STATUS_NOTFOUND, printf("pack_group_struct :: user %d is null\n", m))
        if (bufleft <= strlen(data->users[m])) DRETURN(NSS_STATUS_TRYAGAIN,
                                                       printf("pack_group_struct :: not enought buffer for data->users\n"))
        result->gr_mem[m] = strncpy(nbuff, data->users[m], strlen(data->users[m]) + 1);
        nbuff += strlen(data->users[m]) + 1;
        bufleft -= strlen(data->users[m]) + 1;
    }
    result->gr_mem[data->users_len] = 0;
    DRETURN(NSS_STATUS_SUCCESS, printf("pack_group_struct :: ok\n"))
    clean:
    return ret;
}

enum nss_status _nss_zk_getgrgid_lock(gid_t gid, struct group *result, char *buffer, size_t buflen, int *errnop) {
    int ret = 0;
    int ID = 0;
    int i = 0, j = 0;
    char *tmp;

    zk_group_pack *data = 0;

    zk_list *users = 0;
    zk_list *groups = 0;

    zhandle_t *zh = init_zk();
    if (!zh) DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getgrgid_lock :: Unable to init zookeeper client\n"))

    if (zk_create_group_pack(&data) != ZOK) DRETURN(NSS_STATUS_NOTFOUND,
                                                    printf("_nss_zk_getgrgid_lock :: Unable to create group_pack (malloc)\n"))

    ret = zk_get_users(zh, &users);
    if (ret != ZOK) DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getgrgid_lock :: Unable to list users\n"))

    for (i = 0; i < users->count; i++) {
        if (zk_get_user_groups(zh, users->data[i], &groups) != ZOK) DRETURN(NSS_STATUS_NOTFOUND,
                                                                            printf("_nss_zk_getgrgid_lock :: Unable to list groups for user %s\n",
                                                                                   users->data[i]))

        for (j = 0; j < groups->count; j++) {
            if (zk_get_user_group_id(zh, users->data[i], groups->data[j], &ID, 0) != ZOK) RETURN(NSS_STATUS_NOTFOUND)

            if (ID == gid) {
                if (data->gid == 0) {
                    data->gid = ID;
                    data->name = malloc(strlen(groups->data[j]) + 1);
                    memcpy(data->name, groups->data[j], strlen(groups->data[j]) + 1);
                }
                else if (data->gid != gid) DRETURN(NSS_STATUS_NOTFOUND,
                                                   printf("_nss_zk_getgrgid_lock :: Group ID mismatch %d %d\n",
                                                          data->gid, gid))

                if (data->users_len < NSS_GROUPS_USERS_MAX_SIZE) {
                    tmp = malloc(strlen(users->data[i]) + 1);
                    memcpy(tmp, users->data[i], strlen(users->data[i]) + 1);
                    data->users[data->users_len] = tmp;
                    data->users_len++;
                    tmp = 0;
                }
            }
        }
        zk_delete_list(groups);
        groups = 0;
    }

    if (data->gid == 0) DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getgrgid_lock :: Group ID %d not found\n", gid))

    ret = pack_group_struct(data, result, buffer, buflen);

    if (ret == -1) {
        *errnop = 2;
        DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getgrgid_lock :: Group pack failed\n"))
    }

    if (ret == -2) {
        *errnop = 34;
        DRETURN(NSS_STATUS_TRYAGAIN, printf("_nss_zk_getgrgid_lock :: Group pack failed - try again\n"))
    }

    DRETURN(NSS_STATUS_SUCCESS, printf("_nss_zk_getgrgid_lock :: Group pack OK\n"))

    clean:
    if (users)zk_delete_list(users);
    if (data)zk_delete_group_pack(data);
    if (groups)zk_delete_list(groups);
    if (zh)destroy_zk(zh);
    return ret;
}

enum nss_status _nss_zk_getgrgid_r(gid_t gid, struct group *result, char *buffer, size_t buflen, int *errnop) {
    int ret;
    PD(printf("_nss_zk_getgrgid_r :: Got request\n"));
    NSS_ZK_LOCK();
    ret = _nss_zk_getgrgid_lock(gid, result, buffer, buflen, errnop);
    NSS_ZK_UNLOCK();
    PD(printf("_nss_zk_getgrgid_r :: Got request\n"));
    return ret;
}

enum nss_status _nss_zk_getgrnam_lock(const char *name, struct group *result, char *buffer, size_t buflen,
                                      int *errnop) {
    int ret = 0;
    int ID = 0;
    int i = 0;
    char *tmp;

    zk_group_pack *data = 0;

    zk_list *users = 0;

    zhandle_t *zh = init_zk();
    if (!zh) DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getgrnam_lock :: Unable to init zookeeper client\n"))

    if (zk_create_group_pack(&data) != ZOK) DRETURN(NSS_STATUS_NOTFOUND,
                                                    printf("_nss_zk_getgrnam_lock :: Unable to create group_pack (malloc)\n"))

    ret = zk_get_users(zh, &users);
    if (ret != ZOK) DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getgrnam_lock :: Unable to list users\n"))

    for (i = 0; i < users->count; i++) {
        if (zk_get_user_group_id(zh, users->data[i], name, &ID, 0) == ZOK) {
            if (data->gid == 0) {
                data->gid = ID;
                data->name = malloc(strlen(name) + 1);
                memcpy(data->name, name, strlen(name) + 1);
            }
            else if (data->gid != ID) DRETURN(NSS_STATUS_NOTFOUND,
                                              printf("_nss_zk_getgrnam_lock :: Group ID mismatch %d %d\n", data->gid,
                                                     ID))

            if (data->users_len < NSS_GROUPS_USERS_MAX_SIZE) {
                tmp = malloc(strlen(users->data[i]) + 1);
                memcpy(tmp, users->data[i], strlen(users->data[i]) + 1);
                data->users[data->users_len] = tmp;
                data->users_len++;
                tmp = 0;
            }
        }
    }

    if (data->gid == 0) DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getgrnam_lock :: Group name %s not found\n", name))

    ret = pack_group_struct(data, result, buffer, buflen);

    if (ret == -1) {
        *errnop = 2;
        DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_getgrnam_lock :: Group pack failed\n"))
    }

    if (ret == -2) {
        *errnop = 34;
        DRETURN(NSS_STATUS_TRYAGAIN, printf("_nss_zk_getgrnam_lock :: Group pack failed - try again\n"))
    }

    DRETURN(NSS_STATUS_SUCCESS, printf("_nss_zk_getgrnam_lock :: Group pack OK\n"))

    clean:
    if (users)zk_delete_list(users);
    if (data)zk_delete_group_pack(data);
    if (zh)destroy_zk(zh);
    return ret;
}

enum nss_status _nss_zk_getgrnam_r(const char *name, struct group *result, char *buffer, size_t buflen, int *errnop) {
    int ret;
    PD(printf("_nss_zk_getgrnam_r :: Got request\n"));
    NSS_ZK_LOCK();
    ret = _nss_zk_getgrnam_lock(name, result, buffer, buflen, errnop);
    NSS_ZK_UNLOCK();
    PD(printf("_nss_zk_getgrnam_r :: Request res %d\n", ret));
    return ret;
}

enum nss_status
_nss_zk_initgroups_dyn_lock(const char *user, gid_t group, long int *start, long int *size, gid_t **groupsp,
                            long int limit,
                            int *errnop) {
    int ret = 0;
    int ID = 0;
    int i = 0;

    zk_list *groups = 0;

    zhandle_t *zh = init_zk();
    if (!zh) DRETURN(NSS_STATUS_NOTFOUND, printf("_nss_zk_initgroups_dyn_lock :: Unable to init zookeeper client\n"))

    if (zk_get_user_groups(zh, user, &groups) != ZOK) DRETURN(NSS_STATUS_NOTFOUND,
                                                              printf("_nss_zk_initgroups_dyn_lock :: Unable to find %s user\n",
                                                                     user))

    //realloc data if it is not enought -- nss allow it here
    if (groups->count + *start > *size) {
        long int newsize = *size;

        if (limit <= 0) // allocate wo limits
            newsize = groups->count + *start;
        else if (*size != limit) // allocate to limit
            newsize = limit;

        if (newsize != *size) {
            gid_t *tgroups = *groupsp;
            gid_t *newgroups = 0;

            newgroups = realloc(tgroups, newsize * sizeof(*tgroups));
            if (newgroups) {
                *groupsp = newgroups;
                *size = newsize;
            }
        }
    }

    for (i = 0; i < groups->count; i++) {
        if (zk_get_user_group_id(zh, user, groups->data[i], &ID, 0) != ZOK) DRETURN(NSS_STATUS_NOTFOUND,
                                                                                    printf("_nss_zk_initgroups_dyn_lock :: unable to get group id %s %s\n",
                                                                                           user, groups->data[i]))

        if (group != ID) {
            (*groupsp)[(*start)++] = ID;
        }
    }

    DRETURN(NSS_STATUS_SUCCESS, printf("_nss_zk_initgroups_dyn_lock :: ok - found %ld groups\n", *start))

    clean:
    if (groups)zk_delete_list(groups);
    if (zh)destroy_zk(zh);
    return ret;
}

enum nss_status
_nss_zk_initgroups_dyn(const char *user, gid_t group, long int *start, long int *size, gid_t **groupsp, long int limit,
                       int *errnop) {
    int ret;
    PD(printf("_nss_zk_initgroups_dyn :: Got request\n"));
    NSS_ZK_LOCK();
    ret = _nss_zk_initgroups_dyn_lock(user, group, start, size, groupsp, limit, errnop);
    NSS_ZK_UNLOCK();
    PD(printf("_nss_zk_initgroups_dyn :: Request res %d\n", ret));
    return ret;
}
