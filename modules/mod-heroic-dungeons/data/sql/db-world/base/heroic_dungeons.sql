-- Stratholme predates Heroic dungeon mode, so its original spawns only carry
-- the Normal difficulty bit. Preserve that bit and add the Heroic bit.
UPDATE `creature` SET `spawnMask` = `spawnMask` | 2 WHERE `map` = 329;
UPDATE `gameobject` SET `spawnMask` = `spawnMask` | 2 WHERE `map` = 329;
