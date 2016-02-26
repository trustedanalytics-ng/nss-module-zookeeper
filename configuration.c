#include <string.h>
#include "zk_nss.h"

const char *zk_getenv(const char *name, const char *defaultvalue) {
    const char *value;
    char *tmp = 0;
    value = getenv(name);
    if (!value)
        value = defaultvalue;
    if (value) {
        tmp = malloc(strlen(value) + 1);
        memcpy(tmp, value, strlen(value) + 1);
    }
    return tmp;
}

const char *zk_getfileconfig(const char *file, const char *name, const char *defaultvalue) {
    char buffor[1024];
    char *ret = 0, *tmp = 0;
    int i;
    FILE *fp = fopen(file, "r");

    if (!fp)
        goto file_nf;

    while (fgets(buffor, sizeof(buffor), fp)) {
        tmp = strchr(buffor, '=');

        if (!tmp)
            continue;
        *tmp = 0;
        tmp++;
        if (!strcmp(buffor, name)) {
            i = strlen(tmp) + 1;
            ret = malloc(i);
            memcpy(ret, tmp, i);
            for (i = 0; i < strlen(ret); i++)
                if (ret[i] == 10)
                    ret[i] = 0;
            goto clean;
        }
        tmp = 0;
    }
    file_nf:
    if (!defaultvalue)
        goto clean;
    ret = malloc(strlen(defaultvalue) + 1);
    memcpy(ret, defaultvalue, strlen(defaultvalue) + 1);
    clean:
    if (fp)
        fclose(fp);
    return ret;
}

const char *zk_getfileconfig_n(const char *file1, const char *file2, const char *name, const char *defaultvalue) {
    const char *value = zk_getfileconfig(file1, name, 0);
    if (!value)
        value = zk_getfileconfig(file2, name, defaultvalue);
    return value;
}

const char *zk_getfileconfig_a(const char *name, const char *defaultvalue, int rootdef) {
    if (rootdef)
        return zk_getfileconfig_n(GET_CONFIG_FILE_LOC(), GET_HGM_CONFIG_FILE_LOC(), name, defaultvalue);
    return zk_getfileconfig_n(GET_HGM_CONFIG_FILE_LOC(), GET_CONFIG_FILE_LOC(), name, defaultvalue);
}

const char *zk_getconfig(const char *envname, const char *filename, const char *defaultvalue, int rootdef) {
    const char *value = 0;
    if (envname)
        value = zk_getenv(envname, 0);
    if (filename && !value)
        value = zk_getfileconfig_a(filename, defaultvalue, rootdef);
    return value;
}