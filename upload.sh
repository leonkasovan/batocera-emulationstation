#!/bin/bash
aarch64-linux-gnu-strip -s emulationstation
sftp root@192.168.1.18 <<EOF
cd /userdata/roms/bin
put emulationstation
EOF
#cd /userdata/system/scripts
#put screenshot_auto_scrapper.sh
#cd /userdata/roms/bin/es
#put portmaster.db