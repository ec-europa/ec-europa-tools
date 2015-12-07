# Check Duplicate Modules
The purpose of these scrips is to generate an inventory of the subsites that use a certain (set of) module(s).

It contains a bash script (*checkUsedModules.sh*) for looping through the subsites and applying the *drush dcum* command (*check_used_modules.drush.inc*) for generating the report for a specific subsite

## Installation
Upload the *check_used_modules.drush.inc* file under your user's .drush folder. e.g. /home/my_user/.drush/check_used_modules.drush.inc

Upload checkUsedModules.sh and give execute permission. e.g. chmod o+x ./checkUsedModules.sh

## Usage
### Single subsite check
Navigate to the subsite folder (/sites/subsite) and run: *drush dcum 'module1 module2'*

### Batch check
./checkSubsitesModules.sh path_to_root_multisite_folder 'module1 module2'
