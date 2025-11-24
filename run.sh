#!/bin/bash

# shellcheck disable=SC2124
cmd="$@"
start_time=$(date)
current_path=$(pwd)
cd $current_path || exit

ulimit -c unlimited
export COREDUMP_FILENAME=./
sudo sysctl -w kernel.core_pattern="core.%t"

for arg in $cmd; do
     if [[ $arg == "ps" ]]; then
          ps aux|grep app | awk '{print $6/1024 "MB"}'
     fi

     if [[ $arg == "htop" ]]; then
          htop -p $(pidof app)
     fi
     if [[ $arg == "val" ]]; then
        cd bin
        valgrind --leak-check=full --show-leak-kinds=definite,indirect,possible --track-origins=yes --log-file=./val.txt ./app --http=8088 --console=1 --rpc=7788 --db=43.143.239.75

     fi
done

#dotnet-dump analyze ./core_3070075.3070075 -c "dumpheap -stat" | grep -v "Class Name" | sort -k2,2nr >a.txt
