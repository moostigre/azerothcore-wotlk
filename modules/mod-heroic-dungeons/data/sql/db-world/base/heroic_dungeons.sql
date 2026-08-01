DELETE FROM `areatrigger_scripts` WHERE `entry` IN (2216, 2217);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(2216, 'heroic_stratholme_service_entrance'),
(2217, 'heroic_stratholme_service_entrance');

DELETE FROM `creature_loot_template` WHERE `Entry` = 10436 AND `Item` = 11726 AND `LootMode` = 2;
INSERT INTO `creature_loot_template`
(`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(10436, 11726, 0, 100, 0, 2, 0, 1, 1, 'Heroic Baroness Anastari - Savage Gladiator Chain');

-- Stratholme predates Heroic dungeon mode, so its original spawns only carry
-- the Normal difficulty bit. Preserve that bit and add the Heroic bit.
UPDATE `creature` SET `spawnMask` = `spawnMask` | 2 WHERE `map` = 329;
UPDATE `gameobject` SET `spawnMask` = `spawnMask` | 2 WHERE `map` = 329;
