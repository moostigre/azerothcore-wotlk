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

struct BaronMechanics
{
    bool enabled = false;
    uint32 skeletonEntry = 11197;
    uint32 skeletonCount = 6;
    uint32 raiseDeadSpell = 17473;
    uint32 enrageSpell = 8599;
};

struct AnastariMechanics
{
    bool enabled = false;
    uint32 bansheeEntry = 10464;
    uint32 bansheeCount = 3;
    uint32 wailSpell = 16565;
    uint32 enrageSpell = 8599;
};

struct DungeonConfig
{
    std::string name;
    uint32 mapId = 0;
    bool enabled = true;
    uint32 resetSeconds = 86400;
    float health = 1.0f;
    float meleeDamage = 1.0f;
    float spellDamage = 1.0f;
    bool serviceEntranceForcesHeroic = false;
    std::unordered_map<uint32, CreatureModifier> creatureModifiers;
    std::unordered_map<uint32, std::vector<Ability>> abilities;
    std::unordered_map<uint32, LootRule> lootRules;
    BaronMechanics baron;
    AnastariMechanics anastari;
};

struct Config
{
    bool enabled = true;
    std::string yamlPath = "modules/heroic_dungeons.yaml";
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
