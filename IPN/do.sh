#!/bin/bash
confg1="zhuanfabiao226.click"
confg2="zhuanfabiao424.click"
cmd1='/home/pc-2/click-2.0.1/userlevel/click  /home/pc-2/click-2.0.1/conf/iptable/'"$confg1"' >/dev/null &' 
cmd2='/home/pc-2/click-2.0.1/userlevel/click  /home/pc-2/click-2.0.1/conf/iptable/'"$confg2"' >/dev/null  &'

flag=0
#flag=$1
while [ 1 ]
do
    if [ $flag -eq 0 ];then
        flag=1
        pid=`ps axu |grep "$confg2"| grep -v color| grep -v grep| awk '{print $2}'`
        echo "pid: "$pid
        if [ "$pid" != "" ];then
            sudo kill -9 $pid
        fi
        eval "$cmd1"
        date +%H.%M.%S.%N >> "shiyan"
    else
        pid=`ps axu |grep "$confg1"| grep -v color| grep -v grep| awk '{print $2}'`
        echo "pid: "$pid
        if [ "$pid" != "" ];then
            sudo kill -9 $pid
        fi
        eval "$cmd2"
        date +%H.%M.%S.%N >> "shiyan"
        flag=0
    fi
    sleep 60
done

