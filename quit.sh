#!/bin/bash

directory="/usr/aarch64-linux-gnu/lib"  # Specify the directory path
directory="/usr/lib/aarch64-linux-gnu"
specific_string="thread"       # Specify the specific string

# Iterate through files in the directory
for file in "$directory"/*; do
    if [[ -f "$file" ]]; then        # Check if it's a regular file
        filename=$(basename "$file") # Extract the filename from the path

        # Compare the filename with the specific string
        if [[ $filename == *"$specific_string"* ]]; then
            echo "Match found: $filename"
        # else
            # echo "No match found: $filename"
        fi
    fi
done

