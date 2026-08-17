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
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace
{
    constexpr uint8 MaxCharges = 10;
    constexpr std::chrono::seconds UseCooldown{ 10 };
    constexpr std::chrono::minutes ChargeRechargeTime{ 1 };
    constexpr uint32 GuardOutOfCombatDespawnTime = 30 * IN_MILLISECONDS;

    using GuardPostKey = uint64;

    GuardPostKey MakeKey(uint32 areaId, TeamId team)
    {
        return (uint64(areaId) << 8) | uint8(team);
    }

    std::unordered_map<GuardPostKey, GuardReinforcementPost> GuardPosts;
    GuardReinforcementTracker ActiveReinforcements;
    std::unordered_set<uint32> ExplicitCallers;
    std::mutex GuardPostLock;
}

GuardReinforcementPost::GuardReinforcementPost(uint32 guardEntry, std::chrono::steady_clock::time_point now)
    : _guardEntry(guardEntry), _charges(MaxCharges), _lastRecharge(now)
{
}

std::optional<GuardReinforcementReservation> GuardReinforcementPost::Reserve(
    std::chrono::steady_clock::time_point now)
{
    if (now - _lastRecharge >= ChargeRechargeTime)
    {
        uint64 recovered = std::chrono::duration_cast<std::chrono::minutes>(now - _lastRecharge).count();
        _charges = std::min<uint64>(MaxCharges, uint64(_charges) + recovered);
        _lastRecharge += ChargeRechargeTime * recovered;
    }

    if (!_charges || now < _nextUse)
        return std::nullopt;

    --_charges;
    GuardReinforcementReservation reservation{ _guardEntry, _nextUse, now + UseCooldown };
    _nextUse = reservation.reservedUntil;
    return reservation;
}

void GuardReinforcementPost::Refund(GuardReinforcementReservation const& reservation)
{
    _charges = std::min<uint8>(MaxCharges, _charges + 1);
    if (_nextUse == reservation.reservedUntil)
        _nextUse = reservation.previousNextUse;
}

std::optional<ObjectGuid> GuardReinforcementTracker::GetGuard(ObjectGuid caller) const
{
    if (auto itr = _guardsByCaller.find(caller); itr != _guardsByCaller.end())
        return itr->second;
    return std::nullopt;
}

void GuardReinforcementTracker::Track(ObjectGuid caller, ObjectGuid guard)
{
    Remove(caller);
    _guardsByCaller[caller] = guard;
    _callersByGuard[guard] = caller;
}

void GuardReinforcementTracker::Remove(ObjectGuid creature)
{
    if (auto itr = _guardsByCaller.find(creature); itr != _guardsByCaller.end())
    {
        _callersByGuard.erase(itr->second);
        _guardsByCaller.erase(itr);
    }
    if (auto itr = _callersByGuard.find(creature); itr != _callersByGuard.end())
    {
        _guardsByCaller.erase(itr->second);
        _callersByGuard.erase(itr);
    }
}

void GuardReinforcementTracker::Clear()
{
    _guardsByCaller.clear();
    _callersByGuard.clear();
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
    ActiveReinforcements.Clear();
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

            GuardPosts.emplace(MakeKey(areaId, TeamId(teamValue)), GuardReinforcementPost(guardEntry));
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

    GuardPostKey postKey = 0;
    std::optional<GuardReinforcementReservation> reservation;
    {
        std::lock_guard<std::mutex> lock(GuardPostLock);

        if (std::optional<ObjectGuid> activeGuard = ActiveReinforcements.GetGuard(caller->GetGUID()))
        {
            if (ObjectAccessor::GetCreature(*caller, *activeGuard))
                return false;

            ActiveReinforcements.Remove(caller->GetGUID());
        }

        TeamId guardTeam = player->GetTeamId() == TEAM_ALLIANCE ? TEAM_HORDE : TEAM_ALLIANCE;
        auto itr = GuardPosts.find(MakeKey(caller->GetAreaId(), guardTeam));
        if (itr == GuardPosts.end())
            itr = GuardPosts.find(MakeKey(caller->GetAreaId(), TEAM_NEUTRAL));
        if (itr == GuardPosts.end())
            return false;

        reservation = itr->second.Reserve(std::chrono::steady_clock::now());
        if (!reservation)
            return false;
        postKey = itr->first;
    }

    Position spawnPosition = caller->GetNearPosition(5.0f, frand(0.0f, 2.0f * float(M_PI)));
    if (Creature* guard = caller->SummonCreature(reservation->guardEntry, spawnPosition, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, GuardOutOfCombatDespawnTime))
    {
        {
            std::lock_guard<std::mutex> lock(GuardPostLock);
            ActiveReinforcements.Track(caller->GetGUID(), guard->GetGUID());
        }

        guard->AI()->AttackStart(player);
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(GuardPostLock);
        if (auto itr = GuardPosts.find(postKey); itr != GuardPosts.end())
            itr->second.Refund(*reservation);
    }

    LOG_ERROR("entities.unit", "Failed to summon guard reinforcement {} for creature {} in area {}.",
        reservation->guardEntry, caller->GetGUID().ToString(), caller->GetAreaId());
    return false;
}

void GuardReinforcementMgr::OnCreatureRemoved(Creature* creature)
{
    if (!creature)
        return;

    std::lock_guard<std::mutex> lock(GuardPostLock);
    ActiveReinforcements.Remove(creature->GetGUID());
}
