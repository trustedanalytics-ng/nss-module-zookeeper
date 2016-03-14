#include <stdio.h>
#include "zk_nss.h"

int main(int count, char ** args) {
    int i = 0;
    int ret = 0;

    if(count!=3) {
        printf("Not enought argiments -- user `prog` {exists|id|groups} user\n");
        return 1;
    }

    zhandle_t * u = init_zk();

    if(!strcmp(args[1], "id"))
    {
        ret = zk_get_user_id(u, args[2], &i, 0);

        if(ret == ZNONODE)
        {
            printf("Node not found\n");
            return ZNONODE;
        }
        else if(ret == ZOK)
        {
            printf("All ok - %s - %d\n", args[2], i);
            return 0;
        }
        else
        {
            printf("Error %d\n", ret);
            return ret;
        }
    }
    else if(!strcmp(args[1], "groups"))
    {
        zk_list * zk=0;

        ret = zk_get_user_groups(u, args[2], &zk);

        if(ret!=ZOK)
        {
            printf("zk request failed %d\n", ret);
            return ret;
        }
        else
        {
            printf("Found %d groups for user %s\n", zk->count, args[2]);
            for(i=0;i<zk->count;i++)
                printf("%d: %s\n", i,zk->data[i]);
        }
    }

    return 0;
}
