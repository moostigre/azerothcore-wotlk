/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Group.h"
#include "MiscScript.h"
#include "ScriptMgr.h"
#include "gtest/gtest.h"

namespace
{
LootItem CreateLootItem()
{
    LootItem item{};
    item.itemid = 1;
    item.randomPropertyId = 0;
    item.randomSuffix = 0;
    item.count = 1;
    return item;
}

ObjectGuid PlayerGuid(uint32 counter)
{
    return ObjectGuid::Create<HighGuid::Player>(counter);
}

class TestGroup : public Group
{
public:
    void AddRoll(Roll* roll)
    {
        RollId.push_back(roll);
    }

    std::size_t GetRollCount() const
    {
        return RollId.size();
    }
};
}

TEST(GroupLootRollTest, EligiblePlayerWithPassOnLootCountsAsPass)
{
    LootItem const item = CreateLootItem();
    Roll roll(ObjectGuid::Create<HighGuid::Item>(1), item);
    ObjectGuid const playerGuid = PlayerGuid(1);

    roll.AddPlayerVote(playerGuid, true, true);

    EXPECT_EQ(roll.totalPlayersRolling, 1);
    EXPECT_EQ(roll.totalPass, 1);
    EXPECT_EQ(roll.playerVote[playerGuid], PASS);
    EXPECT_FALSE(roll.IsAutoPass(playerGuid));
}

TEST(GroupLootRollTest, IneligiblePlayerCountsAsForcedAutoPass)
{
    LootItem const item = CreateLootItem();
    Roll roll(ObjectGuid::Create<HighGuid::Item>(1), item);
    ObjectGuid const playerGuid = PlayerGuid(1);

    roll.AddPlayerVote(playerGuid, false, false);

    EXPECT_EQ(roll.totalPlayersRolling, 1);
    EXPECT_EQ(roll.totalPass, 1);
    EXPECT_EQ(roll.playerVote[playerGuid], PASS);
    EXPECT_TRUE(roll.IsAutoPass(playerGuid));
}

TEST(GroupLootRollTest, EligiblePlayerWithoutPassOnLootWaitsForVote)
{
    LootItem const item = CreateLootItem();
    Roll roll(ObjectGuid::Create<HighGuid::Item>(1), item);
    ObjectGuid const playerGuid = PlayerGuid(1);

    roll.AddPlayerVote(playerGuid, false, true);

    EXPECT_EQ(roll.totalPlayersRolling, 1);
    EXPECT_EQ(roll.totalPass, 0);
    EXPECT_EQ(roll.playerVote[playerGuid], NOT_EMITED_YET);
    EXPECT_FALSE(roll.IsAutoPass(playerGuid));
}

TEST(GroupLootRollTest, AutoPassesAllowRollToCompleteWhenOtherPlayersVote)
{
    LootItem const item = CreateLootItem();
    Roll roll(ObjectGuid::Create<HighGuid::Item>(1), item);
    ObjectGuid const optedPassGuid = PlayerGuid(1);
    ObjectGuid const forcedPassGuid = PlayerGuid(2);
    ObjectGuid const needGuid = PlayerGuid(3);
    ObjectGuid const greedGuid = PlayerGuid(4);

    roll.AddPlayerVote(optedPassGuid, true, true);
    roll.AddPlayerVote(forcedPassGuid, false, false);
    roll.AddPlayerVote(needGuid, false, true);
    roll.AddPlayerVote(greedGuid, false, true);

    roll.playerVote[needGuid] = NEED;
    ++roll.totalNeed;
    roll.playerVote[greedGuid] = GREED;
    ++roll.totalGreed;

    EXPECT_EQ(roll.totalPass, 2);
    EXPECT_TRUE(roll.IsComplete());
}

TEST(GroupLootRollTest, AllPassFinalizationUnblocksLootItem)
{
    LootItem item = CreateLootItem();
    item.is_blocked = true;
    Roll roll(ObjectGuid::Create<HighGuid::Item>(1), item);
    roll.AddPlayerVote(PlayerGuid(1), true, true);
    roll.AddPlayerVote(PlayerGuid(2), true, true);

    EXPECT_TRUE(roll.IsComplete());
    EXPECT_TRUE(roll.FinalizeIfAllPassed(item));
    EXPECT_FALSE(item.is_blocked);
}

TEST(GroupLootRollTest, MixedPassDoesNotFinalizeOrUnblockLootItem)
{
    LootItem item = CreateLootItem();
    item.is_blocked = true;
    Roll roll(ObjectGuid::Create<HighGuid::Item>(1), item);
    roll.AddPlayerVote(PlayerGuid(1), true, true);
    roll.AddPlayerVote(PlayerGuid(2), false, true);

    EXPECT_FALSE(roll.IsComplete());
    EXPECT_FALSE(roll.FinalizeIfAllPassed(item));
    EXPECT_TRUE(item.is_blocked);
}

TEST(GroupLootRollTest, TimeoutConvertsOnlyPendingVotesToPasses)
{
    LootItem const item = CreateLootItem();
    Roll roll(ObjectGuid::Create<HighGuid::Item>(1), item);
    ObjectGuid const optedPassGuid = PlayerGuid(1);
    ObjectGuid const forcedPassGuid = PlayerGuid(2);
    ObjectGuid const timedOutGuid = PlayerGuid(3);
    ObjectGuid const needGuid = PlayerGuid(4);

    roll.AddPlayerVote(optedPassGuid, true, true);
    roll.AddPlayerVote(forcedPassGuid, false, false);
    roll.AddPlayerVote(timedOutGuid, false, true);
    roll.AddPlayerVote(needGuid, false, true);
    roll.playerVote[needGuid] = NEED;
    ++roll.totalNeed;

    std::vector<ObjectGuid> const timedOutPlayers = roll.ResolvePendingVotesAsPass();

    ASSERT_EQ(timedOutPlayers.size(), 1);
    EXPECT_EQ(timedOutPlayers[0], timedOutGuid);
    EXPECT_EQ(roll.playerVote[optedPassGuid], PASS);
    EXPECT_EQ(roll.playerVote[forcedPassGuid], PASS);
    EXPECT_EQ(roll.playerVote[timedOutGuid], PASS);
    EXPECT_EQ(roll.playerVote[needGuid], NEED);
    EXPECT_EQ(roll.totalPass, 3);
    EXPECT_EQ(roll.totalNeed, 1);
    EXPECT_TRUE(roll.IsComplete());
    EXPECT_FALSE(roll.IsAutoPass(timedOutGuid));
}

TEST(GroupLootRollTest, TimeoutAllPassFinalizationUnblocksLootItem)
{
    LootItem item = CreateLootItem();
    item.is_blocked = true;
    Roll roll(ObjectGuid::Create<HighGuid::Item>(1), item);
    roll.AddPlayerVote(PlayerGuid(1), false, true);
    roll.AddPlayerVote(PlayerGuid(2), false, true);

    EXPECT_EQ(roll.ResolvePendingVotesAsPass().size(), 2);
    EXPECT_TRUE(roll.FinalizeIfAllPassed(item));
    EXPECT_FALSE(item.is_blocked);
}

TEST(GroupLootRollTest, ResolvingTimeoutTwiceDoesNotDoubleCountPasses)
{
    LootItem const item = CreateLootItem();
    Roll roll(ObjectGuid::Create<HighGuid::Item>(1), item);
    ObjectGuid const timedOutGuid = PlayerGuid(1);
    roll.AddPlayerVote(timedOutGuid, false, true);

    EXPECT_EQ(roll.ResolvePendingVotesAsPass().size(), 1);
    EXPECT_TRUE(roll.ResolvePendingVotesAsPass().empty());
    EXPECT_EQ(roll.totalPass, 1);
}

TEST(GroupLootRollTest, QuestOnlyLootAcceptsExplicitVote)
{
    Loot loot;
    loot.quest_items.push_back(CreateLootItem());
    ObjectGuid const itemGuid = ObjectGuid::Create<HighGuid::Item>(1);
    ObjectGuid const needGuid = PlayerGuid(1);
    Roll* roll = new Roll(itemGuid, loot.quest_items[0]);
    roll->setLoot(&loot);
    roll->itemSlot = 0;
    roll->AddPlayerVote(needGuid, false, true);
    roll->AddPlayerVote(PlayerGuid(2), false, true);

    ScriptRegistry<MiscScript>::InitEnabledHooksIfNeeded(MISCHOOK_END);
    TestGroup group;
    group.AddRoll(roll);

    EXPECT_FALSE(group.CountRollVote(needGuid, itemGuid, ROLL_NEED));
    EXPECT_EQ(roll->playerVote[needGuid], NEED);
    EXPECT_EQ(roll->totalNeed, 1);
    EXPECT_FALSE(roll->IsComplete());
}

TEST(GroupLootRollTest, TimeoutThroughEndRollUnblocksLootAndRemovesRoll)
{
    Loot loot;
    loot.items.push_back(CreateLootItem());
    loot.items[0].is_blocked = true;
    ObjectGuid const itemGuid = ObjectGuid::Create<HighGuid::Item>(1);
    Roll* roll = new Roll(itemGuid, loot.items[0]);
    roll->setLoot(&loot);
    roll->itemSlot = 0;
    roll->AddPlayerVote(PlayerGuid(1), true, true);
    roll->AddPlayerVote(PlayerGuid(2), false, true);

    ScriptRegistry<MiscScript>::InitEnabledHooksIfNeeded(MISCHOOK_END);
    TestGroup group;
    group.AddRoll(roll);

    group.EndRoll(&loot);

    EXPECT_FALSE(loot.items[0].is_blocked);
    EXPECT_EQ(group.GetRollCount(), 0);
}
