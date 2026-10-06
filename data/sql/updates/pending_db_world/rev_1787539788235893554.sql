--
-- Scarlet Monastery Graveyard rare spawns.
-- One mother pool selects either the all-normal state (70%) or one of six rare-location states (5% each).
-- Reference: CMaNGOS commit 3cf1bdf7950bece923c6c2e49cc0cc2209ed4ec5 by anonxs (locations and 70/30 roll).
-- Reference: VMaNGOS world_full_14_june_2021 database (normal replacements and respawn behavior).
SET @CGUID := 5300679;
SET @POOL_MASTER := 201215;
SET @POOL_NORMAL := 201216;
SET @POOL_RARE_1 := 201217;
SET @POOL_RARE_2 := 201218;
SET @POOL_RARE_3 := 201219;
SET @POOL_RARE_4 := 201220;
SET @POOL_RARE_5 := 201221;
SET @POOL_RARE_6 := 201222;

-- Remove the three fixed rare spawns and the four existing creatures that occupy corrected rare locations.
DELETE FROM `creature_multispawn` WHERE `spawnId` IN (12164, 39882, 39888, 39894, 39895, 39908, 1975841) OR `spawnId` BETWEEN @CGUID AND @CGUID+41;
DELETE FROM `pool_creature` WHERE `pool_entry` BETWEEN @POOL_NORMAL AND @POOL_RARE_6 OR `guid` BETWEEN @CGUID AND @CGUID+41;
DELETE FROM `pool_pool` WHERE `pool_id` BETWEEN @POOL_NORMAL AND @POOL_RARE_6 OR `mother_pool` = @POOL_MASTER;
DELETE FROM `pool_template` WHERE `entry` BETWEEN @POOL_MASTER AND @POOL_RARE_6;
DELETE FROM `creature` WHERE `guid` IN (12164, 39882, 39888, 39894, 39895, 39908, 1975841) OR `guid` BETWEEN @CGUID AND @CGUID+41;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`, `VerifiedBuild`, `CreateObject`, `Comment`) VALUES
-- No rare: six normal placeholders, rerolled by the mother pool after a 15-minute respawn.
(@CGUID+0, 6426, 189, 0, 0, 1, 1, 0, 1749.60, 1309.90, 18.52500, 3.89208, 900, 3, 1, 0, 0, 'SM Graveyard rare placeholder 1 - normal state'),
(@CGUID+1, 4308, 189, 0, 0, 1, 1, 0, 1837.61, 1314.56, 19.09210, 1.13864, 900, 2, 1, 0, 0, 'SM Graveyard rare placeholder 2 - normal state'),
(@CGUID+2, 6427, 189, 0, 0, 1, 1, 0, 1848.62, 1340.43, 18.63100, 4.55218, 900, 5, 1, 0, 0, 'SM Graveyard rare placeholder 3 - normal state'),
(@CGUID+3, 6427, 189, 0, 0, 1, 1, 0, 1755.72, 1347.81, 19.49210, 5.04108, 900, 0, 0, 0, 0, 'SM Graveyard rare placeholder 4 - normal state'),
(@CGUID+4, 6426, 189, 0, 0, 1, 1, 0, 1744.47, 1405.73, 21.72250, 1.34533, 900, 3, 1, 0, 0, 'SM Graveyard rare placeholder 5 - normal state'),
(@CGUID+5, 6426, 189, 0, 0, 1, 1, 0, 1812.30, 1418.14, 8.49211, 5.07929, 900, 2, 1, 0, 0, 'SM Graveyard rare placeholder 6 - normal state'),
-- Rare at location 1. The long timers lock the selected state until the instance resets.
(@CGUID+6, 6488, 189, 0, 0, 1, 1, 1, 1749.60, 1309.90, 18.52500, 3.89208, 86400, 3, 1, 0, 0, 'SM Graveyard rare location 1'),
(@CGUID+7, 4308, 189, 0, 0, 1, 1, 0, 1837.61, 1314.56, 19.09210, 1.13864, 86400, 2, 1, 0, 0, 'SM Graveyard placeholder 2 - rare location 1 state'),
(@CGUID+8, 6427, 189, 0, 0, 1, 1, 0, 1848.62, 1340.43, 18.63100, 4.55218, 86400, 5, 1, 0, 0, 'SM Graveyard placeholder 3 - rare location 1 state'),
(@CGUID+9, 6427, 189, 0, 0, 1, 1, 0, 1755.72, 1347.81, 19.49210, 5.04108, 86400, 0, 0, 0, 0, 'SM Graveyard placeholder 4 - rare location 1 state'),
(@CGUID+10, 6426, 189, 0, 0, 1, 1, 0, 1744.47, 1405.73, 21.72250, 1.34533, 86400, 3, 1, 0, 0, 'SM Graveyard placeholder 5 - rare location 1 state'),
(@CGUID+11, 6426, 189, 0, 0, 1, 1, 0, 1812.30, 1418.14, 8.49211, 5.07929, 86400, 2, 1, 0, 0, 'SM Graveyard placeholder 6 - rare location 1 state'),
-- Rare at location 2.
(@CGUID+12, 6426, 189, 0, 0, 1, 1, 0, 1749.60, 1309.90, 18.52500, 3.89208, 86400, 3, 1, 0, 0, 'SM Graveyard placeholder 1 - rare location 2 state'),
(@CGUID+13, 6488, 189, 0, 0, 1, 1, 1, 1837.61, 1314.56, 19.09210, 1.13864, 86400, 2, 1, 0, 0, 'SM Graveyard rare location 2'),
(@CGUID+14, 6427, 189, 0, 0, 1, 1, 0, 1848.62, 1340.43, 18.63100, 4.55218, 86400, 5, 1, 0, 0, 'SM Graveyard placeholder 3 - rare location 2 state'),
(@CGUID+15, 6427, 189, 0, 0, 1, 1, 0, 1755.72, 1347.81, 19.49210, 5.04108, 86400, 0, 0, 0, 0, 'SM Graveyard placeholder 4 - rare location 2 state'),
(@CGUID+16, 6426, 189, 0, 0, 1, 1, 0, 1744.47, 1405.73, 21.72250, 1.34533, 86400, 3, 1, 0, 0, 'SM Graveyard placeholder 5 - rare location 2 state'),
(@CGUID+17, 6426, 189, 0, 0, 1, 1, 0, 1812.30, 1418.14, 8.49211, 5.07929, 86400, 2, 1, 0, 0, 'SM Graveyard placeholder 6 - rare location 2 state'),
-- Rare at location 3.
(@CGUID+18, 6426, 189, 0, 0, 1, 1, 0, 1749.60, 1309.90, 18.52500, 3.89208, 86400, 3, 1, 0, 0, 'SM Graveyard placeholder 1 - rare location 3 state'),
(@CGUID+19, 4308, 189, 0, 0, 1, 1, 0, 1837.61, 1314.56, 19.09210, 1.13864, 86400, 2, 1, 0, 0, 'SM Graveyard placeholder 2 - rare location 3 state'),
(@CGUID+20, 6488, 189, 0, 0, 1, 1, 1, 1848.62, 1340.43, 18.63100, 4.55218, 86400, 5, 1, 0, 0, 'SM Graveyard rare location 3'),
(@CGUID+21, 6427, 189, 0, 0, 1, 1, 0, 1755.72, 1347.81, 19.49210, 5.04108, 86400, 0, 0, 0, 0, 'SM Graveyard placeholder 4 - rare location 3 state'),
(@CGUID+22, 6426, 189, 0, 0, 1, 1, 0, 1744.47, 1405.73, 21.72250, 1.34533, 86400, 3, 1, 0, 0, 'SM Graveyard placeholder 5 - rare location 3 state'),
(@CGUID+23, 6426, 189, 0, 0, 1, 1, 0, 1812.30, 1418.14, 8.49211, 5.07929, 86400, 2, 1, 0, 0, 'SM Graveyard placeholder 6 - rare location 3 state'),
-- Rare at location 4.
(@CGUID+24, 6426, 189, 0, 0, 1, 1, 0, 1749.60, 1309.90, 18.52500, 3.89208, 86400, 3, 1, 0, 0, 'SM Graveyard placeholder 1 - rare location 4 state'),
(@CGUID+25, 4308, 189, 0, 0, 1, 1, 0, 1837.61, 1314.56, 19.09210, 1.13864, 86400, 2, 1, 0, 0, 'SM Graveyard placeholder 2 - rare location 4 state'),
(@CGUID+26, 6427, 189, 0, 0, 1, 1, 0, 1848.62, 1340.43, 18.63100, 4.55218, 86400, 5, 1, 0, 0, 'SM Graveyard placeholder 3 - rare location 4 state'),
(@CGUID+27, 6488, 189, 0, 0, 1, 1, 1, 1755.72, 1347.81, 19.49210, 5.04108, 86400, 0, 0, 0, 0, 'SM Graveyard rare location 4'),
(@CGUID+28, 6426, 189, 0, 0, 1, 1, 0, 1744.47, 1405.73, 21.72250, 1.34533, 86400, 3, 1, 0, 0, 'SM Graveyard placeholder 5 - rare location 4 state'),
(@CGUID+29, 6426, 189, 0, 0, 1, 1, 0, 1812.30, 1418.14, 8.49211, 5.07929, 86400, 2, 1, 0, 0, 'SM Graveyard placeholder 6 - rare location 4 state'),
-- Rare at location 5.
(@CGUID+30, 6426, 189, 0, 0, 1, 1, 0, 1749.60, 1309.90, 18.52500, 3.89208, 86400, 3, 1, 0, 0, 'SM Graveyard placeholder 1 - rare location 5 state'),
(@CGUID+31, 4308, 189, 0, 0, 1, 1, 0, 1837.61, 1314.56, 19.09210, 1.13864, 86400, 2, 1, 0, 0, 'SM Graveyard placeholder 2 - rare location 5 state'),
(@CGUID+32, 6427, 189, 0, 0, 1, 1, 0, 1848.62, 1340.43, 18.63100, 4.55218, 86400, 5, 1, 0, 0, 'SM Graveyard placeholder 3 - rare location 5 state'),
(@CGUID+33, 6427, 189, 0, 0, 1, 1, 0, 1755.72, 1347.81, 19.49210, 5.04108, 86400, 0, 0, 0, 0, 'SM Graveyard placeholder 4 - rare location 5 state'),
(@CGUID+34, 6488, 189, 0, 0, 1, 1, 1, 1744.47, 1405.73, 21.72250, 1.34533, 86400, 3, 1, 0, 0, 'SM Graveyard rare location 5'),
(@CGUID+35, 6426, 189, 0, 0, 1, 1, 0, 1812.30, 1418.14, 8.49211, 5.07929, 86400, 2, 1, 0, 0, 'SM Graveyard placeholder 6 - rare location 5 state'),
-- Rare at location 6.
(@CGUID+36, 6426, 189, 0, 0, 1, 1, 0, 1749.60, 1309.90, 18.52500, 3.89208, 86400, 3, 1, 0, 0, 'SM Graveyard placeholder 1 - rare location 6 state'),
(@CGUID+37, 4308, 189, 0, 0, 1, 1, 0, 1837.61, 1314.56, 19.09210, 1.13864, 86400, 2, 1, 0, 0, 'SM Graveyard placeholder 2 - rare location 6 state'),
(@CGUID+38, 6427, 189, 0, 0, 1, 1, 0, 1848.62, 1340.43, 18.63100, 4.55218, 86400, 5, 1, 0, 0, 'SM Graveyard placeholder 3 - rare location 6 state'),
(@CGUID+39, 6427, 189, 0, 0, 1, 1, 0, 1755.72, 1347.81, 19.49210, 5.04108, 86400, 0, 0, 0, 0, 'SM Graveyard placeholder 4 - rare location 6 state'),
(@CGUID+40, 6426, 189, 0, 0, 1, 1, 0, 1744.47, 1405.73, 21.72250, 1.34533, 86400, 3, 1, 0, 0, 'SM Graveyard placeholder 5 - rare location 6 state'),
(@CGUID+41, 6488, 189, 0, 0, 1, 1, 1, 1812.30, 1418.14, 8.49211, 5.07929, 86400, 2, 1, 0, 0, 'SM Graveyard rare location 6');

-- Every rare-position row chooses equally between Fallen Champion, Ironspine, and Azshir the Sleepless.
DELETE FROM `creature_multispawn` WHERE `spawnId` IN (@CGUID+6, @CGUID+13, @CGUID+20, @CGUID+27, @CGUID+34, @CGUID+41);
INSERT INTO `creature_multispawn` (`spawnId`, `entry`) VALUES
(@CGUID+6, 6489), (@CGUID+6, 6490),
(@CGUID+13, 6489), (@CGUID+13, 6490),
(@CGUID+20, 6489), (@CGUID+20, 6490),
(@CGUID+27, 6489), (@CGUID+27, 6490),
(@CGUID+34, 6489), (@CGUID+34, 6490),
(@CGUID+41, 6489), (@CGUID+41, 6490);

DELETE FROM `pool_template` WHERE `entry` BETWEEN @POOL_MASTER AND @POOL_RARE_6;
INSERT INTO `pool_template` (`entry`, `max_limit`, `description`) VALUES
(@POOL_MASTER, 1, 'Scarlet Monastery Graveyard rare state'),
(@POOL_NORMAL, 6, 'Scarlet Monastery Graveyard - no rare'),
(@POOL_RARE_1, 6, 'Scarlet Monastery Graveyard - rare at location 1'),
(@POOL_RARE_2, 6, 'Scarlet Monastery Graveyard - rare at location 2'),
(@POOL_RARE_3, 6, 'Scarlet Monastery Graveyard - rare at location 3'),
(@POOL_RARE_4, 6, 'Scarlet Monastery Graveyard - rare at location 4'),
(@POOL_RARE_5, 6, 'Scarlet Monastery Graveyard - rare at location 5'),
(@POOL_RARE_6, 6, 'Scarlet Monastery Graveyard - rare at location 6');

DELETE FROM `pool_creature` WHERE `pool_entry` BETWEEN @POOL_NORMAL AND @POOL_RARE_6 OR `guid` BETWEEN @CGUID AND @CGUID+41;
INSERT INTO `pool_creature` (`guid`, `pool_entry`, `chance`, `description`) VALUES
(@CGUID+0, @POOL_NORMAL, 0, 'SM Graveyard normal state - location 1'),
(@CGUID+1, @POOL_NORMAL, 0, 'SM Graveyard normal state - location 2'),
(@CGUID+2, @POOL_NORMAL, 0, 'SM Graveyard normal state - location 3'),
(@CGUID+3, @POOL_NORMAL, 0, 'SM Graveyard normal state - location 4'),
(@CGUID+4, @POOL_NORMAL, 0, 'SM Graveyard normal state - location 5'),
(@CGUID+5, @POOL_NORMAL, 0, 'SM Graveyard normal state - location 6'),
(@CGUID+6, @POOL_RARE_1, 0, 'SM Graveyard rare state 1 - location 1 rare'),
(@CGUID+7, @POOL_RARE_1, 0, 'SM Graveyard rare state 1 - location 2 normal'),
(@CGUID+8, @POOL_RARE_1, 0, 'SM Graveyard rare state 1 - location 3 normal'),
(@CGUID+9, @POOL_RARE_1, 0, 'SM Graveyard rare state 1 - location 4 normal'),
(@CGUID+10, @POOL_RARE_1, 0, 'SM Graveyard rare state 1 - location 5 normal'),
(@CGUID+11, @POOL_RARE_1, 0, 'SM Graveyard rare state 1 - location 6 normal'),
(@CGUID+12, @POOL_RARE_2, 0, 'SM Graveyard rare state 2 - location 1 normal'),
(@CGUID+13, @POOL_RARE_2, 0, 'SM Graveyard rare state 2 - location 2 rare'),
(@CGUID+14, @POOL_RARE_2, 0, 'SM Graveyard rare state 2 - location 3 normal'),
(@CGUID+15, @POOL_RARE_2, 0, 'SM Graveyard rare state 2 - location 4 normal'),
(@CGUID+16, @POOL_RARE_2, 0, 'SM Graveyard rare state 2 - location 5 normal'),
(@CGUID+17, @POOL_RARE_2, 0, 'SM Graveyard rare state 2 - location 6 normal'),
(@CGUID+18, @POOL_RARE_3, 0, 'SM Graveyard rare state 3 - location 1 normal'),
(@CGUID+19, @POOL_RARE_3, 0, 'SM Graveyard rare state 3 - location 2 normal'),
(@CGUID+20, @POOL_RARE_3, 0, 'SM Graveyard rare state 3 - location 3 rare'),
(@CGUID+21, @POOL_RARE_3, 0, 'SM Graveyard rare state 3 - location 4 normal'),
(@CGUID+22, @POOL_RARE_3, 0, 'SM Graveyard rare state 3 - location 5 normal'),
(@CGUID+23, @POOL_RARE_3, 0, 'SM Graveyard rare state 3 - location 6 normal'),
(@CGUID+24, @POOL_RARE_4, 0, 'SM Graveyard rare state 4 - location 1 normal'),
(@CGUID+25, @POOL_RARE_4, 0, 'SM Graveyard rare state 4 - location 2 normal'),
(@CGUID+26, @POOL_RARE_4, 0, 'SM Graveyard rare state 4 - location 3 normal'),
(@CGUID+27, @POOL_RARE_4, 0, 'SM Graveyard rare state 4 - location 4 rare'),
(@CGUID+28, @POOL_RARE_4, 0, 'SM Graveyard rare state 4 - location 5 normal'),
(@CGUID+29, @POOL_RARE_4, 0, 'SM Graveyard rare state 4 - location 6 normal'),
(@CGUID+30, @POOL_RARE_5, 0, 'SM Graveyard rare state 5 - location 1 normal'),
(@CGUID+31, @POOL_RARE_5, 0, 'SM Graveyard rare state 5 - location 2 normal'),
(@CGUID+32, @POOL_RARE_5, 0, 'SM Graveyard rare state 5 - location 3 normal'),
(@CGUID+33, @POOL_RARE_5, 0, 'SM Graveyard rare state 5 - location 4 normal'),
(@CGUID+34, @POOL_RARE_5, 0, 'SM Graveyard rare state 5 - location 5 rare'),
(@CGUID+35, @POOL_RARE_5, 0, 'SM Graveyard rare state 5 - location 6 normal'),
(@CGUID+36, @POOL_RARE_6, 0, 'SM Graveyard rare state 6 - location 1 normal'),
(@CGUID+37, @POOL_RARE_6, 0, 'SM Graveyard rare state 6 - location 2 normal'),
(@CGUID+38, @POOL_RARE_6, 0, 'SM Graveyard rare state 6 - location 3 normal'),
(@CGUID+39, @POOL_RARE_6, 0, 'SM Graveyard rare state 6 - location 4 normal'),
(@CGUID+40, @POOL_RARE_6, 0, 'SM Graveyard rare state 6 - location 5 normal'),
(@CGUID+41, @POOL_RARE_6, 0, 'SM Graveyard rare state 6 - location 6 rare');

DELETE FROM `pool_pool` WHERE `pool_id` BETWEEN @POOL_NORMAL AND @POOL_RARE_6 OR `mother_pool` = @POOL_MASTER;
INSERT INTO `pool_pool` (`pool_id`, `mother_pool`, `chance`, `description`) VALUES
(@POOL_NORMAL, @POOL_MASTER, 70, 'SM Graveyard - no rare'),
(@POOL_RARE_1, @POOL_MASTER, 5, 'SM Graveyard - rare at location 1'),
(@POOL_RARE_2, @POOL_MASTER, 5, 'SM Graveyard - rare at location 2'),
(@POOL_RARE_3, @POOL_MASTER, 5, 'SM Graveyard - rare at location 3'),
(@POOL_RARE_4, @POOL_MASTER, 5, 'SM Graveyard - rare at location 4'),
(@POOL_RARE_5, @POOL_MASTER, 5, 'SM Graveyard - rare at location 5'),
(@POOL_RARE_6, @POOL_MASTER, 5, 'SM Graveyard - rare at location 6');

-- Keep the combat behavior of all three rares, but remove their independent 90% AI-init despawn rolls.
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryorguid` = 6488;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param6`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(6488, 0, 0, 0, 0, 0, 100, 0, 5000, 8000, 6000, 14000, 0, 0, 11, 19642, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 'Fallen Champion - In Combat - Cast Cleave'),
(6488, 0, 1, 0, 0, 0, 100, 0, 2000, 4000, 6000, 8000, 0, 0, 11, 19644, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 'Fallen Champion - In Combat - Cast Strike'),
(6488, 0, 2, 0, 0, 0, 100, 0, 10000, 10000, 30000, 30000, 0, 0, 11, 21949, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 'Fallen Champion - In Combat - Cast Rend');

DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryorguid` = 6489;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param6`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(6489, 0, 0, 0, 0, 0, 100, 0, 5000, 8000, 30000, 30000, 0, 0, 11, 702, 0, 0, 0, 0, 0, 5, 30, 0, 0, 0, 0, 0, 0, 0, 'Ironspine - In Combat - Cast Curse of Weakness'),
(6489, 0, 1, 0, 0, 0, 100, 0, 2000, 3000, 25000, 25000, 0, 0, 11, 3815, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'Ironspine - In Combat - Cast Poison Cloud');

DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryorguid` = 6490;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param6`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(6490, 0, 0, 0, 0, 0, 100, 0, 7000, 11000, 70000, 70000, 0, 0, 11, 5137, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 'Azshir the Sleepless - In Combat - Cast Call of the Grave'),
(6490, 0, 1, 0, 0, 0, 100, 0, 6000, 8000, 20000, 20000, 0, 0, 11, 7399, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'Azshir the Sleepless - In Combat - Cast Terrify'),
(6490, 0, 2, 0, 0, 0, 100, 0, 14000, 14000, 20000, 20000, 0, 0, 11, 9373, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 'Azshir the Sleepless - In Combat - Cast Soul Siphon');
