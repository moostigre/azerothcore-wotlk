-- Shartuul's Transporter scripted event support.

DELETE FROM `spell_script_names`
WHERE `spell_id` IN (40309, 40220, 40222, 40560, 40561, 40563, 40565, 40493, 40725, 40221)
  AND `ScriptName` IN (
    'spell_item_crystalforged_darkrune',
    'spell_shartuul_smash_shield',
    'spell_shartuul_doomguard_punishing_blow',
    'spell_shartuul_doomguard_fel_flames',
    'spell_shartuul_doomguard_throw_axe',
    'spell_shartuul_doomguard_consume_essence',
    'spell_shartuul_doomguard_super_jump'
  );

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(40309, 'spell_item_crystalforged_darkrune'),
(40222, 'spell_shartuul_smash_shield'),
(40560, 'spell_shartuul_doomguard_punishing_blow'),
(40561, 'spell_shartuul_doomguard_fel_flames'),
(40563, 'spell_shartuul_doomguard_throw_axe'),
(40565, 'spell_shartuul_doomguard_consume_essence'),
(40493, 'spell_shartuul_doomguard_super_jump');

UPDATE `creature_template`
SET `ScriptName` = CASE `entry`
    WHEN 23055 THEN 'npc_shartuul_felguard_degrader'
    WHEN 23059 THEN 'npc_shartuul_event_controller'
    WHEN 23113 THEN 'npc_shartuul_doomguard_punisher'
    WHEN 23220 THEN 'npc_shartuul_shivan_assassin'
    WHEN 23212 THEN 'npc_shartuul_moarg_tormenter'
    WHEN 6010 THEN 'npc_shartuul_wave_felhound'
    WHEN 21135 THEN 'npc_shartuul_wave_imp'
    WHEN 19398 THEN 'npc_shartuul_ganarg_underling'
    WHEN 23278 THEN 'npc_shartuul_portable_fel_cannon'
END
WHERE `entry` IN (23055, 23059, 23113, 23220, 23212, 6010, 21135, 19398, 23278);

UPDATE `creature_template`
SET `minlevel` = 72,
    `maxlevel` = 72,
    `faction` = 1829,
    `unit_flags` = 0,
    `dynamicflags` = 0,
    `flags_extra` = 0,
    `HealthModifier` = 13,
    `DamageModifier` = 1,
    `AIName` = '',
    `ScriptName` = 'npc_shartuul_moarg_tormenter'
WHERE `entry` = 23212;

DELETE FROM `creature_template_spell`
WHERE `CreatureID` IN (23055, 23113);

INSERT INTO `creature_template_spell` (`CreatureID`, `Index`, `Spell`, `VerifiedBuild`) VALUES
(23055, 0, 40220, 12340),
(23055, 1, 40219, 12340),
(23055, 2, 40221, 12340),
(23055, 3, 40497, 12340),
(23055, 4, 40222, 12340),
(23113, 0, 40560, 12340),
(23113, 1, 40561, 12340),
(23113, 2, 40563, 12340),
(23113, 3, 40565, 12340),
(23113, 4, 40493, 12340);

DELETE FROM `creature_template_model`
WHERE `CreatureID` IN (23199, 23278, 23212);

INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`) VALUES
(23199, 0, 18288, 1, 1, 12340),
(23212, 0, 19899, 0.5, 1, 12340),
(23278, 0, 18820, 0.5, 1, 12340);

DELETE FROM `creature`
WHERE `guid` = 5300699;

INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curhealth`, `curmana`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`, `ScriptName`, `VerifiedBuild`, `CreateObject`, `Comment`) VALUES
(5300699, 23059, 1, 331, 2457, 1, 1, 0, 3994.9358, -3428.0635, 533.1404, 4.510522, 300, 0, 0, 2488, 0, 0, 0, 0, 0, '', 0, 0, 'Shartuul debug visual test controller');
