#!/usr/bin/env bash

id=1

for agent in master worker scheduler; do
  if [ ! -f "logloss-ps-piag-${agent}" ]; then
    echo "logloss-ps-piag-${agent} does not exist"
    exit -1
  fi
done

./logloss-ps-piag-scheduler -d 0 -s "*" 1>scheduler.log 2>scheduler.err &
pids[$(( id++))]=$!
./logloss-ps-piag-master -d 0 -l 1E-4 -m 127.0.0.1 -s 127.0.0.1 1>master.log 2>master.err &
pids[$(( id++))]=$!

for num in $(seq 5); do
  ./logloss-ps-piag-worker -d 0 -f $num -s 127.0.0.1 1>worker-$num.log 2>worker-$num.err &
  pids[$(( id++))]=$!
done

for pid in ${pids[@]}; do
  echo "Waiting for PID $pid to finish..."
  wait $pid
done
