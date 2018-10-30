<?php

/**
 * @file
 * Drush commands file. Provides commands for analysing sites.
 *
 * Related to NEPT-2073
 */

/**
 * Implements hook_drush_command().
 */
function check_iframes_drush_command() {
  $items = array();
  
    $items['search-iframes'] = array(
    'description' => 'List node having iframes',
    'aliases' => array('chkif'),
    'outputformat' => array(
      'default' => 'table',
      'pipe-format' => 'csv',
      'field-labels' => array(
        'field_name' => 'Field Name',
        'field_type' => 'Field Type',
        'nodeid' => 'NId',
        'title' => 'Node Title',
        'is_avp' => 'AV Portal',
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
 * This callback checks where iframes are used in the site. .
 */
function drush_check_iframes_search_iframes() {
  $results = array();
  $site = drush_get_context('DRUSH_TARGET_SITE_ALIAS');

  if (empty($site)) {
    $site = variable_get('site_name');
  }

  drush_print(sprintf("Checking iframes on site: %s \n", $site));

  // Get the machine name of textfields
  $fields_to_check = array(
    'text',
    'text_long',
    'text_with_summary',
  );
  $fields_names = array();
  foreach ($fields_to_check as $field_type) {
    $fields = field_read_fields(array('entity_type' => 'node', 'type' => $field_type));
    $fields_names = array_merge($fields_names, array_keys($fields));
  }

  // Get all fields information
  $map = field_info_field_map(); 

  foreach ($fields_names as $field_name){
    $summary = FALSE;
    // Manage summary field in query
    $selected_db_field = array('entity_id', "{$field_name}_value");
    if ( "text_with_summary" === $map[$field_name]['type']) {
       $selected_db_field[] = "{$field_name}_summary";
       $summary = TRUE;
    }
   
    $result = db_select("field_data_{$field_name}", 'f')
      ->fields('f', $selected_db_field )
      ->condition('entity_type', 'node')
      ->condition('bundle', $map[$field_name]['bundles']['node'], 'IN')
      ->condition('deleted', 0)
      ->execute()
      ->fetchAll();

    foreach ($result as $row) {
      $row = (array) $row;
      $node = node_load($row['entity_id']);
      $html = render(field_view_field('node', $node, $field_name));
      if ($html) {
        $check_iframe = checksubsite_iframe($html);
        if ($check_iframe > 0) {
          $results[] = array(
            'field_name' => $field_name,
            'field_type' => $field_type,
            'nodeid' => $row['entity_id'],
            'title' => $node->title,
            'is_avp' => ($check_iframe >= 2)? 'AV PORTAL':'NO'
          );
        }
      }
      // Manage the summary field case
      if ($summary && trim($row["{$field_name}_summary"])){
        // Get rendered value of summary 
        $html = render(field_view_field('node', $node, $field_name, 'teaser'));
        if ($html) {
          $check_iframe = checksubsite_iframe($html);
           if ($check_iframe > 0) {
              $results[] = array(
                'field_name' => "$field_name (Summary)",
                'field_type' => $field_type,
                'nodeid' => $row['entity_id'],
                'title' => $node->title,
                'is_avp' => ($check_iframe >= 2)? 'AV PORTAL':'NO',
              );
           }
        }
      } 
    }
  }
  return $results;
}

/**
* Function that checks if there is an iframe tag and if there is an AVPortal iframe in a string.
* It will return 0, 1 (iframe), 2 (av-portal iframe), 3 AVportal Old way.
*/
function checksubsite_iframe($text) {
  $pattern = "/(?:<iframe[^>]*)/";
  if (preg_match($pattern, decode_entities($text))) {
    // Check if it's an AVPortal iframe
    $pattern = "/(?:<iframe[^>]*).(?:ec.europa.eu\/avservices\/play.cfm)/";
    if (preg_match($pattern, decode_entities($text)) ) {
      return 2;
    }
    // check if it's a classic AV portal
    $pattern = "/https:\/\/europa.eu/";
    if (preg_match($pattern, decode_entities($text)) ) {
      return 3;
    }
    return 1;
  }
  return 0;
}
