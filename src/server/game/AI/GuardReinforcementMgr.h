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

class Creature;
class Unit;

class GuardReinforcementMgr
{
public:
    static GuardReinforcementMgr* instance();

    void LoadGuardReinforcements();
    bool TrySummonGuard(Creature* caller, Unit* enemy);

private:
    GuardReinforcementMgr() = default;
};

#define sGuardReinforcementMgr GuardReinforcementMgr::instance()

#endif
