# drush Multilingual Audit

Drupal 7 set of Drush commands to check Multilingual stuff.
This is related to ticket: NEXTEUROPA-13241

## Installation

* Copy the *.inc file into your ```~/.drush/``` directory or make a symlinks of the ec-europa-tools in your ```~/.drush/``` directory.
* Run ```drush cc drush```

## Usage

```
drush check-modules or drush macm
```
Check if the site is using the modules:
* [i18n:multilingual select](https://www.drupal.org/project/i18n)
* [Title](https://www.drupal.org/project/title)

```
drush check-fields or drush macf
```
Check if the site is using fields with particular types: 
* Entity Reference (entityreference)
* Field Collection (field_collection)

```
drush check-wb-mod or drush macwbm
```
Check on which node types the Workbench Moderation tool is used and the number of node using it.
