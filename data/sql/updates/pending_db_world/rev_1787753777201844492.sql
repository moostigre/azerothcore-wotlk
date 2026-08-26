--
-- Add the Firebrand pack missing from the tunnel between the Smolderthorn and Firebrand areas (issue #9805).
-- Source: 2.5.6.69110 sniff dump_2.5.6.69110_2026-08-26_11-50-34.pkt.
SET @CGUID := 5300679;

DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID AND @CGUID + 2 AND `id` IN (9259, 9261, 9262);
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curhealth`, `curmana`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`, `ScriptName`, `VerifiedBuild`, `CreateObject`, `Comment`) VALUES
(@CGUID + 0, 9259, 229, 0, 0, 1, 1, 1, 24.366007, -580.332825, -18.518145, 2.984513, 10800, 0, 0, 8097, 0, 0, 0, 0, 0, '', 69110, 1, NULL),
(@CGUID + 1, 9261, 229, 0, 0, 1, 1, 1, 19.092764, -581.621094, -18.518145, 0.541052, 10800, 0, 0, 6477, 2163, 0, 0, 0, 0, '', 69110, 1, NULL),
(@CGUID + 2, 9262, 229, 0, 0, 1, 1, 1, 20.906603, -577.482910, -18.518145, 4.817109, 10800, 0, 0, 6477, 3244, 0, 0, 0, 0, '', 69110, 1, NULL);
