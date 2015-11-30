<?php
/**
 * @file
 * Drush command that checks if a subsite overwrites modules from the multisite.
 */

/**
 * Implements hook_drush_command().
 */
function check_used_modules_drush_command() {

  $items['drush-check-used-modules'] = array(
    'description' => 'Check for module existance and status in a Multisite instance',
    'aliases' => array('dcum'),
    'callback' => '_drush_command_check_used_modules',
    'arguments' => array(
      'multisite_path' => 'The path to multisite instalation',
      'modules_tocheck' => 'The modules you want to check',
    ),
  );

  return $items;
}

/**
 * Callback for the drush-demo-command command.
 * @param string $multisite_path
 *   multisite absolute path
 */
function _drush_command_check_used_modules($multisite_path = '', $modules_tocheck) {

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
  drush_print("\n");
  drush_print($subsite_path);

  //The subsite does not have custom modules
  if(!is_dir($subsite_modules_path)) {
    drush_print('The subsite does not have custom modules.');
    return;
  }

  // Replace spaced betweend modules with pipe (|) for grepping
  $modules_tocheck = str_replace(' ', '|', $modules_tocheck);

  //Execute the drush command to find our the enabled modules
  exec("drush pml --type=module --status=enabled --no-core --format=csv | grep -E '".$modules_tocheck."'", $modules);

  $found_modules = array();
  foreach($modules as $module) {
    $module_info = explode(',',$module);

    //Get the module name
    preg_match('#\((.*?)\)#', $module_info[count($module_info) - 2], $result);
    $found_modules[] = $result[1];

  }

  //printing the results
  if(!empty($found_modules)) {
    drush_print("-------------------------------------------------------");
    drush_print("Found Modules");
    drush_print("-------------------------------------------------------");
    drush_print(implode("\n",$found_modules));
  }
  else {
    drush_print('Module(s) not found.');
  }

}

