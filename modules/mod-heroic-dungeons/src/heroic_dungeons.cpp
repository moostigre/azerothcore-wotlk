#include "heroic_dungeons.h"

#include "AllCreatureScript.h"
#include "AllSpellScript.h"
#include "Config.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include "Log.h"
#include "Map.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "Unit.h"
#include "UnitScript.h"
#include "WorldScript.h"

#include <fkYAML/node.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace HeroicDungeons
{
namespace
{
struct AbilityState
{
    std::vector<uint32> timers;
    bool wasInCombat = false;
};

Config config;
std::unordered_map<ObjectGuid, AbilityState> abilityStates;
std::unordered_set<uint64> authorizedCasts;
std::unordered_set<ObjectGuid> healthScaledCreatures;

template <typename T>
T ReadValue(fkyaml::node const& node, char const* key, T defaultValue)
{
    if (!node.is_mapping() || !node.contains(key))
        return defaultValue;
    return node[key].get_value<T>();
}

AbilityTarget ParseTarget(std::string const& target)
{
    if (target == "random_player")
        return AbilityTarget::RandomPlayer;
    if (target == "self")
        return AbilityTarget::Self;
    return AbilityTarget::Victim;
}

uint64 CastKey(Creature const* creature, uint32 spellId)
{
    return creature->GetGUID().GetRawValue() ^ (uint64(spellId) << 32);
}

uint32 RandomDelay(uint32 minimum, uint32 maximum)
{
    if (maximum < minimum)
        std::swap(minimum, maximum);
    return minimum == maximum ? minimum : urand(minimum, maximum);
}

void ApplyHealthScaling(Creature* creature)
{
    float baseHealth = creature->GetFlatModifierValue(UNIT_MOD_HEALTH, BASE_VALUE);
    if (baseHealth <= 0.0f)
        baseHealth = static_cast<float>(creature->GetMaxHealth());

    double scaledBaseHealth = std::max(1.0,
        std::round(static_cast<double>(baseHealth) * GetHealthMultiplier(creature)));
    creature->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE,
        static_cast<float>(std::min<double>(scaledBaseHealth, UINT32_MAX)));
    creature->UpdateMaxHealth();
    creature->SetCreateHealth(creature->GetMaxHealth());
    creature->SetHealth(creature->GetMaxHealth());
    healthScaledCreatures.insert(creature->GetGUID());
}

void ParseCreature(DungeonConfig& dungeon, fkyaml::node const& node)
{
    uint32 entry = ReadValue<uint32>(node, "entry", 0);
    if (!entry)
        throw std::runtime_error("A creature rule is missing a non-zero entry");

    CreatureModifier modifier;
    modifier.health = ReadValue<float>(node, "health", 0.0f);
    modifier.meleeDamage = ReadValue<float>(node, "melee_damage", 0.0f);
    if (modifier.health > 0.0f || modifier.meleeDamage > 0.0f)
        dungeon.creatureModifiers[entry] = modifier;

    if (node.contains("spells"))
    {
        for (fkyaml::node const& spellNode : node["spells"])
        {
            Ability ability;
            ability.creatureEntry = entry;
            ability.spellId = ReadValue<uint32>(spellNode, "id", 0);
            ability.damage = ReadValue<float>(spellNode, "damage", 0.0f);
            ability.initialMin = ReadValue<uint32>(spellNode, "initial_min_ms", 0);
            ability.initialMax = ReadValue<uint32>(spellNode, "initial_max_ms", ability.initialMin);
            ability.cooldownMin = ReadValue<uint32>(spellNode, "cooldown_min_ms", 1000);
            ability.cooldownMax = ReadValue<uint32>(spellNode, "cooldown_max_ms", ability.cooldownMin);
            ability.target = ParseTarget(ReadValue<std::string>(spellNode, "target", "victim"));
            ability.replaceOriginal = ReadValue<bool>(spellNode, "replace_original", false);
            if (!ability.spellId)
                throw std::runtime_error("A spell rule is missing a non-zero id");
            dungeon.abilities[entry].push_back(ability);
        }
    }

    if (!node.contains("loot"))
        return;

    fkyaml::node const& lootNode = node["loot"];
    LootRule rule;
    rule.mode = ReadValue<std::string>(lootNode, "mode", "add") == "replace" ?
        LootOverrideMode::Replace : LootOverrideMode::Add;
    if (lootNode.contains("items"))
        for (fkyaml::node const& itemNode : lootNode["items"])
        {
            LootItemRule item;
            item.itemId = ReadValue<uint32>(itemNode, "id", 0);
            item.chance = std::clamp(ReadValue<float>(itemNode, "chance", 0.0f), 0.0f, 100.0f);
            item.minCount = ReadValue<uint8>(itemNode, "min_count", 1);
            item.maxCount = ReadValue<uint8>(itemNode, "max_count", item.minCount);
            item.groupId = ReadValue<uint8>(itemNode, "group_id", 0);
            if (!item.itemId || !item.chance)
                throw std::runtime_error("A loot rule requires non-zero id and chance");
            rule.items.push_back(item);
        }
    dungeon.lootRules[entry] = std::move(rule);
}

void SynchronizeYamlDatabase()
{
    WorldDatabase.DirectExecute(
        "DELETE FROM `creature_loot_template` WHERE `Comment` LIKE 'Heroic YAML:%' OR "
        "`Comment` = 'Heroic Baroness Anastari - Savage Gladiator Chain'");

    for (auto const& [mapId, dungeon] : config.dungeons)
    {
        if (!dungeon.enabled)
            continue;

        WorldDatabase.DirectExecute("UPDATE `creature` SET `spawnMask` = `spawnMask` | 2 WHERE `map` = {}", mapId);
        WorldDatabase.DirectExecute("UPDATE `gameobject` SET `spawnMask` = `spawnMask` | 2 WHERE `map` = {}", mapId);

        for (auto const& [creatureEntry, loot] : dungeon.lootRules)
            for (LootItemRule const& item : loot.items)
                WorldDatabase.DirectExecute(
                    "INSERT INTO `creature_loot_template` "
                    "(`Entry`,`Item`,`Reference`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) "
                    "VALUES ({},{},0,{},0,2,{},{},{},'Heroic YAML: map {} creature {}')",
                    creatureEntry, item.itemId, item.chance, item.groupId, item.minCount, item.maxCount,
                    mapId, creatureEntry);
    }
}

Unit* SelectAbilityTarget(Creature* creature, AbilityTarget target)
{
    switch (target)
    {
        case AbilityTarget::Self:
            return creature;
        case AbilityTarget::RandomPlayer:
            return creature->AI()->SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true, false);
        case AbilityTarget::Victim:
        default:
            return creature->GetVictim();
    }
}
}

Config const& GetConfig()
{
    return config;
}

DungeonConfig const* GetDungeonConfig(uint32 mapId)
{
    auto itr = config.dungeons.find(mapId);
    return itr == config.dungeons.end() ? nullptr : &itr->second;
}

void LoadConfig()
{
    config = Config();
    config.enabled = sConfigMgr->GetOption<bool>("HeroicDungeons.Enable", true);
    config.yamlPath = sConfigMgr->GetOption<std::string>(
        "HeroicDungeons.YamlPath", "etc/modules/heroic_dungeons.yaml");

    if (!config.enabled)
        return;

    try
    {
        std::ifstream input(config.yamlPath);
        if (!input)
            throw std::runtime_error("Unable to open " + config.yamlPath);

        fkyaml::node root = fkyaml::node::deserialize(input);
        if (!root.contains("dungeons") || !root["dungeons"].is_sequence())
            throw std::runtime_error("The YAML root requires a dungeons sequence");

        for (fkyaml::node const& dungeonNode : root["dungeons"])
        {
            DungeonConfig dungeon;
            dungeon.name = ReadValue<std::string>(dungeonNode, "name", "unnamed");
            dungeon.mapId = ReadValue<uint32>(dungeonNode, "map", 0);
            dungeon.enabled = ReadValue<bool>(dungeonNode, "enabled", true);
            dungeon.resetSeconds = ReadValue<uint32>(dungeonNode, "reset_seconds", 86400);
            dungeon.serviceEntranceForcesHeroic =
                ReadValue<bool>(dungeonNode, "service_entrance_forces_heroic", false);
            if (!dungeon.mapId)
                throw std::runtime_error("Dungeon '" + dungeon.name + "' has an invalid map id");

            if (dungeonNode.contains("modifiers"))
            {
                fkyaml::node const& modifiers = dungeonNode["modifiers"];
                dungeon.health = std::max(0.01f, ReadValue<float>(modifiers, "health", 1.0f));
                dungeon.meleeDamage = std::max(0.0f, ReadValue<float>(modifiers, "melee_damage", 1.0f));
                dungeon.spellDamage = std::max(0.0f, ReadValue<float>(modifiers, "spell_damage", 1.0f));
            }

            if (dungeonNode.contains("creatures"))
                for (fkyaml::node const& creatureNode : dungeonNode["creatures"])
                    ParseCreature(dungeon, creatureNode);

            if (dungeonNode.contains("baron_rivendare"))
            {
                fkyaml::node const& boss = dungeonNode["baron_rivendare"];
                dungeon.baron.enabled = ReadValue<bool>(boss, "enabled", true);
                dungeon.baron.skeletonEntry = ReadValue<uint32>(boss, "skeleton_entry", 11197);
                dungeon.baron.skeletonCount = ReadValue<uint32>(boss, "skeleton_count", 6);
                dungeon.baron.raiseDeadSpell = ReadValue<uint32>(boss, "raise_dead_spell", 17473);
                dungeon.baron.enrageSpell = ReadValue<uint32>(boss, "enrage_spell", 8599);
            }

            if (dungeonNode.contains("baroness_anastari"))
            {
                fkyaml::node const& boss = dungeonNode["baroness_anastari"];
                dungeon.anastari.enabled = ReadValue<bool>(boss, "enabled", true);
                dungeon.anastari.bansheeEntry = ReadValue<uint32>(boss, "banshee_entry", 10464);
                dungeon.anastari.bansheeCount = ReadValue<uint32>(boss, "banshee_count", 3);
                dungeon.anastari.wailSpell = ReadValue<uint32>(boss, "wail_spell", 16565);
                dungeon.anastari.enrageSpell = ReadValue<uint32>(boss, "enrage_spell", 8599);
            }

            config.dungeons[dungeon.mapId] = std::move(dungeon);
        }

        SynchronizeYamlDatabase();
        LOG_INFO("module.heroic-dungeons", "Loaded {} heroic dungeon definitions from '{}'",
            config.dungeons.size(), config.yamlPath);
    }
    catch (std::exception const& exception)
    {
        config.dungeons.clear();
        LOG_ERROR("module.heroic-dungeons", "Failed to load heroic dungeon YAML '{}': {}",
            config.yamlPath, exception.what());
    }
}

bool IsEnabledFor(Creature const* creature)
{
    if (!creature || !config.enabled || !creature->GetMap() || !creature->GetMap()->IsHeroic())
        return false;
    DungeonConfig const* dungeon = GetDungeonConfig(creature->GetMapId());
    return dungeon && dungeon->enabled;
}

float GetHealthMultiplier(Creature const* creature)
{
    DungeonConfig const* dungeon = GetDungeonConfig(creature->GetMapId());
    auto itr = dungeon->creatureModifiers.find(creature->GetEntry());
    return itr != dungeon->creatureModifiers.end() && itr->second.health > 0.0f ?
        itr->second.health : dungeon->health;
}

float GetMeleeDamageMultiplier(Creature const* creature)
{
    DungeonConfig const* dungeon = GetDungeonConfig(creature->GetMapId());
    auto itr = dungeon->creatureModifiers.find(creature->GetEntry());
    return itr != dungeon->creatureModifiers.end() && itr->second.meleeDamage > 0.0f ?
        itr->second.meleeDamage : dungeon->meleeDamage;
}

float GetSpellDamageMultiplier(Creature const* creature, uint32 spellId)
{
    DungeonConfig const* dungeon = GetDungeonConfig(creature->GetMapId());
    auto itr = dungeon->abilities.find(creature->GetEntry());
    if (itr != dungeon->abilities.end())
        for (Ability const& ability : itr->second)
            if (ability.spellId == spellId && ability.damage > 0.0f)
                return ability.damage;
    return dungeon->spellDamage;
}

bool ShouldReplaceOriginalCast(Creature const* creature, uint32 spellId)
{
    if (!IsEnabledFor(creature))
        return false;

    DungeonConfig const* dungeon = GetDungeonConfig(creature->GetMapId());
    auto itr = dungeon->abilities.find(creature->GetEntry());
    if (itr != dungeon->abilities.end())
        for (Ability const& ability : itr->second)
            if (ability.spellId == spellId && ability.replaceOriginal)
                return true;
    return false;
}

bool IsAuthorizedHeroicCast(Spell const* spell)
{
    if (!spell || !spell->GetCaster() || !spell->GetCaster()->IsCreature())
        return false;
    Creature const* creature = spell->GetCaster()->ToCreature();
    return authorizedCasts.contains(CastKey(creature, spell->GetSpellInfo()->Id));
}

void UpdateAbilities(Creature* creature, uint32 diff)
{
    DungeonConfig const* dungeon = GetDungeonConfig(creature->GetMapId());
    auto rules = dungeon->abilities.find(creature->GetEntry());
    if (rules == dungeon->abilities.end())
        return;

    AbilityState& state = abilityStates[creature->GetGUID()];
    if (!creature->IsInCombat())
    {
        state.timers.clear();
        state.wasInCombat = false;
        return;
    }

    if (!state.wasInCombat || state.timers.size() != rules->second.size())
    {
        state.timers.clear();
        for (Ability const& ability : rules->second)
            state.timers.push_back(RandomDelay(ability.initialMin, ability.initialMax));
        state.wasInCombat = true;
    }

    if (creature->HasUnitState(UNIT_STATE_CASTING))
        return;

    for (std::size_t index = 0; index < rules->second.size(); ++index)
    {
        uint32& timer = state.timers[index];
        if (timer > diff)
        {
            timer -= diff;
            continue;
        }

        Ability const& ability = rules->second[index];
        if (Unit* target = SelectAbilityTarget(creature, ability.target))
        {
            uint64 key = CastKey(creature, ability.spellId);
            authorizedCasts.insert(key);
            creature->CastSpell(target, ability.spellId, false);
            authorizedCasts.erase(key);
            timer = RandomDelay(ability.cooldownMin, ability.cooldownMax);
            break;
        }

        timer = 1000;
    }
}

void ResetCreature(ObjectGuid guid)
{
    abilityStates.erase(guid);
}

class HeroicDungeonWorldScript final : public WorldScript
{
public:
    HeroicDungeonWorldScript() : WorldScript("HeroicDungeonWorldScript",
        { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        LoadConfig();
    }

    void OnBeforeWorldInitialized() override
    {
        if (!config.enabled)
            return;

        for (auto const& [mapId, dungeon] : config.dungeons)
            if (dungeon.enabled)
                sMapDifficultyMap[MAKE_PAIR32(mapId, DUNGEON_DIFFICULTY_HEROIC)] =
                    MapDifficulty(dungeon.resetSeconds, 5, false);
    }
};

class HeroicDungeonCreatureScript final : public AllCreatureScript
{
public:
    HeroicDungeonCreatureScript() : AllCreatureScript("HeroicDungeonCreatureScript") { }

    void OnCreatureSelectLevel(CreatureTemplate const* /*creatureTemplate*/, Creature* creature) override
    {
        if (!IsEnabledFor(creature))
            return;

        ApplyHealthScaling(creature);
    }

    void OnAllCreatureUpdate(Creature* creature, uint32 diff) override
    {
        if (IsEnabledFor(creature))
        {
            if (!healthScaledCreatures.contains(creature->GetGUID()))
                ApplyHealthScaling(creature);

            DungeonConfig const* dungeon = GetDungeonConfig(creature->GetMapId());
            auto loot = dungeon->lootRules.find(creature->GetEntry());
            if (loot != dungeon->lootRules.end())
            {
                if (loot->second.mode == LootOverrideMode::Replace)
                    creature->SetLootMode(2);
                else
                    creature->AddLootMode(2);
            }
            UpdateAbilities(creature, diff);
        }
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        healthScaledCreatures.erase(creature->GetGUID());
        ResetCreature(creature->GetGUID());
    }
};

class HeroicDungeonUnitScript final : public UnitScript
{
public:
    HeroicDungeonUnitScript() : UnitScript("HeroicDungeonUnitScript", true,
        { UNITHOOK_MODIFY_MELEE_DAMAGE, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN }) { }

    void ModifyMeleeDamage(Unit* /*target*/, Unit* attacker, uint32& damage) override
    {
        if (attacker && attacker->IsCreature() && IsEnabledFor(attacker->ToCreature()))
            damage = static_cast<uint32>(std::llround(damage * GetMeleeDamageMultiplier(attacker->ToCreature())));
    }

    void ModifySpellDamageTaken(Unit* /*target*/, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
    {
        if (damage > 0 && spellInfo && attacker && attacker->IsCreature() && IsEnabledFor(attacker->ToCreature()))
            damage = static_cast<int32>(std::llround(
                damage * GetSpellDamageMultiplier(attacker->ToCreature(), spellInfo->Id)));
    }
};

class HeroicDungeonSpellScript final : public AllSpellScript
{
public:
    HeroicDungeonSpellScript() : AllSpellScript("HeroicDungeonSpellScript", { ALLSPELLHOOK_CAN_PREPARE }) { }

    bool CanPrepare(Spell* spell, SpellCastTargets const* /*targets*/, AuraEffect const* /*triggeredByAura*/) override
    {
        if (!spell || !spell->GetCaster() || !spell->GetCaster()->IsCreature())
            return true;

        Creature const* creature = spell->GetCaster()->ToCreature();
        return !ShouldReplaceOriginalCast(creature, spell->GetSpellInfo()->Id) || IsAuthorizedHeroicCast(spell);
    }
};
}

void AddSC_heroic_dungeons_engine()
{
    new HeroicDungeons::HeroicDungeonWorldScript();
    new HeroicDungeons::HeroicDungeonCreatureScript();
    new HeroicDungeons::HeroicDungeonUnitScript();
    new HeroicDungeons::HeroicDungeonSpellScript();
}
