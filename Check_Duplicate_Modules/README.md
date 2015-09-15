# Check Duplicate Modules
The purpose of these scrips is to generate an inventory of the subsites which overwrite modules already provided by a multisite platform.

It contains a bash script (*checkSubsitesModules.sh*) for looping through the subsites and applying the *drush dcmd* command and the *drush dcmd* command (*check_module_duplicates.drush.inc*) for generating the report for a specific subsite

## Installation
Upload the *check_module_duplicates.drush.inc* file under your user's .drush folder. e.g. /home/my_user/.drush/check_module_duplicates.drush.inc

Upload checkSubsitesModules.sh and give execute permission. e.g. chmod o+x ./checkSubsitesModules.sh

## Usage
### Single subsite check
Navigate to the subsite folder (/sites/subsite) and run: *drush dcmd*

### Batch check
./checkSubsitesModules.sh path_to_root_multisite_folder
