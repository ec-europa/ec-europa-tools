# drush Multilingual Audit

Drupal 7 set of Drush commands to check Features uses.
This is related to ticket: NEPT-89

## Installation

* Copy the *.inc file into your ```~/.drush/``` directory or make a symlinks of the ec-europa-tools in your ```~/.drush/``` directory.
* Run ```drush cc drush```

## Usage

```
drush check-wysiwyg-text-formats or drush aliases
```
Check for each text format used in site: 
* If it uses a WYSIWYG profile;
* If the used WYSIWYG profile has CKEditor LITE plugin activated;
* If field revisions using it contain tracked changes.

