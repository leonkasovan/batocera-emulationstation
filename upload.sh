#!/bin/bash
strip -s emulationstation
sftp root@192.168.1.13 <<EOF
put emulationstation
EOF
