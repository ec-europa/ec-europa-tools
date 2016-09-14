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
   *
   * @return int
   *    Total number of nodes processed.
   */
  public function processContentTranslationNodes() {
    $total = 0;
    $rows = $this->getLatestContent();
    foreach ($rows as $row) {
      if ($this->isContentTranslationEnabled($row->type) && $this->hasTranslatedRevision($row->nid, $row->type)) {
        $this->processNode($row);
        $total++;
      }
    }
    return $total;
  }

  /**
   * Process nodes with translation disabled.
   *
   * @return int
   *    Total number of nodes processed.
   */
  public function processTranslationDisabledNodes() {
    $total = 0;
    $rows = $this->getLatestContent();
    foreach ($rows as $row) {
      if ($this->isTranslationDisabled($row->type)) {
        $this->processNode($row);
        $total++;
      }
    }
    return $total;
  }

  /**
   * Process given node.
   *
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
    node_save($node);
    db_query("DELETE FROM {field_data_body} WHERE entity_type = 'node' AND entity_id = '{$revision['entity_id']}' AND language != 'und'")->execute();
    echo '.';
  }

}

echo 'Setting body field translation flag to 0...' . PHP_EOL;
db_query("UPDATE field_config SET translatable = '0' WHERE field_name = 'body'")->execute();
echo 'Done.' . PHP_EOL;

echo 'Clearing cache...' . PHP_EOL;
cache_clear_all();
echo 'Done.' . PHP_EOL;

// Process nodes.
echo 'Processing nodes...' . PHP_EOL;
$processor = new ContentTranslationFixProcessor();
$content_translation = $processor->processContentTranslationNodes();
$disabled_translation = $processor->processTranslationDisabledNodes();
echo PHP_EOL . 'Done.' . PHP_EOL;
echo 'Nodes with translation: ' . $content_translation . PHP_EOL;
echo 'Nodes with translation disabled: ' . $disabled_translation . PHP_EOL;
echo 'Total: ' . ($content_translation + $disabled_translation) . PHP_EOL;

echo 'Clearing cache...' . PHP_EOL;
cache_clear_all();
echo 'Done.' . PHP_EOL;
