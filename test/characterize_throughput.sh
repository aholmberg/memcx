#!/usr/bin/env bash

for pool_size in `seq 10`; do
  for client_thread_count in `seq 10`; do
    ./tput.exe $pool_size $client_thread_count 10000 || exit
  done
done
