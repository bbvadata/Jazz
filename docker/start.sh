#!/bin/bash

cd /home/jazz || return 1

./jazz start

tail -f /tmp/jazz.log
