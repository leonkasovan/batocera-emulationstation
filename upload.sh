#!/bin/bash
strip -s emulationstation
sftp root@192.168.1.15 <<EOF
put emulationstation
EOF
