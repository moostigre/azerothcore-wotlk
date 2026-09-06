UPDATE `gameobject_template` SET `AIName`='' WHERE `entry` IN (176304,176307,176308,176309);
-- Lock.dbc 43 is the standard Open lock and requires no key or profession skill.
UPDATE `gameobject_template` SET `Data0`=43 WHERE `entry` IN (176304,176307,176308,176309);
UPDATE `gameobject_template` SET `AIName`='' WHERE `entry` IN (175534,175535,175536,175537);
UPDATE `gameobject_template_addon` SET `faction`=168 WHERE `entry` IN (175534,175535,175536,175537);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (175534,175535,175536,175537,176304,176307,176308,176309) AND `source_type`=1;
