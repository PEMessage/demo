#!/bin/sh

docker build -t fzy_parent_dir_builder - <<EOF
# Dockerfile with caching optimizations
FROM alpine:3.18 AS builder

# Install dependencies in a single layer for caching
RUN apk add \
    musl-dev gcc

WORKDIR /w
EOF

# Run the container to copy the binary to host
docker run --rm \
  -e UID="$(id -u)" \
  -e GID="$(id -g)" \
  -v "$PWD":/w \
  fzy_parent_dir_builder sh -c 'pwd ; sh ./run.sh'
