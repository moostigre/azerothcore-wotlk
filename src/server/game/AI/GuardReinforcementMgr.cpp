/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "GuardReinforcementMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QueryResult.h"
#include "Random.h"
#include "SharedDefines.h"
#include <algorithm>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace
{
    constexpr uint8 MaxCharges = 10;
    constexpr std::chrono::seconds UseCooldown{ 10 };
    constexpr std::chrono::minutes ChargeRechargeTime{ 1 };
    constexpr uint32 GuardOutOfCombatDespawnTime = 30 * IN_MILLISECONDS;

    struct GuardPost
    {
        uint32 guardEntry = 0;
        uint8 charges = MaxCharges;
        std::chrono::steady_clock::time_point nextUse;
        std::chrono::steady_clock::time_point lastRecharge = std::chrono::steady_clock::now();
    };

    using GuardPostKey = uint64;

    GuardPostKey MakeKey(uint32 areaId, TeamId team)
    {
        return (uint64(areaId) << 8) | uint8(team);
    }

    std::unordered_map<GuardPostKey, GuardPost> GuardPosts;
    std::unordered_map<ObjectGuid, ObjectGuid> ActiveReinforcements;
    std::unordered_set<uint32> ExplicitCallers;
    std::mutex GuardPostLock;
}

GuardReinforcementMgr* GuardReinforcementMgr::instance()
{
    static GuardReinforcementMgr instance;
    return &instance;
}

void GuardReinforcementMgr::LoadGuardReinforcements()
{
    uint32 oldMSTime = getMSTime();

    std::lock_guard<std::mutex> lock(GuardPostLock);
    GuardPosts.clear();
    ActiveReinforcements.clear();
    ExplicitCallers.clear();

    if (QueryResult result = WorldDatabase.Query("SELECT areaId, team, guardEntry FROM guard_reinforcement"))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 areaId = fields[0].Get<uint32>();
            uint8 teamValue = fields[1].Get<uint8>();
            uint32 guardEntry = fields[2].Get<uint32>();

            if (teamValue > TEAM_NEUTRAL)
            {
                LOG_ERROR("sql.sql", "Table `guard_reinforcement` has invalid team {} for area {}.", teamValue, areaId);
                continue;
            }

            if (!sObjectMgr->GetCreatureTemplate(guardEntry))
            {
                LOG_ERROR("sql.sql", "Table `guard_reinforcement` references missing creature template {} for area {}.", guardEntry, areaId);
                continue;
            }

            GuardPost post;
            post.guardEntry = guardEntry;
            GuardPosts.emplace(MakeKey(areaId, TeamId(teamValue)), post);
        } while (result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query("SELECT creatureEntry FROM guard_reinforcement_caller"))
    {
        do
        {
            uint32 creatureEntry = result->Fetch()[0].Get<uint32>();
            if (!sObjectMgr->GetCreatureTemplate(creatureEntry))
            {
                LOG_ERROR("sql.sql", "Table `guard_reinforcement_caller` references missing creature template {}.", creatureEntry);
                continue;
            }

            ExplicitCallers.insert(creatureEntry);
        } while (result->NextRow());
    }

    LOG_INFO("server.loading", ">> Loaded {} guard reinforcement definitions and {} explicit callers in {} ms.",
        GuardPosts.size(), ExplicitCallers.size(), GetMSTimeDiffToNow(oldMSTime));
}

bool GuardReinforcementMgr::TrySummonGuard(Creature* caller, Unit* enemy)
{
    if (!caller || !enemy || !caller->IsAlive() || caller->IsPet() || caller->IsCharmed() || caller->IsSummon())
        return false;

    // This method is called from the visibility hot path. Reject ordinary creatures
    // before taking the shared guard-post lock.
    if (!caller->IsGuard() && !caller->IsCivilian() && !ExplicitCallers.contains(caller->GetEntry()))
        return false;

    Player* player = enemy->GetCharmerOrOwnerPlayerOrPlayerItself();
    if (!player || !caller->IsHostileTo(player))
        return false;

    uint32 guardEntry = 0;
    {
        std::lock_guard<std::mutex> lock(GuardPostLock);

        auto activeItr = ActiveReinforcements.find(caller->GetGUID());
        if (activeItr != ActiveReinforcements.end())
        {
            if (ObjectAccessor::GetCreature(*caller, activeItr->second))
                return false;

            ActiveReinforcements.erase(activeItr);
        }

        TeamId guardTeam = player->GetTeamId() == TEAM_ALLIANCE ? TEAM_HORDE : TEAM_ALLIANCE;
        auto itr = GuardPosts.find(MakeKey(caller->GetAreaId(), guardTeam));
        if (itr == GuardPosts.end())
            itr = GuardPosts.find(MakeKey(caller->GetAreaId(), TEAM_NEUTRAL));
        if (itr == GuardPosts.end())
            return false;

        GuardPost& post = itr->second;
        auto now = std::chrono::steady_clock::now();
        if (now - post.lastRecharge >= ChargeRechargeTime)
        {
            uint64 recovered = std::chrono::duration_cast<std::chrono::minutes>(now - post.lastRecharge).count();
            post.charges = std::min<uint64>(MaxCharges, uint64(post.charges) + recovered);
            post.lastRecharge += ChargeRechargeTime * recovered;
        }

        if (!post.charges || now < post.nextUse)
            return false;

        --post.charges;
        post.nextUse = now + UseCooldown;
        guardEntry = post.guardEntry;
    }

    Position spawnPosition = caller->GetNearPosition(5.0f, frand(0.0f, 2.0f * float(M_PI)));
    if (Creature* guard = caller->SummonCreature(guardEntry, spawnPosition, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, GuardOutOfCombatDespawnTime))
    {
        {
            std::lock_guard<std::mutex> lock(GuardPostLock);
            ActiveReinforcements[caller->GetGUID()] = guard->GetGUID();
        }

        guard->AI()->AttackStart(player);
        return true;
    }

    LOG_ERROR("entities.unit", "Failed to summon guard reinforcement {} for creature {} in area {}.",
        guardEntry, caller->GetGUID().ToString(), caller->GetAreaId());
    return false;
}
