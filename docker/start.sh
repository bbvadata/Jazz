#!/bin/bash

cd /home/jadmin || return 1

./jazz start

tail -f /tmp/jazz.log
