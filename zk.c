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

void my_watcher_func(zhandle_t *zzh, int type, int state,
                     const char *path, void *watcherCtx) {
}

zhandle_t *init_zk() {
    const char *adres = GET_CONFIG_QUORUM_ADRESS();
    if (!adres)
        return 0;

    int log_level = ZOO_LOG_LEVEL_ERROR;
    D log_level = ZOO_LOG_LEVEL_DEBUG;
    zoo_set_debug_level(log_level);
    zhandle_t *zh = zookeeper_init(adres, my_watcher_func, 2000, 0,
                                   NULL, 0);

    free((char *)adres);
    if (!zh)
        return zh;

    if (zk_add_auth(zh) != ZOK) {
        destroy_zk(zh);
        zh = 0;
    }


    return zh;
}

void destroy_zk(zhandle_t *zh) {
    zookeeper_close(zh);
}

int zk_get_debug() {
    const char *d = GET_DEBUG();
    if (!d)
        return 0;
    int res = d[0] ? 1 : 0;
    free((char *)d);
    return res;
}

int zk_add_auth(zhandle_t *zh) {
    const char *method = GET_AUTH_SCHEME();
    const char *user = GET_AUTH_USER();
    const char *pass = GET_AUTH_PASS();
    int ret = 0;

    if (method) {
        if (!user || !pass) DRETURN(ZAUTHFAILED,
                                    printf("zk_add_auth :: auth method is specified, but can not find user or/and password\n"))

        char *tmp = malloc(strlen(user) + strlen(pass) + 2);
        memcpy(tmp, user, strlen(user));
        tmp[strlen(user)] = ':';
        memcpy(tmp + strlen(user) + 1, pass, strlen(pass) + 1);

        ret = zoo_add_auth(zh, method, tmp, strlen(tmp), 0, 0);
        free(tmp);
        if (ret != ZOK) DRETURN(ZAUTHFAILED, printf("zk_add_auth :: auth failed\n"))
    }

    clean:
    if (method)free((char *)method);
    if (user)free((char *)user);
    if (pass)free((char *)pass);
    return ret;
}

const char *get_user_znode_path(const char *user) {
    const char *znodepath = GET_CONFIG_ZNODE_ADRESS();
    size_t len1 = strlen(znodepath);
    size_t len2 = strlen(user);
    char *tmp = malloc(len1 + len2 + 2);

    if (!tmp)
        return 0;

    memcpy(tmp, znodepath, len1);
    tmp[len1] = '/';
    memcpy(tmp + len1 + 1, user, len2 + 1);
    free((char *)znodepath);
    return tmp;
}

const char *get_user_group_znode_path(const char *user, const char *group) {
    const char *znodepath = GET_CONFIG_ZNODE_ADRESS();
    size_t len1 = strlen(znodepath);
    size_t len2 = strlen(user);
    size_t len3 = strlen(group);
    char *tmp = malloc(len1 + len2 + len3 + 3);

    if (!tmp)
        return 0;

    memcpy(tmp, znodepath, len1);
    tmp[len1] = '/';
    memcpy(tmp + len1 + 1, user, len2);
    tmp[len1 + 1 + len2] = '/';
    memcpy(tmp + len1 + len2 + 2, group, len3 + 1);
    free((char *)znodepath);
    return tmp;
}

int zk_get_znode_id(zhandle_t *zh, const char *znode, int *ID, zk_stat *zks) {
    char buff[32];
    int bsize = 32;
    *ID = 0;

    if (!znode)
        return ZAPIERROR;

    int u = zoo_get(zh, znode, 0, buff, &bsize, zks);

    if (u != ZOK) {
        goto clean;
    }

    buff[bsize] = 0;

    if (bsize <= 0) {
        u = ZBADARGUMENTS;
        goto clean;
    }

    //validate chars for atoi
    for (bsize = 0; bsize < strlen(buff); bsize++)
        if (buff[bsize] < '0' || buff[bsize] > '9') {
            u = ZBADARGUMENTS;
            goto clean;
        }

    *ID = atoi(buff);
    clean:
    return u;
}

int zk_get_user_id(zhandle_t *zh, const char *user, int *ID, zk_stat *zks) {
    const char *user_znode = get_user_znode_path(user);
    int ret = zk_get_znode_id(zh, user_znode, ID, zks);
    free((char *)user_znode);
    return ret;
}

int zk_get_user_group_id(zhandle_t *zh, const char *user, const char *group, int *ID, zk_stat *zks) {
    const char *group_znode = get_user_group_znode_path(user, group);
    int ret = zk_get_znode_id(zh, group_znode, ID, zks);
    free((char *)group_znode);
    return ret;
}

int zk_create_list(zk_list **zkg) {
    zk_list *zk = (zk_list *) malloc(sizeof(zk_list));

    if (!zk)
        return 1;

    zk->count = 0;
    zk->data = 0;

    *zkg = zk;

    return 0;
}

void zk_delete_list(zk_list *zkg) {
    int i = 0;
    if (!zkg)
        return;

    if (zkg->data != 0) {
        for (i = 0; i < zkg->count; i++) {
            if (zkg->data[i])
                free(zkg->data[i]);
        }
        free(zkg->data);
    }
    free(zkg);
}

int zk_list_znode(zhandle_t *zh, const char *znode, zk_list **zk) {
    int ret = 0;
    zk_list *zku = 0;

    if (!znode || zk_create_list(&zku)) {
        ret = ZAPIERROR;
        goto clean;
    }

    ret = zoo_get_children(zh, znode, 0, zku);
    if (ret != ZOK)
        goto clean;

    *zk = zku;
    zku = 0;

    clean:
    if (zku)
        zk_delete_list(zku);
    return ret;
}

int zk_get_user_groups(zhandle_t *zh, const char *user, zk_list **zk) {
    int ret = 0;
    const char *user_znode = get_user_znode_path(user);
    ret = zk_list_znode(zh, user_znode, zk);
    free((char *)user_znode);
    return ret;
}

int zk_get_users(zhandle_t *zh, zk_list **zk) {
    int ret = 0;
    const char *znode = GET_CONFIG_ZNODE_ADRESS();
    ret = zk_list_znode(zh, znode, zk);
    free((char *)znode);
    return ret;
}

int zk_get_user(zhandle_t *zh, const char *user) {
    int ID = 0;
    return zk_get_user_id(zh, user, &ID, 0);
}


int zk_create_group_pack(zk_group_pack **gp) {
    int i = 0;
    *gp = 0;
    zk_group_pack *pack = (zk_group_pack *) malloc(sizeof(zk_group_pack));
    if (!pack)
        return -1;

    pack->gid = 0;
    pack->name = 0;
    pack->users_len = 0;
    for (i = 0; i < NSS_GROUPS_USERS_MAX_SIZE; i++)
        pack->users[i] = 0;
    *gp = pack;
    return 0;
}

void zk_delete_group_pack(zk_group_pack *gp) {
    if (!gp)
        return;
    int i = 0;
    for (i = 0; i < gp->users_len; i++) {
        free(gp->users[i]);
    }
    if (gp->name)
        free(gp->name);
    free(gp);
}
