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
function _drush_command_check_used_modules($modules_tocheck) {

  //subsite absolute path
  $subsite_path = $_SERVER['PWD'];

  //Navigate to the subsite folder
  chdir($subsite_path);

  //Print the subsite path
  drush_print("\n");
  drush_print($subsite_path);

  // Replace spaced betweend modules with pipe (|) for grepping
  $modules_tocheck = str_replace(' ', '|', $modules_tocheck);

  //Execute the drush command to find our the enabled modules
  exec("drush pml --type=module --status=enabled --no-core --format=csv | grep -E '".$modules_tocheck."'", $modules);

  $found_modules = array();
  foreach($modules as $module) {

    $module_info = explode(',',$module);

    //Get the module name, module name is located at $result[1]
    preg_match('#\((.*?)\)#', $module_info[count($module_info) - 2], $result);

    /**
     * Since js_injector and css_injector were implemented as platform "dependencies"
     * We perform a specific query to check if the site also has data in the database
     * for this module.
     * @todo replace this with a generic schema implementation
     */
    if ($result[1] == 'js_injector' || $result[1] == 'css_injector') {
      $query = db_query("SELECT crid FROM {".$result[1]."_rule} LIMIT 1")->rowCount();
      if ($query == 1) {
        $found_modules[] = $result[1]." (with data in database!)";
      }
      else {
        $found_modules[] = $result[1];
      }
    }
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

