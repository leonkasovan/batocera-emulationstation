#!/bin/bash
strip -s emulationstation
sftp root@192.168.1.12 <<EOF
put emulationstation
EOF
