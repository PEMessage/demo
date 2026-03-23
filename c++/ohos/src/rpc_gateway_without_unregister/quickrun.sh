#!/bin/bash
set -x

cd /home/zhuojw/demo_new/c++/ohos

trap "kill -- -$$" EXIT

# Start SamgrServer
./out/samgr_server/app &
sleep 1

# Start Backend Server (rpc_gateway_without_unregister_server)
./out/rpc_gateway_without_unregister_server/app &
sleep 1

# Start Gateway (rpc_gateway_without_unregister_gateway)
./out/rpc_gateway_without_unregister_gateway/app &
sleep 1

echo "========================================"
echo "Test 1: First client registration"
echo "========================================"
./out/rpc_gateway_without_unregister_client/app &
CLIENT_PID=$!
sleep 20
echo "Killing first client (PID: $CLIENT_PID)..."
kill $CLIENT_PID 2>/dev/null
wait $CLIENT_PID 2>/dev/null


echo ""
echo "========================================"
echo "Test 2: Second client registration (after first died)"
echo "========================================"
./out/rpc_gateway_without_unregister_client/app &
CLIENT_PID=$!
sleep 20
echo "Killing second client (PID: $CLIENT_PID)..."
kill $CLIENT_PID 2>/dev/null
wait $CLIENT_PID 2>/dev/null
sleep 10

#
# echo ""
# echo "========================================"
# echo "Test 3: Third client registration"
# echo "========================================"
# ./out/rpc_gateway_without_unregister_client/app &
# CLIENT_PID=$!
# sleep 5
# echo "Killing third client (PID: $CLIENT_PID)..."
# kill $CLIENT_PID 2>/dev/null
# wait $CLIENT_PID 2>/dev/null
# sleep 2
#
# echo ""
# echo "========================================"
# echo "All tests completed. Keeping services alive for observation..."
# echo "========================================"
# sleep 5
