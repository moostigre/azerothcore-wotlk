#include "heroic_dungeons.h"

#include "AreaTriggerScript.h"
#include "Chat.h"
#include "Group.h"
#include "Player.h"

namespace
{
class HeroicStratholmeServiceEntrance final : public AreaTriggerScript
{
public:
    HeroicStratholmeServiceEntrance() : AreaTriggerScript("heroic_stratholme_service_entrance") { }

    bool OnTrigger(Player* player, AreaTrigger const* /*trigger*/) override
    {
        HeroicDungeons::DungeonConfig const* dungeon =
            HeroicDungeons::GetDungeonConfig(HeroicDungeons::MAP_STRATHOLME);
        if (!HeroicDungeons::GetConfig().enabled || !dungeon || !dungeon->enabled ||
            !dungeon->serviceEntranceForcesHeroic)
            return false;

        if (Group* group = player->GetGroup())
            group->SetDungeonDifficulty(DUNGEON_DIFFICULTY_HEROIC);
        else
            player->SetDungeonDifficulty(DUNGEON_DIFFICULTY_HEROIC);

        ChatHandler(player->GetSession()).SendSysMessage(
            "|cff00ff00[Heroic Dungeons]|r Heroic Stratholme selected through the service entrance.");
        return false;
    }
};
}

void AddSC_stratholme_heroic_entrance()
{
    new HeroicStratholmeServiceEntrance();
}
