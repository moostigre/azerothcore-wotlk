#include "heroic_dungeons.h"

#include "AllCreatureScript.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "TemporarySummon.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace
{
struct AnastariState
{
    bool bansheeWave = false;
    bool enraged = false;
};

std::unordered_map<ObjectGuid, AnastariState> anastariStates;

class HeroicBaronessAnastariScript final : public AllCreatureScript
{
public:
    HeroicBaronessAnastariScript() : AllCreatureScript("HeroicBaronessAnastariScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        HeroicDungeons::Config const& config = HeroicDungeons::GetConfig();
        if (!config.anastariEnabled || creature->GetEntry() != HeroicDungeons::NPC_BARONESS_ANASTARI ||
            !HeroicDungeons::IsEnabledFor(creature))
        {
            return;
        }

        // Creature loot defaults to mode 1 even on Heroic dungeon maps. Enable
        // the module's mode 2 rows only after the map difficulty is verified.
        creature->AddLootMode(2);

        AnastariState& state = anastariStates[creature->GetGUID()];
        if (!creature->IsInCombat() || !creature->IsAlive())
        {
            state = AnastariState();
            return;
        }

        if (!state.bansheeWave && creature->HealthBelowPct(70))
        {
            state.bansheeWave = true;
            creature->CastSpell(creature, config.anastariWailSpell, true);

            constexpr float TWO_PI = 6.28318530718f;
            for (uint32 index = 0; index < config.anastariBansheeCount; ++index)
            {
                float angle = TWO_PI * float(index) / float(std::max<uint32>(1, config.anastariBansheeCount));
                float x = creature->GetPositionX() + std::cos(angle) * 5.0f;
                float y = creature->GetPositionY() + std::sin(angle) * 5.0f;
                if (TempSummon* summon = creature->SummonCreature(config.anastariBansheeEntry, x, y,
                    creature->GetPositionZ(), angle, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000))
                {
                    if (Unit* victim = creature->GetVictim())
                        summon->AI()->AttackStart(victim);
                }
            }
        }

        if (!state.enraged && creature->HealthBelowPct(35))
        {
            state.enraged = true;
            creature->CastSpell(creature, config.anastariEnrageSpell, true);
        }
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature->GetEntry() == HeroicDungeons::NPC_BARONESS_ANASTARI)
            anastariStates.erase(creature->GetGUID());
    }
};
}

void AddSC_boss_baroness_anastari_heroic()
{
    new HeroicBaronessAnastariScript();
}
