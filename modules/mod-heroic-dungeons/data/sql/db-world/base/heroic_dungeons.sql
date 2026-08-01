DELETE FROM `areatrigger_scripts` WHERE `entry` IN (2216, 2217);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(2216, 'heroic_stratholme_service_entrance'),
(2217, 'heroic_stratholme_service_entrance');

-- Stratholme predates Heroic dungeon mode, so its original spawns only carry
-- the Normal difficulty bit. Preserve that bit and add the Heroic bit.
UPDATE `creature` SET `spawnMask` = `spawnMask` | 2 WHERE `map` = 329;
UPDATE `gameobject` SET `spawnMask` = `spawnMask` | 2 WHERE `map` = 329;
