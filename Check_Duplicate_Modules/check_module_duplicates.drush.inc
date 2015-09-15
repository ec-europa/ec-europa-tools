<?php
/**
 * @file
 * Drush command that checks if a subsite overwrites modules from the multisite.
 */

/**
 * Implements hook_drush_command().
 */
function check_module_duplicates_drush_command() {

  $items['drush-check-module-duplicates'] = array(
    'description' => 'Check for module duplicate in a Multisite instance',
    'aliases' => array('dcmd'),
    'callback' => '_drush_command_check_module_duplicates',
    'arguments' => array(
      'multisite_path' => 'The path to multisite instalation',
    ),
  );

  return $items;
}

/**
 * Callback for the drush-demo-command command.
 * @param string $multisite_path
 *   multisite absolute path
 */
function _drush_command_check_module_duplicates($multisite_path = '') {

  //subsite absolute path
  $subsite_path = $_SERVER['PWD'];

  //multisite absolute path
  if($multisite_path == '') {
    $multisite_path = getcwd();
  }

  //Paths from the multisite where to look for duplicates
  $core_paths = array(
    $multisite_path.'/sites/all/modules',
    $multisite_path.'/profiles');

  $enabled_local_modules = array();
  $subsite_modules_path = $subsite_path.'/modules';

  //Navigate to the subsite folder
  chdir($subsite_path);

  //Print the subsite path
  drush_print($subsite_path);

  //The subsite does not have custom modules
  if(!is_dir($subsite_modules_path)) return;

  //Execute the drush command to find our the enabled modules
  exec("drush pml --type=module --status=enabled --no-core --format=csv", $modules);

  foreach($modules as $module) {
    $module_info = explode(',',$module);

    //Get the module name
    preg_match('#\((.*?)\)#', $module_info[count($module_info) - 2], $result);

    //Create an array of enabled modules
    if(!empty($result)) {
      //Find if the enabled module is a local module or not (provided by the multisite)
      $search_command = "find " .$subsite_modules_path. " -type d -name '" . $result[1] . "'";
      exec($search_command, $local_module);
      if (!empty($local_module)) {
        foreach ($local_module as $found_module) {
          $info_file = $found_module . '/' . $result[1] . '.info';

          //check if the folder is a module folder or not
          if (file_exists($info_file)) {
            $enabled_local_modules[] = array('name' => $result[1], 'version' => $module_info[2]);
          }
        }
      }
      //reset the $local_module variable
      $local_module = array();
    }
  }

  //Check the multisite modules
  foreach($core_paths as $path) {
    foreach ($enabled_local_modules as $module_info) {

      //search for folders with the same name as the subsite module
      $command = "find " . $path . " -type d -name '" . $module_info['name'] . "'";
      exec($command, $multisite_module);

      if (!empty($multisite_module)) {
        foreach ($multisite_module as $found_module) {
          $info_file = $found_module . '/' . $module_info['name'] . '.info';

          //check if the folder is a module folder or not
          if (file_exists($info_file)) {
            $info = drupal_parse_info_file($info_file);
            drush_print($info['name'] . ',' . $module_info['name'] . ',' . $module_info['version'] . ',' . $info['version']);
          }
        }
      }

      //reset the $multisite_module variable
      $multisite_module = array();
    }
  }

}

