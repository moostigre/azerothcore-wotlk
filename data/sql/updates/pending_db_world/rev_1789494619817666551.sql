-- Alexei-only weights, normalized from 18,368 recorded drops of these 27 items (2026-09-15).
-- https://www.wowhead.com/wotlk/npc=10504/lord-alexei-barov#drops
-- Hammer of the Vesper takes the remainder, avoiding an empty roll from rounding.
DELETE FROM `reference_loot_template` WHERE `Entry` = 35095;
INSERT INTO `reference_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(35095, 14611, 0, 7.883275, 0, 1, 1, 1, 1, 'Bloodmail Hauberk'),
(35095, 14612, 0, 0.604312, 0, 1, 1, 1, 1, 'Bloodmail Legguards'),
(35095, 14614, 0, 0.647866, 0, 1, 1, 1, 1, 'Bloodmail Belt'),
(35095, 14615, 0, 0.751307, 0, 1, 1, 1, 1, 'Bloodmail Gauntlets'),
(35095, 14616, 0, 0.729530, 0, 1, 1, 1, 1, 'Bloodmail Boots'),
(35095, 14620, 0, 0.865636, 0, 1, 1, 1, 1, 'Deathbone Girdle'),
(35095, 14621, 0, 0.707753, 0, 1, 1, 1, 1, 'Deathbone Sabatons'),
(35095, 14622, 0, 0.854747, 0, 1, 1, 1, 1, 'Deathbone Gauntlets'),
(35095, 14623, 0, 0.854747, 0, 1, 1, 1, 1, 'Deathbone Legguards'),
(35095, 14624, 0, 8.182709, 0, 1, 1, 1, 1, 'Deathbone Chestplate'),
(35095, 14626, 0, 8.939460, 0, 1, 1, 1, 1, 'Necropile Robe'),
(35095, 14629, 0, 1.137848, 0, 1, 1, 1, 1, 'Necropile Cuffs'),
(35095, 14631, 0, 1.246733, 0, 1, 1, 1, 1, 'Necropile Boots'),
(35095, 14632, 0, 1.235845, 0, 1, 1, 1, 1, 'Necropile Leggings'),
(35095, 14633, 0, 1.126960, 0, 1, 1, 1, 1, 'Necropile Mantle'),
(35095, 14636, 0, 1.224956, 0, 1, 1, 1, 1, 'Cadaverous Belt'),
(35095, 14637, 0, 8.694469, 0, 1, 1, 1, 1, 'Cadaverous Armor'),
(35095, 14638, 0, 1.273955, 0, 1, 1, 1, 1, 'Cadaverous Leggings'),
(35095, 14640, 0, 1.121516, 0, 1, 1, 1, 1, 'Cadaverous Gloves'),
(35095, 14641, 0, 1.290287, 0, 1, 1, 1, 1, 'Cadaverous Walkers'),
(35095, 18680, 0, 8.759800, 0, 1, 1, 1, 1, 'Ancient Bone Bow'),
(35095, 18681, 0, 8.465810, 0, 1, 1, 1, 1, 'Burial Shawl'),
(35095, 18682, 0, 8.601916, 0, 1, 1, 1, 1, 'Ghoul Skin Leggings'),
(35095, 18683, 0, 0, 0, 1, 1, 1, 1, 'Hammer of the Vesper'),
(35095, 18684, 0, 8.667247, 0, 1, 1, 1, 1, 'Dimly Opalescent Ring'),
(35095, 23200, 0, 3.168554, 0, 1, 1, 1, 1, 'Totem of Sustaining'),
(35095, 23201, 0, 3.936193, 0, 1, 1, 1, 1, 'Libram of Divinity');

-- Keep Lightforge Bracers at 5%; this reference remains the group's 95% fallback.
UPDATE `creature_loot_template` SET `Item` = 35095, `Reference` = 35095, `Chance` = 0 WHERE `Entry` = 10504 AND `Item` = 35031 AND `Reference` = 35031 AND `GroupId` = 1;
