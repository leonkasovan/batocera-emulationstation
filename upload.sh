#!/bin/bash
aarch64-linux-gnu-strip -s emulationstation
sftp root@192.168.1.10 <<EOF
put emulationstation
put portmaster.db
cd /userdata/system/scripts
put screenshot_auto_scrapper.sh
EOF
