/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "IntegrationTestFixture.h"

namespace
{
class UpdateEligibilityCreature : public TestCreature
{
public:
    using Creature::GetCurrentCell;

    void SetRespawnState(bool compatibilityMode, DeathState state)
    {
        _respawnCompatibilityMode = compatibilityMode;
        m_deathState = state;
    }
};
}

class CreatureUpdateEligibilityTest : public IntegrationTestFixture
{
protected:
    void CheckUpdateEligibility(bool compatibilityMode, DeathState state, bool expected)
    {
        UpdateEligibilityCreature creature;
        creature.ForceInitValues(1, 27631);
        creature.SetTestMap(GetTestMap());
        creature.SetRespawnState(compatibilityMode, state);

        ASSERT_FALSE(creature.isActiveObject());
        ASSERT_FALSE(creature.IsInCombat());
        ASSERT_FALSE(GetTestMap()->isCellMarked(creature.GetCurrentCell().GetCellCoord().GetId()));
        EXPECT_EQ(creature.IsUpdateNeeded(), expected);
    }
};

// cppcheck-suppress syntaxError
TEST_F(CreatureUpdateEligibilityTest, OffscreenDynamicCorpseKeepsUpdating)
{
    CheckUpdateEligibility(false, DeathState::Corpse, true);
}

// cppcheck-suppress syntaxError
TEST_F(CreatureUpdateEligibilityTest, OffscreenLivingDynamicCreatureCanStopUpdating)
{
    CheckUpdateEligibility(false, DeathState::Alive, false);
}

// cppcheck-suppress syntaxError
TEST_F(CreatureUpdateEligibilityTest, OffscreenDeadDynamicCreatureCanStopUpdating)
{
    CheckUpdateEligibility(false, DeathState::Dead, false);
}

// cppcheck-suppress syntaxError
TEST_F(CreatureUpdateEligibilityTest, CompatibilityCorpseKeepsExistingUpdatePolicy)
{
    CheckUpdateEligibility(true, DeathState::Corpse, false);
}
