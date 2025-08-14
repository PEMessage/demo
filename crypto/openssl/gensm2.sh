#!/bin/bash

set -xe
# gen private
openssl ecparam -name SM2 -genkey -noout -out sm2-private.pem
# derive public
openssl ec -in sm2-private.pem -pubout -out sm2-public.pem

# print info
openssl ec -in sm2-private.pem -noout -text
