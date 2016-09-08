<?php
/**
 * @file
 * Drush implementation to fix NEXTEUROPA-10038.
 */

/**
 * Implements hook_drush_command().
 */
function body_fix_drush_command() {
  $items = array();
  $items['list-impacted-sites'] = array(
    'description' => 'List the sites impacted by the body translatable issue.',
    'arguments' => array(
      'timestamp' => 'The timestamp when the body translatable causing issue has been executed.',
    ),
  );

  return $items;
}

/**
 * Drush command callback. drush_body_fix_list_impacted_sites.
 */
function drush_body_fix_list_impacted_sites($timestamp = 1472562000) {
  require_once DRUPAL_ROOT . '/includes/install.inc';
  global $base_url;
  $site_name = variable_get('site_name', 'Unnamed site');

  drush_print(dt('Start body_fix_list_impacted_sites for"@name" site (@url)', array('@name' => $site_name, '@url' => $base_url)));
  drush_print('');

  // 1. Check the situation about the module "Nexteuropa Cod" module and the
  // execution of the hook_update_7003.
  $schema_version = drupal_get_installed_schema_version('nexteuropa_core');

  if ($schema_version == SCHEMA_UNINSTALLED) {
    drush_print(dt('"Nexteuropa Core" module is not enabled!'));
  }
  elseif ($schema_version < 7003) {
    drush_print(dt('hook_update_7003 of the "Nexteuropa Core module is NOT executed!'));
  }
  else {
    drush_print(dt('hook_update_7003 of the "Nexteuropa Core module is executed!'));
  }

  // 2. Current value for the body flag.
  $query = db_select('field_config', 'fc')->condition('fc.field_name', 'body');
  $query->fields('fc', array('translatable'));
  $result = $query->execute()->fetchField();

  drush_print('');
  drush_print(dt('Body translatable: @flag', array('@flag' => $result)));

  // 3. Retrieving content type having translation mode other than Entity
  // translation.
  $types = _body_fix_list_get_content_type_by_translation_mode(array(0, 1, 2, 4));

  $query = db_select('field_data_body', 'fdb');
  $query->join('node', 'n', 'fdb.entity_id = n.nid');
  $query->condition('fdb.entity_type', 'node');
  $query->fields('fdb', array('bundle'));
  $query->addExpression('count(fdb.entity_id)', 'total_count');
  $query->groupBy("fdb.bundle");
  $total_results = $query->execute()->fetchAllAssoc('bundle');

  drush_print(' ');
  $results = array(
    0 => array(),
    1 => array(),
    2 => array(),
    4 => array(),
  );
  foreach ($types as $machine_name => $type) {
    $total = 0;
    if (isset($total_results[$machine_name]) && $total_results[$machine_name]->total_count) {
      $total = $total_results[$machine_name]->total_count;
    }

    $trans_mode = $type->trans_mode;
    $results[$trans_mode][$machine_name] = $total;
  }

  foreach ($results as $mode => $bundle_info) {
    $trans_label = _body_fix_list_get_translation_mode_label($mode);
    drush_print('');
    drush_print(dt('Content types for translation mode : "@mode"', array('@mode' => $trans_label)));
    drush_print('=========================================================');
    if ($bundle_info) {
      foreach ($bundle_info as $machine_name => $content_nbr) {
        if ($content_nbr) {
          drush_print(dt('@type - number of content with a filled body: @number', array('@type' => $machine_name, '@number' => $content_nbr)));
        }
        else {
          drush_print(dt('@type: no content with a filled body', array('@type' => $machine_name)));
        }
      }
    }
    else {
      drush_print(dt('No content type uses this translation mode'));
    }
  }

  drush_print('');
  // 4. Detect if the duplicates exists in field_data_body.
  $duplicates = db_query("SELECT b1.entity_id as nid, b1.revision_id as vid, b1.bundle as bundle, n.uid as user_id, u.name as user_name
     FROM {field_data_body} as b1, field_data_body as b2, {node} as n, {users} as u WHERE b1.entity_id = n.nid AND n.uid = u.uid AND
     b1.entity_id = b2.entity_id AND b1.revision_id = b2.revision_id AND b1.language = 'und' AND b2.language <> 'und' AND
     ( b1.body_value <> b2.body_value OR b1.body_summary <> b2.body_summary)")->execute();

  if (is_array($duplicates) && !empty($duplicates)) {
    drush_print(dt('NOK duplicates found in the field_body_table'));
    foreach ($duplicates as $duplicate) {
      drush_print(dt('|Content ID|Revision ID|Content type|User ID|User Name|'));
      $param = array(
        '@nid' => $duplicate->nid,
        '@vid' => $duplicate->vid,
        '@bundle' => $duplicate->bundle,
        '@uid' => $duplicate->user_id,
        '@name' => $duplicate->user_name,
      );
      drush_print(dt('|@nid|@vid|@bundle|@uid|@name|', $param));
    }
  }
  else {
    drush_print(dt('OK No duplicates found in the field_body_table'));
  }

  drush_print('');
  drush_print(dt('End body_fix_list_impacted_sites for"@name" site (@url)', array('@name' => $site_name, '@url' => $base_url)));
  drush_print('');
}


/**
 * Retrieves the translation mode label for a mode code.
 *
 * @param int $mode
 *   The translation mode code amongst the following:
 *   - 0: Multilingual support disabled;
 *   - 1: Multilingual support enabled;
 *   - 2: Multilingual support with translation (Content translation);
 *   - 4: Multilingual support with field translation (Entity translation).
 *
 * @return string
 *   The translation mode label; I.E.:
 *   - 0: Multilingual support disabled;
 *   - 1: Multilingual support enabled;
 *   - 2: Content translation;
 *   - 4: Entity translation.
 */
function _body_fix_list_get_translation_mode_label($mode) {
  switch ($mode) {
    case 0:
      return "Multilingual support disabled";

    case 1:
      return 'Multilingual support enabled';

    case 2:
      return 'Content translation';

    case 4:
      return 'Entity translation';
  }
}

/**
 * Returns all the available node types for a specific translation modes.
 *
 * It is also filtered on the existence of body field in node type structure.
 *
 * @param array $modes
 *   Array containing translation mode codes of the  to use as filter>.
 *   Possible code are:
 *   - 0: Multilingual support disabled;
 *   - 1: Multilingual support enabled;
 *   - 2: Multilingual support with translation (Content translation);
 *   - 4: Multilingual support with field translation (Entity translation).
 *
 * @return array
 *   An array of node types, as objects, keyed by the type.
 */
function _body_fix_list_get_content_type_by_translation_mode($modes = array()) {
  $types = node_type_get_types();
  if (!$modes) {
    $modes = array(0, 1, 2, 4);
  }
  $filtered_types = array();

  foreach ($types as $machine_name => $type_info) {
    $translation_mode = variable_get('language_content_type_' . $machine_name);
    $is_body_used = field_info_instance('node', 'body', $machine_name);
    if (in_array($translation_mode, $modes) && $is_body_used) {
      $type_info->trans_mode = $translation_mode;
      $filtered_types[$machine_name] = $type_info;
    }
  }

  return $filtered_types;
}
