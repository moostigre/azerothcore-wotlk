#include "heroic_dungeons.h"

#include "AllCreatureScript.h"
#include "AllSpellScript.h"
#include "Config.h"
#include "Creature.h"
#include "CreatureAI.h"
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

#include <algorithm>
#include <charconv>
#include <cmath>
#include <sstream>
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

std::vector<std::string> Split(std::string const& value, char delimiter)
{
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string part;
    while (std::getline(stream, part, delimiter))
        parts.push_back(part);
    return parts;
}

bool ParseUInt(std::string const& value, uint32& result)
{
    auto const* begin = value.data();
    auto const* end = begin + value.size();
    auto parsed = std::from_chars(begin, end, result);
    return parsed.ec == std::errc() && parsed.ptr == end;
}

bool ParseFloat(std::string const& value, float& result)
{
    try
    {
        std::size_t consumed = 0;
        result = std::stof(value, &consumed);
        return consumed == value.size() && std::isfinite(result);
    }
    catch (...)
    {
        return false;
    }
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

void ParseCreatureModifiers(std::string const& value)
{
    for (std::string const& record : Split(value, ';'))
    {
        if (record.empty())
            continue;

        std::vector<std::string> fields = Split(record, ':');
        uint32 entry = 0;
        CreatureModifier modifier;
        if (fields.size() != 3 || !ParseUInt(fields[0], entry) ||
            !ParseFloat(fields[1], modifier.health) || !ParseFloat(fields[2], modifier.meleeDamage))
        {
            LOG_ERROR("module.heroic-dungeons", "Invalid creature modifier '{}'", record);
            continue;
        }

        config.creatureModifiers[entry] = modifier;
    }
}

void ParseAbilities(std::string const& value)
{
    for (std::string const& record : Split(value, ';'))
    {
        if (record.empty())
            continue;

        std::vector<std::string> fields = Split(record, ':');
        Ability ability;
        uint32 target = 0;
        uint32 replaceOriginal = 0;
        if (fields.size() != 9 || !ParseUInt(fields[0], ability.creatureEntry) ||
            !ParseUInt(fields[1], ability.spellId) || !ParseFloat(fields[2], ability.damage) ||
            !ParseUInt(fields[3], ability.initialMin) || !ParseUInt(fields[4], ability.initialMax) ||
            !ParseUInt(fields[5], ability.cooldownMin) || !ParseUInt(fields[6], ability.cooldownMax) ||
            !ParseUInt(fields[7], target) || !ParseUInt(fields[8], replaceOriginal) || target > 2)
        {
            LOG_ERROR("module.heroic-dungeons", "Invalid heroic ability '{}'", record);
            continue;
        }

        ability.target = static_cast<AbilityTarget>(target);
        ability.replaceOriginal = replaceOriginal != 0;
        config.abilities[ability.creatureEntry].push_back(ability);
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

void LoadConfig()
{
    config = Config();
    config.enabled = sConfigMgr->GetOption<bool>("HeroicDungeons.Enable", true);
    config.stratholmeEnabled = sConfigMgr->GetOption<bool>("HeroicDungeons.Stratholme.Enable", true);
    config.stratholmeResetSeconds =
        sConfigMgr->GetOption<uint32>("HeroicDungeons.Stratholme.ResetSeconds", 86400);
    config.health = std::max(0.01f,
        sConfigMgr->GetOption<float>("HeroicDungeons.Stratholme.HealthMultiplier", 3.0f));
    config.meleeDamage = std::max(0.0f,
        sConfigMgr->GetOption<float>("HeroicDungeons.Stratholme.MeleeDamageMultiplier", 1.6f));
    config.spellDamage = std::max(0.0f,
        sConfigMgr->GetOption<float>("HeroicDungeons.Stratholme.SpellDamageMultiplier", 1.4f));
    config.serviceEntranceForcesHeroic = sConfigMgr->GetOption<bool>(
        "HeroicDungeons.Stratholme.ServiceEntranceForcesHeroic", true);
    config.baronEnabled = sConfigMgr->GetOption<bool>("HeroicDungeons.Stratholme.Baron.Enable", true);
    config.baronSkeletonEntry =
        sConfigMgr->GetOption<uint32>("HeroicDungeons.Stratholme.Baron.SkeletonEntry", 11197);
    config.baronSkeletonCount =
        sConfigMgr->GetOption<uint32>("HeroicDungeons.Stratholme.Baron.SkeletonCount", 6);
    config.baronRaiseDeadSpell =
        sConfigMgr->GetOption<uint32>("HeroicDungeons.Stratholme.Baron.RaiseDeadSpell", 17473);
    config.baronEnrageSpell =
        sConfigMgr->GetOption<uint32>("HeroicDungeons.Stratholme.Baron.EnrageSpell", 8599);
    config.anastariEnabled = sConfigMgr->GetOption<bool>("HeroicDungeons.Stratholme.Anastari.Enable", true);
    config.anastariBansheeEntry =
        sConfigMgr->GetOption<uint32>("HeroicDungeons.Stratholme.Anastari.BansheeEntry", 10464);
    config.anastariBansheeCount =
        sConfigMgr->GetOption<uint32>("HeroicDungeons.Stratholme.Anastari.BansheeCount", 3);
    config.anastariWailSpell =
        sConfigMgr->GetOption<uint32>("HeroicDungeons.Stratholme.Anastari.WailSpell", 16565);
    config.anastariEnrageSpell =
        sConfigMgr->GetOption<uint32>("HeroicDungeons.Stratholme.Anastari.EnrageSpell", 8599);

    ParseCreatureModifiers(sConfigMgr->GetOption<std::string>(
        "HeroicDungeons.Stratholme.CreatureModifiers", ""));
    ParseAbilities(sConfigMgr->GetOption<std::string>("HeroicDungeons.Stratholme.Abilities", ""));

    LOG_INFO("module.heroic-dungeons",
        "Loaded Heroic Stratholme: enabled={}, {} creature overrides, {} ability sets",
        config.enabled && config.stratholmeEnabled, config.creatureModifiers.size(), config.abilities.size());
}

bool IsEnabledFor(Creature const* creature)
{
    return creature && config.enabled && config.stratholmeEnabled && creature->GetMap() &&
        creature->GetMapId() == MAP_STRATHOLME && creature->GetMap()->IsHeroic();
}

float GetHealthMultiplier(Creature const* creature)
{
    auto itr = config.creatureModifiers.find(creature->GetEntry());
    return itr != config.creatureModifiers.end() && itr->second.health > 0.0f ? itr->second.health : config.health;
}

float GetMeleeDamageMultiplier(Creature const* creature)
{
    auto itr = config.creatureModifiers.find(creature->GetEntry());
    return itr != config.creatureModifiers.end() && itr->second.meleeDamage > 0.0f ?
        itr->second.meleeDamage : config.meleeDamage;
}

float GetSpellDamageMultiplier(Creature const* creature, uint32 spellId)
{
    auto itr = config.abilities.find(creature->GetEntry());
    if (itr != config.abilities.end())
        for (Ability const& ability : itr->second)
            if (ability.spellId == spellId && ability.damage > 0.0f)
                return ability.damage;
    return config.spellDamage;
}

bool ShouldReplaceOriginalCast(Creature const* creature, uint32 spellId)
{
    if (!IsEnabledFor(creature))
        return false;

    auto itr = config.abilities.find(creature->GetEntry());
    if (itr != config.abilities.end())
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
    auto rules = config.abilities.find(creature->GetEntry());
    if (rules == config.abilities.end())
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
        if (config.enabled && config.stratholmeEnabled)
        {
            sMapDifficultyMap[MAKE_PAIR32(MAP_STRATHOLME, DUNGEON_DIFFICULTY_HEROIC)] =
                MapDifficulty(config.stratholmeResetSeconds, 5, false);
        }
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

        uint64 maximumHealth = std::max<uint64>(1,
            static_cast<uint64>(std::llround(creature->GetMaxHealth() * GetHealthMultiplier(creature))));
        creature->SetMaxHealth(static_cast<uint32>(std::min<uint64>(maximumHealth, UINT32_MAX)));
        creature->SetHealth(creature->GetMaxHealth());
    }

    void OnAllCreatureUpdate(Creature* creature, uint32 diff) override
    {
        if (IsEnabledFor(creature))
            UpdateAbilities(creature, diff);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
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
