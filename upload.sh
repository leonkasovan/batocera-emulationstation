#!/bin/bash
strip -s emulationstation
sftp root@192.168.1.16 <<EOF
put emulationstation
EOF
