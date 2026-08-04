#include "heroic_dungeons.h"

#include "AreaTriggerScript.h"
#include "Chat.h"
#include "Group.h"
#include "Player.h"

#include <algorithm>

namespace
{
class HeroicDungeonEntrance final : public AreaTriggerScript
{
public:
    HeroicDungeonEntrance() : AreaTriggerScript("heroic_dungeon_entrance") { }

    bool OnTrigger(Player* player, AreaTrigger const* trigger) override
    {
        HeroicDungeons::DungeonConfig const* dungeon = nullptr;
        for (auto const& [mapId, candidate] : HeroicDungeons::GetConfig().dungeons)
            if (std::find(candidate.heroicEntranceTriggers.begin(), candidate.heroicEntranceTriggers.end(),
                trigger->entry) != candidate.heroicEntranceTriggers.end())
            {
                dungeon = HeroicDungeons::GetDungeonConfig(mapId);
                break;
            }

        if (!HeroicDungeons::GetConfig().enabled || !dungeon || !dungeon->enabled)
            return false;

        if (Group* group = player->GetGroup())
            group->SetDungeonDifficulty(DUNGEON_DIFFICULTY_HEROIC);
        else
            player->SetDungeonDifficulty(DUNGEON_DIFFICULTY_HEROIC);

        ChatHandler(player->GetSession()).PSendSysMessage(
            "|cff00ff00[Heroic Dungeons]|r Heroic {} selected.", dungeon->name);
        return false;
    }
};
}

void AddSC_heroic_dungeon_entrance()
{
    new HeroicDungeonEntrance();
}
