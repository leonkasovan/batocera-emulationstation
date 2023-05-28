#!/bin/bash
# This is an gameStop events script for scrapping screenshot just taken.
# Put this file in /userdata/system/scripts and chmod a+x filename
 
# It's good practice to set all constants before anything else.
# logfile=/userdata/system/logs/batocera_custom_script.log
directory="/userdata/roms/anbernic/screenshots"
rom_directory="/userdata/roms"

# echo "Batocera Custom Script (/userdata/system/scripts)" > $logfile
# echo "[`date`] at `pwd`" >> $logfile
# echo "Args: $@" >> $logfile

rom_filename=$5
system=$2
rom_extension="${rom_filename##*.}"
rom_title=$(basename "$rom_filename" ".$rom_extension")

# Case selection for first parameter parsed, our event.
case $1 in
	gameStop)
		# Commands here will be executed on the stop of every game.
		# Iterate through files in the directory
		for file in "$directory"/*; do
			if [[ -f "$file" ]]; then        # Check if it's a regular file
				if [[ $file == *"$rom_title"* ]]; then # Compare the filename with the specific string
					# echo "Match found: $file" >> $logfile
					source="$file"
					target="$rom_directory/$system/images/$rom_title-image.png"
				fi
			fi
		done
		mkdir -p "$rom_directory/$system/images"
		cp "$source" "$target"
	;;
esac


