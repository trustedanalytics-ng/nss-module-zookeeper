docker build -t nss-module-zookeeper:stable -f Dockerfile .
docker run -it -d nss-module-zookeeper:stable /bin/bash

export container=`docker ps | grep nss-module-zookeeper:stable | cut -d" " -f1 | head -1`

docker exec -i $container sh -c "cd zookeeper-3.4.9/src/c && echo 'Run ./configure on zookeeper' && ./configure && echo 'Edit config.h file' && sed -i '/^#define HAVE_GETLOGIN 1/d' config.h && sed -i '/^#define HAVE_GETPWUID_R 1/d' config.h && sed -i '/^#define HAVE_GETUID 1/d' config.h && echo 'Run make install '&& make install && echo 'Compile nss-module-zookeeper' && cd ../../../nss-module-zookeeper/ && make compile"
docker cp $container:/nss-module-zookeeper/libnss_zk.so.2 libnss_zk.so.2
docker cp $container:/zookeeper-3.4.9/src/c/.libs/libzookeeper_mt.so.2.0.0 libzookeeper_mt.so.2

mkdir lib64
mv libnss_zk.so.2 lib64/
mv libzookeeper_mt.so.2 lib64/

tar -cvzf zookeeper_nss-1.0-el7-x86_64.tar.gz lib64/*
