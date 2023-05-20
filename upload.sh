#!/bin/bash
strip -s emulationstation
sftp root@192.168.1.14 <<EOF
put emulationstation
#put test
EOF
