CREATE TABLE IF NOT EXISTS `attriboost_attributes` (
  `guid` int unsigned NOT NULL,
  `unallocated` int DEFAULT NULL,
  `stamina` int unsigned DEFAULT NULL,
  `strength` int unsigned DEFAULT NULL,
  `agility` int unsigned DEFAULT NULL,
  `intellect` int unsigned DEFAULT NULL,
  `spirit` int unsigned DEFAULT NULL,
  `settings` int DEFAULT NULL,
  `comment` varchar(50) COLLATE utf8mb4_general_ci DEFAULT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;