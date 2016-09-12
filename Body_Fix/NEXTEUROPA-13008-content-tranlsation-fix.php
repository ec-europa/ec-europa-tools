<?php

/**
 * Class ContentTranslationFixProcessor.
 */
class ContentTranslationFixProcessor {

  /**
   * Date of nexteuropa_core_7003() update hook deployment.
   */
  const DEPLOYMENT_DATE = 1472428860;

  /**
   * Content translation mode ID.
   */
  const TRANSLATION_CONTENT_TRANSLATION = 2;

  /**
   * Translation disabled mode ID.
   */
  const TRANSLATION_DISABLED = 0;

  /**
   * Get content type list of content that has been created from deployment date.
   *
   * @return array
   *    Array of content type machine names.
   */
  public function getContentTypes() {
    $query = db_select('field_data_body', 'f');
    $query->fields('f', ['bundle']);
    $query->condition('f.entity_type', 'node');
    $query->groupBy('f.bundle');
    return $query->execute()->fetchCol();
  }

  /**
   * Check whereas content translation is enabled for given content type.
   *
   * @param string $type
   *    Content type machine name.
   *
   * @return bool
   *    TRUE content translation enable, FALSE otherwise.
   */
  public function isContentTranslationEnabled($type) {
    return variable_get("language_content_type_{$type}", 0) == self::TRANSLATION_CONTENT_TRANSLATION;
  }

  /**
   * Check whereas translation is disabled for given content type.
   *
   * @param string $type
   *    Content type machine name.
   *
   * @return bool
   *    TRUE content translation enable, FALSE otherwise.
   */
  public function isTranslationDisabled($type) {
    return variable_get("language_content_type_{$type}", 0) == self::TRANSLATION_DISABLED;
  }

  /**
   * Get content changed after the deployment date.
   *
   * @return object[]
   *   Object containing an array of NID and node type.
   */
  public function getLatestContent() {
    $query = db_select('node', 'n')
      ->fields('n', ['nid', 'type'])
      ->condition('n.changed', self::DEPLOYMENT_DATE, '>=');
    return $query->execute()->fetchAll();
  }

  /**
   * Get latest revision with language different than LANGUAGE_NONE.
   *
   * @param int $nid
   *    Node ID.
   * @param string $type
   *    Node type.
   *
   * @return array|NULL
   *    Latest revision entry or NULL if none found.
   */
  public function getLatestTranslatedRevision($nid, $type) {
    $query = db_select('field_revision_body', 'f');
    $query->fields('f');
    $query->condition('f.entity_type', 'node');
    $query->condition('f.entity_id', $nid);
    $query->condition('f.bundle', $type);
    $query->condition('f.language', LANGUAGE_NONE, '!=');
    $query->orderBy('f.revision_id', 'DESC');
    $query->range(0, 1);
    return $query->execute()->fetchAssoc();
  }

  /**
   * Get latest revision.
   *
   * @param int $nid
   *    Node ID.
   * @param string $type
   *    Node type.
   *
   * @return array|NULL
   *    Latest revision entry or NULL if none found.
   */
  public function getLatestNotTranslatedRevision($nid, $type) {
    $query = db_select('field_revision_body', 'f');
    $query->fields('f');
    $query->condition('f.entity_type', 'node');
    $query->condition('f.entity_id', $nid);
    $query->condition('f.bundle', $type);
    $query->condition('f.language', LANGUAGE_NONE);
    $query->orderBy('f.revision_id', 'DESC');
    $query->range(0, 1);
    return $query->execute()->fetchAssoc();
  }

  /**
   * Check whereas node latest revision is translated.
   *
   * @param int $nid
   *    Node ID.
   * @param string $type
   *    Node type.
   *
   * @return bool
   *    Latest revision entry or NULL if none found.
   */
  public function hasTranslatedRevision($nid, $type) {
    $revision1 = $this->getLatestTranslatedRevision($nid, $type);
    $revision2 = $this->getLatestNotTranslatedRevision($nid, $type);
    if (!$revision1) {
      return FALSE;
    }
    if ($revision2['revision_id'] > $revision1['revision_id']) {
      return FALSE;
    }
    return TRUE;
  }

  /**
   * Process nodes with content translation enabled.
   */
  public function processContentTranslationNodes() {
    $rows = $this->getLatestContent();
    foreach ($rows as $row) {
      if ($this->isContentTranslationEnabled($row->type) && $this->hasTranslatedRevision($row->nid, $row->type)) {
        $this->processNode($row);
      }
    }
  }

  /**
   * Process nodes with translation disabled.
   */
  public function processTranslationDisabledNodes() {
    $rows = $this->getLatestContent();
    foreach ($rows as $row) {
      if ($this->isTranslationDisabled($row->type)) {
        $this->processNode($row);
      }
    }
  }

  /**
   * @param $node
   */
  public function processNode($node) {
    $revision = $this->getLatestTranslatedRevision($node->nid, $node->type);
    $node = node_load($node->nid);
    $node->body = [
      LANGUAGE_NONE => [
        [
          'format' => $revision['body_format'],
          'summary' => $revision['body_summary'],
          'value' => $revision['body_value'],
        ],
      ],
    ];
    $node->revision = TRUE;
    db_query("DELETE FROM {field_revision_body} WHERE entity_type = 'node' AND entity_id = '{$revision['entity_id']}' AND revision_id = '{$revision['revision_id']}' AND language = '{$revision['language']}'")->execute();
    db_query("DELETE FROM {field_data_body} WHERE entity_type = 'node' AND entity_id = '{$revision['entity_id']}' AND revision_id = '{$revision['revision_id']}' AND language = '{$revision['language']}'")->execute();
    node_save($node);
    echo '.';
  }

}

// Process nodes.
$processor = new ContentTranslationFixProcessor();
$processor->processContentTranslationNodes();
$processor->processTranslationDisabledNodes();
echo PHP_EOL;
