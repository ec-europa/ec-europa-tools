<?php
/**
 * @file
 * Drush commands file. Provides commands for analysing multilingual settings.
 *
 * Related to NEXTEUROPA-13241
 */

/**
 * Implements hook_drush_command().
 */
function multilingual_audit_drush_command() {
  $items = array();

  $items['check-fields'] = array(
    'description' => 'List entity reference fields instances',
    'aliases' => array('macf'),
    'outputformat' => array(
      'default' => 'table',
      'pipe-format' => 'csv',
      'field-labels' => array(
        'field-name' => 'Field name',
        'type' => 'Field type',
        'entities' => 'Entity:Bundle(count)',
        'has_content' => 'Has content ?'
      ),
      'table-metadata' => array(
        'process-cell' => '_drush_field_info_process_cell',
      ),
      'output-data-type' => 'format-table',
    ),
  );

  $items['check-modules'] = array(
    'description' => 'List modules that might cause troubles',
    'aliases' => array('macm'),
    'outputformat' => array(
      'default' => 'table',
      'pipe-format' => 'list',
      'field-labels' => array(
        'package' => 'Package',
        'name' => 'Name',
        'type' => 'Type',
        'status' => 'Status',
        'version' => 'Version'
      ),
      'output-data-type' => 'format-table',
    ),
  );

  $items['check-wb-mod'] = array(
    'description' => 'List node types using workbench moderation module',
    'aliases' => array('macwbm'),
    'outputformat' => array(
      'default' => 'table',
      'pipe-format' => 'csv',
      'field-labels' => array(
        'node-type' => 'Node type',
        'count' => 'Count',
      ),
      'table-metadata' => array(
        'process-cell' => '_drush_field_info_process_cell',
      ),
      'output-data-type' => 'format-table',
    ),
  );

  return $items;
}

/**
 * Drush command callback.
 *
 * This callback check the Workbench Moderation configuration.
 */
function drush_multilingual_audit_check_wb_mod() {
  $result = array();

  $entity_info = entity_get_info('node');
  foreach (array_keys($entity_info['bundles']) as $content_type) {
    $options = variable_get('node_options_' . $content_type, array());

    if (in_array('revision', $options) && in_array('moderation', $options)) {
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', $content_type);
      $count = $query->count()->execute();

      $result[] = array(
        'node-type' => $content_type,
        'count' => $count,
      );
    }
  }

  if (empty($result)) {
    drush_print('Workbench Moderation is not enabled on this site.');
  }

  return $result;

}

/**
 * Drush command callback.
 *
 * This callback check status of particular modules.
 */
function drush_multilingual_audit_check_modules() {
  $result = array();

  $module_to_check = array(
    'i18n_select',
    'title',
  );

  $extension_info = drush_get_extensions(FALSE);
  uasort($extension_info, '_drush_pm_sort_extensions');

  foreach ($extension_info as $key => $extension) {
    if (!in_array($extension->name, $module_to_check)) {
      unset($extension_info[$key]);
      continue;
    }

    $extension_type = drush_extension_get_type($extension);
    if (!in_array($extension_type, array('module'))) {
      unset($extension_info[$key]);
      continue;
    }
    $status = drush_get_extension_status($extension);
    if (!in_array($status, array('enabled'))) {
      unset($extension_info[$key]);
      continue;
    }

    $row['package'] = $extension->info['package'];
    $row['name'] = $extension->label;
    $row['type'] = ucfirst($extension_type);
    $row['status'] = ucfirst($status);
    $row['version'] = @$extension->info['version'];

    $result[$key] = $row;
    unset($row);
  }

  return $result;
}

/**
 * Drush command callback.
 *
 * This callback check the type of particular fields.
 */
function drush_multilingual_audit_check_fields() {
  $result = array();

  $site = drush_get_context('DRUSH_TARGET_SITE_ALIAS');

  if (empty($site)) {
    $site = variable_get('site_name');
  }

  drush_print(sprintf("Checking fields on site: %s", $site));

  // TODO: Add modules to check in the command options.
  $fields_to_check = array(
    'taxonomy_term_reference',
    'entityreference',
    'field_collection',
    'paragraphs',
  );

  foreach ($fields_to_check as $field_type) {
    $map = field_info_fields();

    $info = array_filter(field_info_fields(), function($item) use ($field_type){
      return $item['type'] == $field_type;
    });

    foreach ($info as $field_name => $field) {
      $has_content = FALSE;
      $owners = array();
      $total = 0;

      foreach($map[$field_name]['bundles'] as $entity => $bundles) {
        foreach($bundles as $bundle) {
          try {
            $query = new EntityFieldQuery();
            $query->entityCondition('entity_type', $entity)
              ->entityCondition('bundle', $bundle);
            $count = $query->count()->execute();
          } catch (Exception $e) {
            drush_log($e->getMessage(), 'error');
          }

          $has_content =  dt('No');
          if ($count >= 1) {
            $query = db_select('node', 'n');
            $query->condition('n.type', $bundle)
                  ->condition('n.tnid', 0, '<>');
            $query->where('n.tnid <> n.nid');
            $query->addExpression('count(n.nid)');
            $translation_results = $query->execute()->fetchField();
            $has_content = ($translation_results) ? dt('Translated!') : dt('Yes');
          }
          $total += $count;
          $owners[] = sprintf("%s:%s(%s)", $entity, $bundle, $count);
        }
      }

      sort($owners);

      $result[$field_name] = array(
        'field-name' => $field_name,
        'type' => $field['type'],
        'entities' => implode(', ', $owners),
        'has_content' => sprintf("%s (%s)", $has_content, $total)
      );
    }

    if (empty($result)) {
      drush_print(sprintf('There is no %s field type enabled on this site.', $field_type));
    }
  }

  return $result;
}
