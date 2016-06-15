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
function _drush_command_check_used_modules($modules_to_check) {

  //subsite absolute path
  $subsite_path = $_SERVER['PWD'];

  //Print the subsite path
  drush_print("\n");
  drush_print($subsite_path);

  // Replace spaces between modules with pipe (|) and escape them for regular
  // expression matching.
  $modules_to_check = preg_split('/\s+/', $modules_to_check);
  $modules_to_check = array_map(
    function ($module) {
      return preg_quote($module);
    },
    $modules_to_check
  );
  $modules_to_check = '/' . implode('|', $modules_to_check) . '/';
  
  $extensions = system_rebuild_module_data();

  // Filter on enabled modules only.
  $extensions = array_filter($extensions, function ($project) {
    return 1 == $project->status && 'module' == $project->type;
  });

  $found_modules = array();

  foreach ($extensions as $extension) {

    if (1 !== preg_match($modules_to_check, $extension->name) &&
        1 !== preg_match($modules_to_check, $extension->info->name)
    ) {
      continue;
    }

    /**
     * Since js_injector and css_injector were implemented as platform "dependencies"
     * We perform a specific query to check if the site also has data in the database
     * for this module.
     * @todo replace this with a generic schema implementation
     */
    if ($extension->name == 'js_injector' || $extension->name == 'css_injector') {
      $query = db_query("SELECT crid FROM {".$extension->name."_rule} LIMIT 1")->rowCount();
      if ($query == 1) {
        $found_modules[] = $extension->name." (with data in database!)";
      }
      else {
        $found_modules[] = $extension->name;
      }
    }
    else if ($extension->name) {
      $found_modules[] = $extension->name;
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

