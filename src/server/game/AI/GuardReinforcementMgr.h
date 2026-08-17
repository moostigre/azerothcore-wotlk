/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef ACORE_GUARD_REINFORCEMENT_MGR_H
#define ACORE_GUARD_REINFORCEMENT_MGR_H

#include "Define.h"
#include "ObjectGuid.h"

#include <chrono>
#include <optional>
#include <unordered_map>

class Creature;
class Unit;

struct GuardReinforcementReservation
{
    uint32 guardEntry;
    std::chrono::steady_clock::time_point previousNextUse;
    std::chrono::steady_clock::time_point reservedUntil;
};

class GuardReinforcementPost
{
public:
    explicit GuardReinforcementPost(uint32 guardEntry,
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
    std::optional<GuardReinforcementReservation> Reserve(std::chrono::steady_clock::time_point now);
    void Refund(GuardReinforcementReservation const& reservation);
    uint8 GetCharges() const { return _charges; }

private:
    uint32 _guardEntry;
    uint8 _charges;
    std::chrono::steady_clock::time_point _nextUse;
    std::chrono::steady_clock::time_point _lastRecharge;
};

class GuardReinforcementTracker
{
public:
    std::optional<ObjectGuid> GetGuard(ObjectGuid caller) const;
    void Track(ObjectGuid caller, ObjectGuid guard);
    void Remove(ObjectGuid creature);
    void Clear();

private:
    std::unordered_map<ObjectGuid, ObjectGuid> _guardsByCaller;
    std::unordered_map<ObjectGuid, ObjectGuid> _callersByGuard;
};

class GuardReinforcementMgr
{
public:
    static GuardReinforcementMgr* instance();

    void LoadGuardReinforcements();
    bool TrySummonGuard(Creature* caller, Unit* enemy);
    void OnCreatureRemoved(Creature* creature);

private:
    GuardReinforcementMgr() = default;
};

#define sGuardReinforcementMgr GuardReinforcementMgr::instance()

#endif
