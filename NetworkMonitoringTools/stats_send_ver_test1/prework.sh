#!/bin/sh

hostname=`ls /home/kamiuck/myworkspace/mininet-host/`

for i in $hostname
do
chmod +x main_pro
cp -f main_pro /home/kamiuck/myworkspace/mininet-host/$i/
rm -f /home/kamiuck/myworkspace/mininet-host/$i/log/*
done
