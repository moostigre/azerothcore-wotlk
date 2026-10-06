/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "GameObjectUtils.h"

#include <gtest/gtest.h>

using Acore::GameObjectUtils::ShouldApplyTrapStartDelay;

TEST(GameObjectTrapStartDelayTest, OwnerAppliesDelayWithoutSpellId)
{
    EXPECT_TRUE(ShouldApplyTrapStartDelay(true, 0));
}

TEST(GameObjectTrapStartDelayTest, SpellSummonedTrapAppliesDelayWithoutOwner)
{
    EXPECT_TRUE(ShouldApplyTrapStartDelay(false, 31706));
}

TEST(GameObjectTrapStartDelayTest, StaticTrapWithoutOwnerOrSpellIdRemainsImmediate)
{
    EXPECT_FALSE(ShouldApplyTrapStartDelay(false, 0));
}

TEST(GameObjectTrapStartDelayTest, OwnerWithSpellIdAppliesDelay)
{
    EXPECT_TRUE(ShouldApplyTrapStartDelay(true, 31706));
}
