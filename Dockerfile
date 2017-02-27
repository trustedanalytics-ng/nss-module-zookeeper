FROM centos:7.2.1511
RUN yum groupinstall -y "Development tools"
RUN curl http://www-eu.apache.org/dist/zookeeper/zookeeper-3.4.9/zookeeper-3.4.9.tar.gz | tar zxvf - && cd zookeeper-3.4.9/src/c && ./configure
COPY . /nss-module-zookeeper
ENV CFLAGS "-O2 -D_FORTIFY_SOURCE=2 -fstack-protector -Wformat -Wformat-security -fPIE -fPIC"
