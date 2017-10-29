#!/bin/sh

export LIBLOCK_SERVER_CORE=10
export LIBLOCK_LOCK_NAME=multircl
sudo ./memcached -t 47 -u root

