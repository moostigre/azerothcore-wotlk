/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "GuardReinforcementMgr.h"
#include "gtest/gtest.h"

namespace
{
using Clock = std::chrono::steady_clock;

TEST(GuardReinforcementPostTest, EnforcesCooldownAndRechargesCharges)
{
    Clock::time_point start;
    GuardReinforcementPost post(68, start);

    EXPECT_TRUE(post.Reserve(start));
    EXPECT_FALSE(post.Reserve(start + std::chrono::seconds(9)));
    EXPECT_TRUE(post.Reserve(start + std::chrono::seconds(10)));
    for (uint8 charge = 2; charge < 10; ++charge)
        EXPECT_TRUE(post.Reserve(start + std::chrono::seconds(charge * 10)));
    EXPECT_EQ(post.GetCharges(), 1);
    EXPECT_TRUE(post.Reserve(start + std::chrono::seconds(100)));
    EXPECT_EQ(post.GetCharges(), 0);
    EXPECT_FALSE(post.Reserve(start + std::chrono::seconds(110)));
    EXPECT_TRUE(post.Reserve(start + std::chrono::seconds(120)));
}

TEST(GuardReinforcementPostTest, RefundRestoresChargeAndCooldown)
{
    Clock::time_point start;
    GuardReinforcementPost post(68, start);
    auto reservation = post.Reserve(start);

    ASSERT_TRUE(reservation);
    post.Refund(*reservation);
    EXPECT_EQ(post.GetCharges(), 10);
    EXPECT_TRUE(post.Reserve(start));
}

TEST(GuardReinforcementTrackerTest, RemovesCallerAndGuardRelationships)
{
    ObjectGuid caller(uint64(1));
    ObjectGuid guard(uint64(2));
    GuardReinforcementTracker tracker;

    tracker.Track(caller, guard);
    EXPECT_EQ(tracker.GetGuard(caller), guard);
    tracker.Remove(guard);
    EXPECT_FALSE(tracker.GetGuard(caller));
    tracker.Track(caller, guard);
    tracker.Remove(caller);
    EXPECT_FALSE(tracker.GetGuard(caller));
}
}
