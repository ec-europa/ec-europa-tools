<?php
/**
 * @file
 * Drush implementation to control NEXTEUROPA-10038 fix results.
 */

/**
 * Implements hook_drush_command().
 */
function body_fix_drush_command() {
  $items = array();
  $items['list-impacted-sites'] = array(
    'description' => 'Lists the sites impacted by the body translatable issue.',
    'aliases' => array('lis'),
    'options' => array(
      'complete-report' => 'Gives a complete reports with problematic node data (0 & 1).',
      'list-revision-items' => 'List problematic records in revision table too (0 & 1).',
    ),
  );
  $items['translation-mode-statistics'] = array(
    'description' => 'Gives statistics about contents created created after a specific time.',
    'arguments' => array(
      'timestamp' => 'The timestamp of the specific time to use as reference in the report.',
    ),
    'aliases' => array('tms'),
  );

  return $items;
}

/**
 * Drush command callback. drush_body_fix_translation_mode_statistics.
 */
function drush_body_fix_translation_mode_statistics($timestamp = 1472562000) {

  global $base_url;
  $site_name = variable_get('site_name', 'Unnamed site');

  drush_print(dt('Start body_fix_list_impacted_sites for "@name" site (@url)', array('@name' => $site_name, '@url' => $base_url)));
  drush_print('');
  require_once DRUPAL_ROOT . '/includes/install.inc';

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

  // 2. Retrieving content type having translation mode other than
  // Entity translation.
  $types = _body_fix_list_get_content_type_by_translation_mode(array(0, 1, 2));
  if (!$types) {
    drush_print(dt('All content types are using "Entity translation" mode'));
  }

  // For next steps, we load need the full list of content types using
  // body fields.
  $types = _body_fix_list_get_content_type_by_translation_mode(array(0, 1, 2, 4));

  $update_start = $timestamp;
  $type_names = array_keys($types);
  $query = db_select('field_revision_body', 'frb');
  $query->join('node_revision', 'nr', 'nr.vid = frb.revision_id');
  $query->condition('frb.entity_type', 'node')
    ->condition('nr.timestamp', $update_start, '>=')
    ->condition('frb.bundle', $type_names, 'IN');
  $query->fields('frb', array('bundle'));
  $query->addExpression('count(frb.entity_id)', 'revision_count');
  $query->groupBy("frb.bundle");
  $revision_results = $query->execute()->fetchAllAssoc('bundle');

  $query = db_select('field_data_body', 'fdb');
  $query->join('node', 'n', 'n.nid = fdb.entity_id');
  $query->condition('fdb.entity_type', 'node')
    ->condition('n.changed', $update_start, '>=')
    ->condition('fdb.bundle', $type_names, 'IN');
  $query->fields('fdb', array('bundle'));
  $query->addExpression('count(fdb.entity_id)', 'content_count');
  $query->groupBy("fdb.bundle");
  $content_results = $query->execute()->fetchAllAssoc('bundle');

  $query = db_select('field_data_body', 'fdb');
  $query->join('node', 'n', 'n.nid = fdb.entity_id');
  $query->condition('fdb.entity_type', 'node');
  $query->fields('fdb', array('bundle'));
  $query->addExpression('count(fdb.entity_id)', 'total_count');
  $query->groupBy("fdb.bundle");
  $total_results = $query->execute()->fetchAllAssoc('bundle');

  drush_print(' ');
  $date = new DateTime("@" . $timestamp);
  $date->setTimezone(new DateTimeZone('Europe/Brussels'));
  $str_date = $date->format('d/m/y H:i:s');
  drush_print(dt('Statistics about content created/edited after @date', array('@date' => $str_date)));
  drush_print('==================================================================');
  $values = array();
  $values[] = array(
    dt('CONTENT TYPE'),
    dt('TRANSLATION MODE'),
    dt('NODE #'),
    dt('NODE REVISION #'),
    dt('NODE TOTAL #'),
  );
  foreach ($types as $machine_name => $type) {
    $content_nr = 0;
    if (isset($content_results[$machine_name]) && $content_results[$machine_name]->content_count) {
      $content_nr = $content_results[$machine_name]->content_count;
    }
    $revision_nr = 0;
    if (isset($revision_results[$machine_name]) && $revision_results[$machine_name]->revision_count) {
      $revision_nr = $revision_results[$machine_name]->revision_count;
    }
    $total = 0;
    if (isset($total_results[$machine_name]) && $total_results[$machine_name]->total_count) {
      $total = $total_results[$machine_name]->total_count;
    }

    $trans_mode = $type->trans_mode;
    $trans_label = _body_fix_list_get_translation_mode_label($trans_mode);
    $values[] = array(
      $machine_name,
      $trans_label,
      $content_nr,
      $revision_nr,
      $total,
    );
  }
  drush_print_table($values, TRUE);
  drush_print('');
  drush_print(dt('End body_fix_list_impacted_sites for "@name" site (@url)', array('@name' => $site_name, '@url' => $base_url)));
  drush_print('');
}

/**
 * Drush command callback. drush_body_fix_list_impacted_sites.
 */
function drush_body_fix_list_impacted_sites() {
  $complete_report = drush_get_option('complete-report', 0);
  $message_table_sep = ($complete_report) ? ':' : '';
  require_once DRUPAL_ROOT . '/includes/install.inc';
  global $base_url;
  $site_name = variable_get('site_name', 'Unnamed site');

  drush_print(dt('Start body_fix_list_impacted_sites for "@name" site (@url)', array('@name' => $site_name, '@url' => $base_url)));
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

  drush_print(' ');
  drush_print('+++++++++++++++++++++++++++++++++++++++++++++++++++');
  drush_print(' ');

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
  drush_print('+++++++++++++++++++++++++++++++++++++++++++++++++++');
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
    drush_print('================================================================================');
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
        if ($field_checks) {
          drush_print('');
          drush_print(dt('@type - WARNING: Possible problems detected!', array('@type' => strtoupper($machine_name))));
          drush_print('----------------------------------------------------');
          if ($field_checks['data']) {
            $data = $field_checks['data'];
            $param = array(
              '@amount' => count($data),
              '@total' => $content_total,
              '@separator' => $message_table_sep,
            );
            drush_print('');
            drush_print(dt('>>> Orphans in the field DATA table: @amount on @total@separator', $param));

            if ($complete_report) {
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
              drush_print_table($values, TRUE);
            }
            $problem_exist = TRUE;
          }

          if ($field_checks['revision']) {
            $revision = $field_checks['revision'];
            $list_revision_items = drush_get_option('list-revision-items', 0);
            $param = array(
              '@amount' => count($revision),
              '@separator' => ($list_revision_items) ? $message_table_sep : '',
            );
            drush_print('');
            drush_print(dt('>>> Orphans in the field REVISION table: @amount@separator', $param));
            if ($complete_report && $list_revision_items) {
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
              drush_print_table($values, TRUE);
            }
            $problem_exist = TRUE;
          }
        }
      }
    }

    if (!$problem_exist) {
      drush_print(dt('OK: no orphan found.'));
    }
  }

  drush_print(' ');
  drush_print('+++++++++++++++++++++++++++++++++++++++++++++++++++');
  drush_print(' ');

  // 5. Detect if the duplicates exists in field_data_body.
  drush_print(dt('Duplicated values scanning.'));
  drush_print('===========================');
  $duplicates = db_query("SELECT DISTINCT b1.entity_id, b1.bundle as bundle, b1.revision_id, u.uid as user_id,
    u.name as user_name
    FROM node as n INNER JOIN field_data_body as b1 ON b1.entity_id = n.nid
    INNER JOIN field_data_body as b2 ON b2.entity_id = n.nid
    INNER JOIN node_revision as nr ON n.vid = nr.vid
    INNER JOIN users as u ON nr.uid = u.uid WHERE b1.revision_id = b2.revision_id AND
    b1.language = 'und' AND b2.language <> 'und'
    AND ( b1.body_value <> b2.body_value OR b1.body_summary <> b2.body_summary)");
  $row_count = $duplicates->rowCount();

  if ($row_count) {
    $param = array(
      '@nr' => $row_count,
      '@separator' => $message_table_sep,
    );
    drush_print(dt('NOK @nr contents found with duplicates in the field_body_table@separator', $param));
    if ($complete_report && $row_count) {
      $values = array();
      $values[] = array(
        dt('Content ID'),
        dt('Revision ID'),
        dt('Content type'),
        dt('User ID'),
        dt('User Name'),
      );

      foreach ($duplicates as $duplicate) {
        $value = array(
          $duplicate->entity_id,
          $duplicate->revision_id,
          $duplicate->bundle,
          $duplicate->user_id,
          $duplicate->user_name,
        );
        $values[] = $value;
      }

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

/**
 * Checks the language relationship between nodes and field values.
 *
 * It checks if this relationship is coherent compared to the field translatable
 * flag the translation mode and the content type.
 *
 * @param string $bundle
 *   The machine name of the content type of node to test.
 * @param int $trans_mode
 *   The translation mode (0, 1, 2, 4).
 * @param string $field_name
 *   The machine of the field to test.
 * @param bool $field_translatable
 *   The field translatable flag value.
 *
 * @return array
 *   Associative array listing field values having a incoherent language
 *   compared to its related node:
 *   - 'data': array concering the field related field_data table
 *     (e.g. field_data_body);
 *   - 'revision': array concering the field related field_revision table
 *     (e.g. field_revision_body).
 */
function _body_fix_check_content_type_translation($bundle, $trans_mode, $field_name, $field_translatable = FALSE) {
  $problem_nr = array(
    'data' => 0,
    'revision' => 0,
  );
  $data_table_name = 'field_data_' . $field_name;
  $revision_table_name = 'field_revision_' . $field_name;
  if ($field_translatable) {
    switch ($trans_mode) {
      case 0:
        // Multilingual support disabled.
        // field_data check.
        $query = db_select($data_table_name, 'fd');
        $query->condition('fd.entity_type', 'node')
            ->condition('fd.bundle', $bundle)
            ->condition('fd.language', LANGUAGE_NONE, '<>');
        $query->fields('fd', array('entity_id', 'revision_id', 'language'));
        $problem_nr['data'] = $query->execute()->fetchAll();

        // Field_revision check.
        $query = db_select($revision_table_name, 'fr');
        $query->condition('fr.entity_type', 'node')
            ->condition('fr.bundle', $bundle)
            ->condition('fr.language', LANGUAGE_NONE, '<>');
        $query->fields('fr', array('entity_id', 'revision_id', 'language'));
        $problem_nr['revision'] = $query->execute()->fetchAll();
        break;

      case 1:
      case 2:
        // Multilingual support enable + content translation case.
        // field_data check.
        $query = db_select($data_table_name, 'fd');
        $query->leftJoin('node', 'n', 'fd.entity_id = n.nid AND fd.revision_id = n.vid AND fd.language = n.language');
        $query->condition('fd.entity_type', 'node')
            ->condition('fd.bundle', $bundle)
            ->isNull('n.nid');
        $query->fields('fd', array('entity_id', 'revision_id', 'language'));
        $problem_nr['data'] = $query->execute()->fetchAll();

        // Field_revision check.
        $query = db_select($revision_table_name, 'fr');
        $query->leftJoin('node_revision', 'nr', 'fr.entity_id = nr.nid AND fr.revision_id = nr.vid');
        $query->leftJoin('node', 'n', 'n.nid = nr.nid AND fr.language = n.language');
        $query->condition('fr.entity_type', 'node')
            ->condition('fr.bundle', $bundle)
            ->isNull('n.nid');
        $query->fields('fr', array('entity_id', 'revision_id', 'language'));
        $problem_nr['revision'] = $query->execute()->fetchAll();
        break;

      case 4:
        // Entity translation case.
        // field_data check.
        $query = db_select($data_table_name, 'fd');
        $query->leftJoin('entity_translation', 'et', 'fd.entity_id = et.entity_id AND fd.revision_id = et.revision_id
          AND et.entity_type = fd.entity_type AND fd.language = et.language');
        $query->condition('fd.entity_type', 'node')
            ->condition('fd.bundle', $bundle)
            ->isNull('et.language');
        $query->fields('fd', array('entity_id', 'revision_id', 'language'));
        $problem_nr['data'] = $query->execute()->fetchAll();

        // Field_revision check.
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
    // Translatable flag = 0.
    // Data check.
    $query = db_select($data_table_name, 'fd')
        ->condition('fd.entity_type', 'node')
        ->condition('fd.bundle', $bundle)
        ->condition('fd.language', LANGUAGE_NONE, '<>');
    $query->fields('fd', array('entity_id', 'revision_id', 'language'));
    $problem_nr['data'] = $query->execute()->fetchAll();

    // Revision check.
    $query = db_select($revision_table_name, 'fr')
        ->condition('fr.entity_type', 'node')
        ->condition('fr.bundle', $bundle)
        ->condition('fr.language', LANGUAGE_NONE, '<>');
    $query->fields('fr', array('entity_id', 'revision_id', 'language'));
    $problem_nr['revision'] = $query->execute()->fetchAll();
  }

  if (!$problem_nr['data'] && !$problem_nr['revision']) {
    return FALSE;
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
 * @param string $field_name
 *   The machine name of the field on which the node type list must be
 *   filtered.
 *
 * @return array
 *   An array of node types, as objects, keyed by the type.
 */
function _body_fix_list_get_content_type_by_translation_mode($modes = array(), $field_name = 'body') {
  $types = node_type_get_types();
  if (!$modes) {
    $modes = array(0, 1, 2, 4);
  }
  $filtered_types = array();

  foreach ($types as $machine_name => $type_info) {
    $translation_mode = variable_get('language_content_type_' . $machine_name);
    $is_field_used = field_info_instance('node', $field_name, $machine_name);
    if (in_array($translation_mode, $modes) && $is_field_used) {
      $type_info->trans_mode = $translation_mode;
      $filtered_types[$machine_name] = $type_info;
    }
  }

  return $filtered_types;
}
