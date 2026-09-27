-- Notes belong to the account at creation. Character names are historical snapshots.
CREATE TABLE IF NOT EXISTS `account_history` (
  `id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `account_id` int unsigned NOT NULL,
  `realm_id` int unsigned NOT NULL,
  `character_guid` int unsigned NOT NULL DEFAULT 0,
  `character_name` varchar(12) NOT NULL DEFAULT '',
  `author_account` int unsigned NOT NULL DEFAULT 0,
  `author_guid` int unsigned NOT NULL DEFAULT 0,
  `author_name` varchar(12) NOT NULL,
  `created_at` bigint unsigned NOT NULL,
  `note` varchar(255) NOT NULL,
  `removed_at` bigint unsigned DEFAULT NULL,
  `remover_account` int unsigned NOT NULL DEFAULT 0,
  `remover_realm` int unsigned NOT NULL DEFAULT 0,
  `remover_guid` int unsigned NOT NULL DEFAULT 0,
  `remover_name` varchar(12) NOT NULL DEFAULT '',
  `removal_reason` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  KEY `idx_account_history` (`account_id`, `id`),
  CONSTRAINT `fk_account_history_account` FOREIGN KEY (`account_id`) REFERENCES `account` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELETE FROM `rbac_permissions` WHERE `id` IN (946, 947, 948);
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(946, 'Command: history list'),
(947, 'Command: history add'),
(948, 'Command: history remove');

DELETE FROM `rbac_linked_permissions` WHERE `linkedId` IN (946, 947, 948);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(197, 946),
(197, 947),
(197, 948);
