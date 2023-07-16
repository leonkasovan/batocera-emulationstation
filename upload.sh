#!/bin/bash
aarch64-linux-gnu-strip -s emulationstation
sftp root@192.168.1.12 <<EOF
cd /userdata/roms/bin/es
put portmaster.db
put gen_db_from_port_master.lua
cd /userdata/roms/bin
put emulationstation
cd /userdata/system/scripts
put screenshot_auto_scrapper.sh
EOF
#cd /userdata/system/scripts
#put screenshot_auto_scrapper.sh
#cd /userdata/roms/bin/es
#put portmaster.db