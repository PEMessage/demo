#!/bin/bash

set -x

# --inventory/-i: specify inventory host file or host list
# --module-name/-m: specify action to execute
ansible -i inventory.ini -m ping server
ansible -i inventory.ini -m ping local
ansible -i inventory.ini -m ping all



ansible-playbook -i inventory.ini playbook.yml
