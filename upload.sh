#!/bin/bash
aarch64-linux-gnu-strip -s emulationstation
sftp root@192.168.1.14 <<EOF
put emulationstation
cd /userdata/system/scripts
put screenshot_auto_scrapper.sh
EOF
