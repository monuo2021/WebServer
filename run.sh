#!/bin/bash
export WEBSERVER_DB_USER="monuo"
export WEBSERVER_DB_PASSWD="TLtl2584380333"
export WEBSERVER_DB_NAME="TinyWebServer"
# ./server -p 9006 -l 1 -m 0 -o 1 -s 10 -t 10 -c 1 -a 1
# 开启日志
./server -p 9006 -l 0 -m 0 -o 1 -s 10 -t 10 -c 0 -a 1