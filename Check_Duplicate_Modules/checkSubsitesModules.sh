#!/bin/bash
# This script loops through the subsites of a Drupal multisite instance
# and runs the drush dcmd command

multisite_path=$1
sites_folder=$multisite_path"/sites"
exclude_folders=("all" "default")   #folder list to exclude from the multisite /sites folder

# generic function for checking if an array contains a value
containsElement () {
  local e
  for e in "${@:2}"; do
     if [ "$e" == "$1" ]; then
        return 1
     fi
  done
  return 0
}

# check if the given path is a valid multisite root folder
if [ ! -d $sites_folder ]; then
  echo "You have to specify a working multisite root folder path"
  echo "Usage: `basename $0` absolute_path"
  exit 0;
fi

# loop through subsites
for i in $(find $sites_folder -maxdepth 1 -mindepth 1 -type d)    # loop through the folders
  do
    containsElement $(basename $i) "${exclude_folders[@]}"        # checks if the folder is all or default folder
    if [ $? == 0 ]; then                                          # it's not all or default folde
       cd $i                                                      # navigate to the subsite's folder
       drush dcmd                                                 # run the drush command
    fi
  done
