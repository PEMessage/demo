#!/bin/bash

set -x

# --inventory/-i: specify inventory host file or host list
#                 or put in ansible.cfg(recommand)
# --module-name/-m: specify action to execute
ansible -m ping all
ansible -m ping server
ansible -m ping localhost

ansible -m gather_facts all



ansible-playbook playbook.yml
