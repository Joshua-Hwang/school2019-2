#!/bin/bash

for i in {500..5000..100}
do
    ./worstlp.py $i 2>/dev/null | ./solvelp 2>> log.log 1>/dev/null
done
