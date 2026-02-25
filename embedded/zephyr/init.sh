#!/bin/bash

set -ex

WEST='uvx --with jsonschema --with pyelftools west'

if [ ! -d .west ] ; then
    (
        $WEST init -l demo
    )
fi

$WEST update -o=--depth=1 -n




