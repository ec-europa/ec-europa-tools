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
    'aliases' => array('lis'),
    'options' => array(
      'verbose' => 'Give a complete reports'
    ),
  );

  return $items;
}

/**
 * Drush command callback. drush_body_fix_list_impacted_sites.
 */
function drush_body_fix_list_impacted_sites() {
  $verbose = drush_get_option('verbose', 1);
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
  $field_info = field_info_field('body');
  $flag = $field_info['translatable'];

  drush_print('');
  drush_print(dt('Body translatable: @flag', array('@flag' => $flag)));

  // 4. Retrieving content type having translation mode other than Entity
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
    if ($total) {
      $field_checks = _body_fix_check_content_type_translation($machine_name, $trans_mode, 'body', $flag);
      $results[$trans_mode][$machine_name] = array(
        'checked_fields' => $field_checks,
        'total' => $total,
      );
    }
  }

  foreach ($results as $mode => $bundle_info) {
    $trans_label = _body_fix_list_get_translation_mode_label($mode);
    drush_print('');
    drush_print(dt('Problematic Content types for translation mode : "@mode"', array('@mode' => $trans_label)));
    drush_print('=========================================================');
    $problem_exist = FALSE;
    if ($bundle_info) {
      $header = array(
        'Related entity_id',
        'Related revision_id',
        'Set language',
      );
      foreach ($bundle_info as $machine_name => $info) {
        $field_checks = $info['checked_fields'];
        $content_total = $info['total'];
        if ($field_checks['data']) {
          $data = $field_checks['data'];
          $param = array(
            '@type' => $machine_name,
            '@amount' => count($data),
            '@total' => $content_total
          );
          drush_print(dt('WARNING: @type - orphan in the field DATA table: @amount on @total', $param));

          if ($verbose) {
            $values = array();
            $values[] = $header;
            foreach ($data as $orphan) {
              $value = array(
                $orphan->entity_id,
                $orphan->revision_id,
                $orphan->language,
              );
              $values[] = $value;
            }
            drush_print(dt('List of found records:'));
            drush_print('+++++++++++++++++++++++++');
            drush_print_table($values, TRUE);
          }
          $problem_exist = TRUE;
        }

        if ($field_checks['revision']) {
          $revision = $field_checks['revision'];
          drush_print(dt('WARNING: @type - orphan in the field REVISION table: @amount', array('@type' => $machine_name, '@amount' => count($revision))));

          if ($verbose) {
            $values = array();
            $values[] = $header;
            foreach ($field_checks['revision'] as $orphan) {
              $value = array(
                $orphan->entity_id,
                $orphan->revision_id,
                $orphan->language,
              );
              $values[] = $value;
            }
            drush_print(dt('List of found records:'));
            drush_print('+++++++++++++++++++++++++');
            drush_print_table($values, TRUE);
          }
          $problem_exist = TRUE;
        }
      }
    }

    if (!$problem_exist) {
      drush_print(dt('OK: no orphan found.'));
    }
  }

  drush_print('');
  // 5. Detect if the duplicates exists in field_data_body.
  $duplicates = db_query("SELECT DISTINCT b1.entity_id, b1.bundle as bundle, b1.revision_id, u.uid as user_id,
    u.name as user_name
    FROM node as n INNER JOIN field_data_body as b1 ON b1.entity_id = n.nid
    INNER JOIN field_data_body as b2 ON b2.entity_id = n.nid
    INNER JOIN node_revision as nr ON n.vid = nr.vid
    INNER JOIN users as u ON nr.uid = u.uid WHERE b1.revision_id = b2.revision_id AND
    b1.language = 'und' AND b2.language <> 'und'
    AND ( b1.body_value <> b2.body_value OR b1.body_summary <> b2.body_summary)");

  if (!empty($duplicates)) {
    $count = 0;
    $values = array();
    $values[] = array(
      dt('Content ID'),
      dt('Revision ID'),
      dt('Content type'),
      dt('User ID'),
      dt('User Name'),
    );

    foreach ($duplicates as $duplicate) {
      $count++;
      $value = array(
        $duplicate->entity_id,
        $duplicate->revision_id,
        $duplicate->bundle,
        $duplicate->user_id,
        $duplicate->user_name,
      );
      $values[] = $value;
    }

    drush_print(dt('NOK @nr contents found with duplicates in the field_body_table', array('@nr' => $count)));
    if ($verbose && $count) {
      drush_print('List of problematic contents:');
      drush_print('+++++++++++++++++++++++++++++');
      drush_print_table($values, TRUE);
    }
  }
  else {
    drush_print(dt('OK No duplicates found in the field_body_table'));
  }

  drush_print('');
  drush_print(dt('End body_fix_list_impacted_sites for "@name" site (@url)', array('@name' => $site_name, '@url' => $base_url)));
  drush_print('');
}

function _body_fix_check_content_type_translation($bundle, $trans_mode, $field_name, $field_translatable  = FALSE) {
  $problem_nr = array(
    'data' => 0,
    'revision' => 0
  );
  $data_table_name = 'field_data_' . $field_name;
  $revision_table_name = 'field_revision_' . $field_name;
  if ($field_translatable) {
    switch($trans_mode) {
      case 0:
        // TODO FIND A IMPROVED WAY.
        // Data check
        $query = db_select($data_table_name, 'fd');
        $query->condition('fd.entity_type', 'node')
              ->condition('fd.bundle', $bundle)
              ->condition('fd.language', LANGUAGE_NONE, '<>');
        $query->fields('fd', array('entity_id', 'revision_id', 'language'));
        $problem_nr['data'] = $query->execute()->fetchAll();

        // Revision check.
        $query = db_select($revision_table_name, 'fr');
        $query->condition('fr.entity_type', 'node')
              ->condition('fr.bundle', $bundle)
              ->condition('fr.language', LANGUAGE_NONE, '<>');
        $query->fields('fr', array('entity_id', 'revision_id', 'language'));
        $problem_nr['revision'] = $query->execute()->fetchAll();
        break;

      case 1:
      case 2:
        // TODO FIND A IMPROVED WAY.
        // Data check
        $query = db_select($data_table_name, 'fd');
        $query->join('node', 'n', 'fd.entity_id = n.nid AND fd.revision_id = n.vid AND fd.language = n.language');
        $query->condition('fd.entity_type', 'node')
              ->condition('fd.bundle', $bundle);
        $query->fields('fd', array('entity_id'));
        $ok_ids = $query->execute()->fetchCol();

        $query = db_select($data_table_name, 'fd');
        $query->condition('fd.entity_type', 'node')
              ->condition('fd.bundle', $bundle)
              ->condition('fd.entity_id', $ok_ids, 'NOT IN');
        $query->fields('fd', array('entity_id', 'revision_id', 'language'));
        $problem_nr['data'] = $query->execute()->fetchAll();

          // Revision check.
        $query = db_select($revision_table_name, 'fr');
        $query->join('node_revision', 'nr', 'fr.entity_id = nr.nid AND fr.revision_id = nr.vid');
        $query->join('node', 'n', 'n.nid = nr.nid');
        $query->condition('fr.entity_type', 'node')
              ->condition('fr.bundle', $bundle)
              ->where('fr.language = n.language');
        $query->fields('fr', array('entity_id'));
        $ok_ids = $query->execute()->fetchCol();

        $query = db_select($revision_table_name, 'fr');
        $query->condition('fr.entity_type', 'node')
              ->condition('fr.bundle', $bundle)
              ->condition('fr.entity_id', $ok_ids, 'NOT IN');
        $query->fields('fr', array('entity_id', 'revision_id', 'language'));
        $problem_nr['revision'] = $query->execute()->fetchAll();
        break;

      case 4:
        // TODO FIND A IMPROVED WAY.
        // Data check
        $query = db_select($data_table_name, 'fd');
        $query->leftJoin('entity_translation', 'et', 'fd.entity_id = et.entity_id AND fd.revision_id = et.revision_id
          AND et.entity_type = fd.entity_type AND fd.language = et.language');
        $query->condition('fd.entity_type', 'node')
              ->condition('fd.bundle', $bundle)
              ->isNull('et.language');
        $query->fields('fd', array('entity_id', 'revision_id', 'language'));
        $problem_nr['data'] = $query->execute()->fetchAll();

        // Revision check.
        $query = db_select($revision_table_name, 'fr');
        $query->leftJoin('entity_translation_revision', 'etr', 'fr.entity_id = etr.entity_id AND fr.revision_id = etr.revision_id
          AND etr.entity_type = fr.entity_type AND fr.language = etr.language');
        $query->condition('fr.entity_type', 'node')
          ->condition('fr.bundle', $bundle)
          ->isNull('etr.language');
        $query->fields('fr', array('entity_id', 'revision_id', 'language'));
        $query_results = $query->execute()->fetchAll();
        $problem_nr['revision'] = $query_results;
        break;

    }
  }
  else {
    // Data check
    $query = db_select($data_table_name, 'fd')
      ->condition('fd.entity_type', 'node')
      ->condition('fd.language', LANGUAGE_NONE, '<>');
    $query->fields('fd', array('entity_id', 'revision_id', 'language'));
    $problem_nr['data'] = $query->execute()->fetchAll();

    // Revision check.
    $query = db_select($revision_table_name, 'fr')
      ->condition('fr.entity_type', 'node')
      ->condition('fr.language', LANGUAGE_NONE, '<>');
    $query->fields('fr', array('entity_id', 'revision_id', 'language'));
    $problem_nr['revision'] = $query->execute()->fetchAll();
  }

  return $problem_nr;
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
 *   Otherwise 'Undefined'.
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

  return 'Undefined';
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
