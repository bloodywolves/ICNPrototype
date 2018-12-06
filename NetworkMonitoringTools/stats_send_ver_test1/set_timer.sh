#!/bin/sh

##hour in $1, min in $2
read -p "input the hour:
" hour
read -p "input the min:
" min
awk -v var_hour="$hour" -v var_min="$min" -f set_timer_lst.awk timer.lst >timer.lst1
cp timer.lst1 timer.lst