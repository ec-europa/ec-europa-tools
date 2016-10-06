<?php
/**
* @file
* Drush commands file. Provides commands for analysing features uses.
*
* Related to NEPT-89
*/

/**
* Implements hook_drush_command().
*/
function feature_audit_drush_command() {
  $items = array();

  $items['check-wysiwyg-text-formats'] = array(
    'description' => 'List text formats using WYSIWYG profiles and gives some information on main activated CKEDITOR options like LITE.',
    'aliases' => array('facwf'),
    'outputformat' => array(
      'default' => 'table',
      'pipe-format' => 'csv',
      'field-labels' => array(
        'text-format' => 'Text format',
        'wysiwyg-editor' => 'WYSIWYG Editor',
        'ckeditor-lite' => 'Change tracking activated ?',
        'has_tracked_changes' => 'Content and fields tracked changes ?'
      ),
      'output-data-type' => 'format-table',
    ),
  );

  return $items;
}

/**
 * Drush command callback.
 */
function drush_feature_audit_check_wysiwyg_text_formats() {
  $results = array();
  // 1.- Gets activates text formats with WYSIWYG profiles
  $text_formats = filter_formats();
  $field_to_check = _feature_audit_get_type_fields_list(['text_long', 'text_with_summary', 'text']);
  foreach ($text_formats as $format_id => $format) {
    if (module_exists('wysiwyg')) {
      module_load_include('module', 'wysiwyg', 'wysiwyg');
      $profile = wysiwyg_profile_load($format_id);
      if ($profile) {
        if ($profile->editor == 'ckeditor') {
          // 2.- Check information about CKEDITOR LITE in each WYSIWYG profiles.
          if (isset($profile->settings['buttons']['lite'])) {
            // 3.- Check content type fields using which text formats and see if tracked changes are stored for this fields.
            $has_tracked_changes = array();
            $field_names = array_keys($field_to_check);
            foreach ($field_names as $field_name) {
               $query = db_select('field_revision_' . $field_name, 'fr');
               $query->condition($field_name . '_format', $format_id)
                     ->where($field_name . '_value REGEXP(:regex)', array(':regex' => '/<span[^>]+class="[^"]*ice-[^>]+>/'));
               $query->fields('fr', array('entity_type', 'bundle'));
               $query->distinct(TRUE);
               $query->orderBy('entity_type')->orderBy('bundle');
               $query_results = $query->execute()->fetchAllAssoc('entity_type');
               if ($query_results) {
                 $value = array();
                 foreach ($query_results as $query_result) {
                   $value[] = $query_result->bundle . '(' . $query_result->entity_type . ')';
                 }
                 $has_tracked_changes[] =  $field_name . ' in ' . implode(', ', $value);
               }
            }

            $results[$format_id] = array(
              'text-format' => $format->name,
              'wysiwyg-editor' => $profile->editor,
              'ckeditor-lite' => 'Yes',
              'has_tracked_changes' => ($has_tracked_changes) ? implode('; ', $has_tracked_changes): 'None',
            );
          }
          else {
            $results[$format_id] = array(
              'text-format' => $format->name,
              'wysiwyg-editor' => $profile->editor,
              'ckeditor-lite' => 'No',
              'has_tracked_changes' => 'None'
            );
          }
        }
        else {
          $results[$format_id] = array(
            'text-format' => $format->name,
            'wysiwyg-editor' => $profile->editor,
            'ckeditor-lite' => 'No',
            'has_tracked_changes' => 'None'
          );
        }
      }
      else {
        $results[$format_id] = array(
          'text-format' => $format->name,
          'wysiwyg-editor' => 'None',
          'ckeditor-lite' => 'No',
          'has_tracked_changes' => 'None'
        );
      }
    }
    else {
      // If module does not exist, we can bypass the full process and
      // return a light table.
      $results[$format_id] = array(
        'text-format' => $format->name,
        'wysiwyg-editor' => 'None (WYSIWYG is not enabled)',
        'ckeditor-lite' => 'No',
        'has_tracked_changes' => 'None'
      );
    }
  }

  //4.- Format and return the results
  return $results;
}

function _feature_audit_get_type_fields_list($field_types = array()){
  $field_infos = field_info_fields();
  if ($field_types) {
    return array_filter($field_infos, function ($item) use ($field_types) {
      return in_array($item['type'], $field_types);
    });
  }

  return $field_infos;
}