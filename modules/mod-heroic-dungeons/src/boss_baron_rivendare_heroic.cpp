#include "heroic_dungeons.h"

#include "AllCreatureScript.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "Map.h"
#include "Random.h"
#include "TemporarySummon.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace
{
struct BaronState
{
    bool skeletonWave = false;
    bool enraged = false;
};

std::unordered_map<ObjectGuid, BaronState> baronStates;

class HeroicBaronRivendareScript final : public AllCreatureScript
{
public:
    HeroicBaronRivendareScript() : AllCreatureScript("HeroicBaronRivendareScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        HeroicDungeons::DungeonConfig const* dungeon = HeroicDungeons::GetDungeonConfig(creature->GetMapId());
        if (!dungeon || !dungeon->baron.enabled ||
            creature->GetEntry() != HeroicDungeons::NPC_BARON_RIVENDARE ||
            !HeroicDungeons::IsEnabledFor(creature))
        {
            return;
        }

        BaronState& state = baronStates[creature->GetGUID()];
        if (!creature->IsInCombat() || !creature->IsAlive())
        {
            state = BaronState();
            return;
        }

        if (!state.skeletonWave && creature->HealthBelowPct(70))
        {
            state.skeletonWave = true;
            creature->CastSpell(creature, dungeon->baron.raiseDeadSpell, true);

            constexpr float TWO_PI = 6.28318530718f;
            for (uint32 index = 0; index < dungeon->baron.skeletonCount; ++index)
            {
                float angle = TWO_PI * float(index) / float(std::max<uint32>(1, dungeon->baron.skeletonCount));
                float x = creature->GetPositionX() + std::cos(angle) * 6.0f;
                float y = creature->GetPositionY() + std::sin(angle) * 6.0f;
                if (TempSummon* summon = creature->SummonCreature(dungeon->baron.skeletonEntry, x, y,
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
            creature->CastSpell(creature, dungeon->baron.enrageSpell, true);
        }
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature->GetEntry() == HeroicDungeons::NPC_BARON_RIVENDARE)
            baronStates.erase(creature->GetGUID());
    }
};
}

void AddSC_boss_baron_rivendare_heroic()
{
    new HeroicBaronRivendareScript();
}
