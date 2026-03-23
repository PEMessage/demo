#!/bin/bash
set -x

cd /home/zhuojw/demo_new/c++/ohos

trap "kill -- -$$" EXIT

#the rest of your code goes here
./out/samgr_server/app &
sleep 1

./out/rpc_gateway_server/app &
sleep 1

./out/rpc_gateway_gateway/app &
sleep 1

# Start Client for 8 seconds then kill it
./out/rpc_gateway_client/app &
CLIENT_PID=$!
sleep 8
kill $CLIENT_PID 2>/dev/null

# Keep Gateway and Server running to observe behavior after client dies
sleep 10


