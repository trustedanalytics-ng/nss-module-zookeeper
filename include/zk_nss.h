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

#ifndef ZK_NSS_H
#define ZK_NSS_H

#include "configuration.h"

#include <nss.h>
#include <pwd.h>
#include <grp.h>
#include <zookeeper/zookeeper.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

// nss -- return macro
#define RETURN(ID) { ret = ID; goto clean; }
#define DRETURN(ID, log) { ret = ID; PD(log); goto clean; }
#define PD(log) { D log; }
#define DEBUG() zk_get_debug()
#define D if(DEBUG())

// nss -- data definitions
typedef struct String_vector zk_list;
typedef struct Stat zk_stat;

#define NSS_GROUPS_USERS_MAX_SIZE 16
struct _zk_group_pack {
    char *name;
    int gid;
    char *users[NSS_GROUPS_USERS_MAX_SIZE];
    int users_len;
};

typedef struct _zk_group_pack zk_group_pack;


//nss -- zookeeper
zhandle_t *init_zk();

int zk_get_debug();

void destroy_zk(zhandle_t *zh);

int zk_add_auth(zhandle_t *zh);

const char *get_user_group_znode_path(const char *user, const char *group);

const char *get_user_znode_path(const char *user);

int zk_get_user(zhandle_t *zh, const char *user);

int zk_get_znode_id(zhandle_t *zh, const char *znode, int *ID, zk_stat *zks);

int zk_get_user_id(zhandle_t *zh, const char *user, int *ID, zk_stat *zks);

int zk_get_user_group_id(zhandle_t *zh, const char *user, const char *group, int *ID, zk_stat *zks);

int zk_get_user_groups(zhandle_t *zh, const char *user, zk_list **zk);

int zk_get_users(zhandle_t *zh, zk_list **zk);

// -- functions to create structs
int zk_create_list(zk_list **zkg);

void zk_delete_list(zk_list *zkg);

int zk_create_group_pack(zk_group_pack **gp);

void zk_delete_group_pack(zk_group_pack *gp);


//nss -- passwd
enum nss_status _nss_zk_setpwent(void);

enum nss_status _nss_zk_endpwent(void);

enum nss_status _nss_zk_setpwent_lock(void);

enum nss_status _nss_zk_endpwent_lock(void);

enum nss_status _nss_zk_getpwent_lock(struct passwd *result, char *buffer, size_t buflen, int *errnop);

enum nss_status _nss_zk_getpwuid_lock(uid_t uid, struct passwd *result, char *buffer, size_t buflen, int *errnop);

enum nss_status _nss_zk_getpwnam_lock(const char *name, struct passwd *result, char *buffer, size_t buflen,
                                      int *errnop);

enum nss_status _nss_zk_getpwent_r(struct passwd *result, char *buffer, size_t buflen, int *errnop);

enum nss_status _nss_zk_getpwuid_r(uid_t uid, struct passwd *result, char *buffer, size_t buflen, int *errnop);

enum nss_status _nss_zk_getpwnam_r(const char *name, struct passwd *result, char *buffer, size_t buflen, int *errnop);


//nss -- groups
enum nss_status _nss_zk_getgrgid_r(gid_t gid, struct group *result, char *buffer, size_t buflen, int *errnop);

enum nss_status _nss_zk_getgrnam_r(const char *name, struct group *result, char *buffer, size_t buflen, int *errnop);

enum nss_status _nss_zk_initgroups_dyn(const char *user, gid_t group, long int *start, long int *size, gid_t **groupsp,
                                       long int limit, int *errnop);

#endif
