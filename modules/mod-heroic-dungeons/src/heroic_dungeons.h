#ifndef MOD_HEROIC_DUNGEONS_H
#define MOD_HEROIC_DUNGEONS_H

#include "Define.h"
#include "ObjectGuid.h"

#include <unordered_map>
#include <string>
#include <vector>

class Creature;
class Spell;
class Unit;

namespace HeroicDungeons
{
constexpr uint32 MAP_STRATHOLME = 329;
constexpr uint32 NPC_BARON_RIVENDARE = 10440;
constexpr uint32 NPC_BARONESS_ANASTARI = 10436;

enum class AbilityTarget : uint8
{
    Victim = 0,
    RandomPlayer = 1,
    Self = 2
};

struct CreatureModifier
{
    float health = 0.0f;
    float meleeDamage = 0.0f;
};

struct Ability
{
    uint32 creatureEntry = 0;
    uint32 spellId = 0;
    float damage = 0.0f;
    uint32 initialMin = 0;
    uint32 initialMax = 0;
    uint32 cooldownMin = 0;
    uint32 cooldownMax = 0;
    AbilityTarget target = AbilityTarget::Victim;
    bool replaceOriginal = false;
};

enum class LootOverrideMode : uint8
{
    Add,
    Replace
};

struct LootItemRule
{
    uint32 itemId = 0;
    float chance = 0.0f;
    uint8 minCount = 1;
    uint8 maxCount = 1;
    uint8 groupId = 0;
};

struct LootRule
{
    LootOverrideMode mode = LootOverrideMode::Add;
    std::vector<LootItemRule> items;
};

enum class PhaseActionType : uint8
{
    Cast,
    Summon
};

struct PhaseAction
{
    PhaseActionType type = PhaseActionType::Cast;
    uint32 id = 0;
    AbilityTarget target = AbilityTarget::Self;
    uint32 count = 1;
    float radius = 5.0f;
    uint32 despawnMs = 10000;
};

struct Phase
{
    uint8 healthBelow = 0;
    bool once = true;
    uint32 repeatMs = 1000;
    std::vector<PhaseAction> actions;
};

struct DungeonConfig
{
    std::string name;
    uint32 mapId = 0;
    bool enabled = true;
    uint32 resetSeconds = 86400;
    uint8 minimumLevel = 1;
    uint8 maximumLevel = 80;
    std::vector<uint32> heroicEntranceTriggers;
    float health = 1.0f;
    float meleeDamage = 1.0f;
    float spellDamage = 1.0f;
    std::unordered_map<uint32, CreatureModifier> creatureModifiers;
    std::unordered_map<uint32, std::vector<Ability>> abilities;
    std::unordered_map<uint32, LootRule> lootRules;
    std::unordered_map<uint32, std::vector<Phase>> phases;
};

struct Config
{
    bool enabled = true;
    std::string yamlPath = "etc/modules/heroic_dungeons.yaml";
    std::unordered_map<uint32, DungeonConfig> dungeons;
};

Config const& GetConfig();
DungeonConfig const* GetDungeonConfig(uint32 mapId);
void LoadConfig();
bool IsEnabledFor(Creature const* creature);
float GetHealthMultiplier(Creature const* creature);
float GetMeleeDamageMultiplier(Creature const* creature);
float GetSpellDamageMultiplier(Creature const* creature, uint32 spellId);
bool ShouldReplaceOriginalCast(Creature const* creature, uint32 spellId);
bool IsAuthorizedHeroicCast(Spell const* spell);
void UpdateAbilities(Creature* creature, uint32 diff);
void ResetCreature(ObjectGuid guid);
}

#endif
