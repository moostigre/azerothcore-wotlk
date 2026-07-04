/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Cell.h"
#include "CellImpl.h"
#include "AllSpellScript.h"
#include "CreatureScript.h"
#include "CharmInfo.h"
#include "GameObjectScript.h"
#include "GameTime.h"
#include "GridNotifiers.h"
#include "DBCStores.h"
#include "Player.h"
#include "PlayerScript.h"
#include "ReputationMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "StringFormat.h"
#include "WorldPacket.h"
#include "WorldStateDefines.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/// @todo: this import is not necessary for compilation and marked as unused by the IDE
//  however, for some reasons removing it would cause a damn linking issue
//  there is probably some underlying problem with imports which should properly addressed
//  see: https://github.com/azerothcore/azerothcore-wotlk/issues/9766
#include "GridNotifiersImpl.h"

enum ShartuulTransporter
{
    NPC_SHARTUUL_EVENT_CONTROLLER       = 23059,
    NPC_FELGUARD_DEGRADER              = 23055,
    NPC_DOOMGUARD_PUNISHER             = 23113,
    NPC_SHARTUUL_SHIVAN_ASSASSIN       = 23220,
    NPC_FEL_IMP_DEFENDER               = 23078,
    NPC_FELHOUND_DEFENDER              = 23173,
    NPC_SHARTUUL_GANARG_UNDERLING      = 19398,
    NPC_SHARTUUL_MOARG_TORMENTER       = 23212,
    NPC_SHARTUUL_PORTABLE_FEL_CANNON   = 23278,
    NPC_EYE_OF_SHARTUUL_TRANSFORM      = 23227,
    NPC_EYE_OF_SHARTUUL                = 23228,
    NPC_DREADMAW                       = 23275,
    NPC_EREDAR_SHARTUUL                = 23230,
    NPC_EYE_TENTACLE                   = 15726,
    NPC_AETHER_RAY                     = 22181,
    NPC_SHARTUUL_WAVE_FELHOUND         = 6010,
    NPC_SHARTUUL_WAVE_IMP              = 21135,
    NPC_LEGION_RING_INFERNAL           = 23075,
    NPC_LEGION_RING_INVISMAN_LG        = 23260,
    NPC_LEGION_RING_STUN_ROPE_DUMMY    = 23313,
    NPC_WORLD_EVENT_GENERATOR          = 12434,
    NPC_SHARTUUL_BOUNDARY_ANCHOR       = NPC_WORLD_EVENT_GENERATOR,
    MODEL_INVISIBLE                    = 11686,
    MODEL_SHARTUUL_GANARG_UNDERLING    = 18288,
    MODEL_SHARTUUL_MOARG_TORMENTER     = 19899,
    MODEL_SHARTUUL_PORTABLE_FEL_CANNON = 18820,
    MODEL_EYE_OF_SHARTUUL              = 19579,
    MODEL_DREADMAW                     = 20854,
    MODEL_EREDAR_SHARTUUL              = 20975,

    GO_SHARTUUL_LEGION_RING_OBELISK    = 185588,
    GO_SHARTUUL_LEGION_RING_PORTAL     = 185587,
    GO_SHARTUUL_LEGION_RING_FOG        = 185593,

    SPELL_CRYSTALFORGED_DARKRUNE       = 40309,
    SPELL_POSSESS                      = 530,
    SPELL_SMASH_SHIELD                 = 40222,
    SPELL_SHARTUUL_VISUAL_SHELL_SHIELD = 40158,
    SPELL_SHARTUUL_GREEN_LIGHTNING     = 40057,
    SPELL_SHARTUUL_GREEN_LIGHTNING_THIN = 40146,
    SPELL_SHARTUUL_BOUNDARY_TEST_40071 = 40071,
    SPELL_SHARTUUL_BOUNDARY_DRAIN_LIFE = 689,
    SPELL_SHARTUUL_SHIELD_HIT_VISUAL   = 7750,
    SPELL_SHARTUUL_RING_DAMAGE         = 40950,
    SPELL_SHARTUUL_IMP_FIREBOLT        = 27267,
    SPELL_SHARTUUL_DOOMGUARD_PUNISHING_BLOW = 40560,
    SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES = 40561,
    SPELL_SHARTUUL_DOOMGUARD_THROW_AXE = 40563,
    SPELL_SHARTUUL_DOOMGUARD_SUPER_JUMP = 40493,
    SPELL_SHARTUUL_DOOMGUARD_SUPER_JUMP_IMPACT = 40262,
    SPELL_SHARTUUL_MOARG_ACID_GEYSER_CASTBAR = 44431,
    SPELL_SHARTUUL_MOARG_ACID_GEYSER_VISUAL = 37140,
    SPELL_SHARTUUL_FEL_CANNON_BLAST    = 36238,
    SPELL_SHARTUUL_FEL_CANNON_BOLT     = 40068,
    SPELL_SHARTUUL_FEL_CANNON_TRACER   = 41599,
    SPELL_SHARTUUL_WAR_STOMP_VISUAL    = 33707,
    SPELL_SHARTUUL_STUN_VISUAL         = 24394,
    SPELL_SHARTUUL_EYE_DARK_GLARE      = 42011,
    SPELL_SHARTUUL_EYE_DISRUPTION      = 37061,
    SPELL_SHARTUUL_SHADOW_RESONANCE    = 41048,
    SPELL_SHARTUUL_DREADMAW_GROWTH     = 24086,
    SPELL_SHARTUUL_EREDAR_INCINERATE   = 19396,
    SPELL_SHARTUUL_GENERIC_SHADOW_BOLT = 9613,
    SPELL_SHARTUUL_GENERIC_FIREBALL    = 133,
    SPELL_SHARTUUL_GENERIC_GLARE       = 26105,
    SPELL_SHARTUUL_GENERIC_MIND_FLAY   = 28310,
    SPELL_SHARTUUL_GENERIC_IMMOLATE    = 12742,
    SPELL_SHARTUUL_GENERIC_CHARGE      = 22911,
    SPELL_SHARTUUL_GENERIC_STUN        = 24394,
    SPELL_SHARTUUL_SHIVAN_DEATH_BLAST  = 40736,
    SPELL_SHARTUUL_SHIVAN_SIPHON_LIFE  = 41597,
    SPELL_SHARTUUL_SHIVAN_SHADOW_NOVA  = 40737,
    SPELL_SHARTUUL_SHIVAN_ASPECT_FLAME = 41593,
    SPELL_SHARTUUL_SHIVAN_ASPECT_ICE   = 41594,
    SPELL_SHARTUUL_SHIVAN_ASPECT_SHADOW = 41595,
    SPELL_SHARTUUL_SHIVAN_PYROBLAST    = 41578,
    SPELL_SHARTUUL_SHIVAN_FLAME_BUFFET = 41596,
    SPELL_SHARTUUL_SHIVAN_CLEANSING_FLAME = 41589,
    SPELL_SHARTUUL_SHIVAN_ICEBLAST     = 41579,
    SPELL_SHARTUUL_SHIVAN_ICY_LEAP     = 40727,
    SPELL_SHARTUUL_SHIVAN_ICE_BLOCK    = 41590,
    SPELL_SHARTUUL_SHIVAN_CHAOS_STRIKE = 40741,
    SPELL_SHARTUUL_OPEN_PORTAL_VISUAL  = 45977,
    SPELL_SHARTUUL_MURU_OPEN_PORTAL_PERIODIC = 45994,
    SPELL_SHARTUUL_MURU_OPEN_PORTAL    = 45977,
    SPELL_SHARTUUL_MURU_SUMMON_FLOOR   = 45978,
    SPELL_SHARTUUL_MURU_SUMMON_VISUAL  = 45989,
    SPELL_SHARTUUL_MURU_TRANSFORM_MISSILE_A = 46178,
    SPELL_SHARTUUL_MURU_TRANSFORM_MISSILE_B = 46208,
    SPELL_SHARTUUL_SELF_VISUAL         = 37816,
    SPELL_SHARTUUL_DREADMAW_SUMMON_CHANNEL = 40646,
    SPELL_SHARTUUL_DREADMAW_INTRO      = 40648,
    SPELL_SHARTUUL_LEGION_RING_CHANNEL = 40605,
    SPELL_SHARTUUL_DREADMAW_AURA       = 42173,

    ACTION_SHARTUUL_START_PHASE_ONE    = 1,
    ACTION_SHARTUUL_RESET              = 2,
    ACTION_SHARTUUL_DOOMGUARD_SUPER_JUMP = 230552,
    ACTION_SHARTUUL_DOOMGUARD_FEL_FLAMES = 230555,
    ACTION_SHARTUUL_MOARG_ACID_GEYSER = 230557,
    ACTION_SHARTUUL_TEST_EYE_INTRO    = 230560,

    DATA_SHARTUUL_PLAYER               = 1,
    DATA_SHARTUUL_DEGRADER             = 2,
    DATA_SHARTUUL_DOOMGUARD_SUPER_JUMP_TARGET = 230551,
    DATA_SHARTUUL_EVENT_STUN_TIMER     = 230553,
    DATA_SHARTUUL_DOOMGUARD_FEL_FLAMES_TARGET = 230554,
    DATA_SHARTUUL_MOARG_ACID_GEYSER_TARGET = 230556,
    DATA_SHARTUUL_TEST_HARNESS_CREATURE = 230558,
    DATA_SHARTUUL_PHASE_THREE_SUMMONER = 230559,

    EVENT_SHARTUUL_WAVE_ONE            = 1,
    EVENT_SHARTUUL_BOUNDARY_CHECK      = 2,
    EVENT_SHARTUUL_RESPAWN_IDLE        = 3,
    EVENT_SHARTUUL_IDLE_PATROL         = 4,
    EVENT_SHARTUUL_ARENA_VISUALS       = 5,
    EVENT_SHARTUUL_IMP_FIREBOLT        = 6,
    EVENT_SHARTUUL_NEXT_WAVE           = 7,
    EVENT_SHARTUUL_DOOMGUARD_FEL_FLAMES = 8,
    EVENT_SHARTUUL_DOOMGUARD_PUNISHING_BLOW = 9,
    EVENT_SHARTUUL_DOOMGUARD_FEL_FLAMES_TICK = 10,
    EVENT_SHARTUUL_DOOMGUARD_PHASE_NEXT_WAVE = 11,
    EVENT_SHARTUUL_GANARG_CANNON_CHECK = 12,
    EVENT_SHARTUUL_GANARG_CANNON_FINISH = 13,
    EVENT_SHARTUUL_CANNON_FIRE = 14,
    EVENT_SHARTUUL_MOARG_ACID_GEYSER = 15,
    EVENT_SHARTUUL_GANARG_MOARG_SHIELD_CHECK = 16,
    EVENT_SHARTUUL_GANARG_MOARG_SHIELD_FINISH = 17,
    EVENT_SHARTUUL_SUPER_JUMP_LAND = 18,
    EVENT_SHARTUUL_SUPER_JUMP_START = 19,
    EVENT_SHARTUUL_MOARG_ACID_GEYSER_FINISH = 20,
    EVENT_SHARTUUL_PHASE_THREE_START = 21,
    EVENT_SHARTUUL_PHASE_THREE_BOSS_ABILITY = 22,
    EVENT_SHARTUUL_PHASE_THREE_DARK_GLARE_HIT = 23,
    EVENT_SHARTUUL_PHASE_THREE_INCINERATE_HIT = 24,
    EVENT_SHARTUUL_PHASE_THREE_TENTACLES = 25,
    EVENT_SHARTUUL_TEST_BOSS_ABILITIES = 26,
    EVENT_SHARTUUL_PHASE_THREE_SHARTUUL_ACTIVATE = 27,
    EVENT_SHARTUUL_TEST_PORTAL_VISUALS = 28,
    EVENT_SHARTUUL_TEST_EYE_INTRO_PORTAL = 29,
    EVENT_SHARTUUL_TEST_EYE_INTRO_SHARTUUL = 30,
    EVENT_SHARTUUL_TEST_EYE_INTRO_YELL = 31,
    EVENT_SHARTUUL_TEST_EYE_INTRO_LAUGH = 32,
    EVENT_SHARTUUL_TEST_EYE_INTRO_EYE = 33,
    POINT_SHARTUUL_DOOMGUARD_SUPER_JUMP = 230550,

    SHARTUUL_CONTROLLER_SEARCH_RANGE   = 180,
    SHARTUUL_SHIELD_HELPER_COUNT       = 1,
    SHARTUUL_BOUNDARY_DIVIDERS_PER_SIDE = 3,
    SHARTUUL_BOUNDARY_POINTS_PER_SIDE  = SHARTUUL_BOUNDARY_DIVIDERS_PER_SIDE + 2,
    SHARTUUL_SHIELD_HAMMER_DAMAGE      = 15,
    SHARTUUL_PHASE_ONE_MAX_WAVE        = 6,
    SHARTUUL_PHASE_ONE_WAVE_INTERVAL_MS = 20000,
    SHARTUUL_DOOMGUARD_FEL_FLAMES_TICKS = 5,
    SHARTUUL_DOOMGUARD_PHASE_MAX_WAVE = 7,
    FACTION_OGRILA                     = 1038
};

float constexpr ShartuulBoundaryBeamZOffset = 1.0f;
float constexpr ShartuulArenaBoundaryInset = 5.0f;

struct ShartuulPendingSuperJumpImpact
{
    ObjectGuid DoomguardGuid;
    ObjectGuid TargetGuid;
    uint32 Timer = 0;
};

struct ShartuulPendingSuperJumpRequest
{
    ObjectGuid DoomguardGuid;
    ObjectGuid TargetGuid;
};

struct ShartuulPendingFelFlames
{
    ObjectGuid DoomguardGuid;
    ObjectGuid TargetGuid;
    ObjectGuid HelperGuid;
    uint32 DurationTimer = 0;
    uint32 TickTimer = 0;
    uint32 VisualTimer = 0;
    uint8 TicksDone = 0;
};

static std::unordered_map<ObjectGuid, ShartuulPendingSuperJumpRequest> ShartuulPendingSuperJumpRequests;
static std::unordered_map<ObjectGuid, ShartuulPendingSuperJumpImpact> ShartuulPendingSuperJumpImpacts;
static std::unordered_map<ObjectGuid, ShartuulPendingFelFlames> ShartuulPendingFelFlamesCasts;
static std::unordered_set<ObjectGuid> ShartuulNativeSuperJumpCasters;
static std::unordered_set<ObjectGuid> ShartuulFelFlamesVisualCasters;

uint32 constexpr FactionShartuulIdle = 35;
uint32 constexpr FactionShartuulControlled = 2237;
uint32 constexpr FactionShartuulWaveDemon = 2238;

Position const ShartuulCenter = { 2693.44f, 7110.67f, 365.10f, 0.37965f };
Position const ShartuulDegraderSpawn = { 2690.4033f, 7089.1978f, 364.6187f, -2.9844513f };
Position const ShartuulCaptiveDemonSpawn = { 2720.119f, 7117.3936f, 367.30975f, 4.26541f };
Position const ShartuulShieldCenter = { 2720.2156f, 7118.0107f, 371.19757f, 4.2377276f };
Position const ShartuulGateCrystal = { 2720.3293f, 7117.7560f, 379.24738f, 3.9606972f };
float constexpr ShartuulDegraderLeashRange = 115.0f;
Position const ShartuulWaveOneSpawn[] =
{
    { 2696.257f, 7098.310f, 365.212f, 0.739891f },
    { 2758.402f, 7144.602f, 365.185f, 3.800588f }
};
Position const ShartuulWaveSpawn[] =
{
    { 2696.257f, 7098.310f, 365.212f, 0.739891f },
    { 2758.402f, 7144.602f, 365.185f, 3.800588f },
    { 2712.799f, 7143.090f, 365.721f, 2.388301f },
    { 2734.587f, 7136.870f, 365.167f, 3.431647f }
};
Position const ShartuulDoomguardWaveSpawn[] =
{
    { 2674.62f, 7092.54f, 365.13f, 0.58f },
    { 2702.31f, 7162.18f, 365.58f, 4.82f },
    { 2765.92f, 7134.46f, 365.21f, 3.59f },
    { 2739.42f, 7066.31f, 365.39f, 2.40f },
    { 2691.06f, 7116.78f, 365.19f, 0.05f },
    { 2724.50f, 7138.42f, 365.23f, 3.87f },
    { 2747.90f, 7103.08f, 365.29f, 2.84f },
    { 2708.23f, 7080.12f, 365.19f, 1.25f }
};
Position const ShartuulMoargWaveSpawn[] =
{
    { 2742.65f, 7101.85f, 365.76f, 2.48f },
    { 2690.18f, 7131.31f, 365.19f, 5.82f },
    { 2748.87f, 7095.51f, 365.28f, 2.55f }
};
Position const ShartuulPhaseThreePortalSpawn = { 2648.9253f, 7153.0366f, 378.99683f, 5.8356433f };
Position const ShartuulPhaseThreeSummonerSpawn = { 2656.2249f, 7143.5090f, 365.08762f, 5.8933425f };
Position const ShartuulPhaseThreeBossSpawn = { 2720.119f, 7117.3936f, 367.30975f, 4.26541f };
Position const ShartuulPhaseThreeEyeSpawn = { 2683.7046f, 7133.1290f, 364.54816f, 5.8972664f };
Position const ShartuulTentacleSpawn[] =
{
    { 2664.0f, 7102.0f, 365.2f, 0.30f },
    { 2700.0f, 7160.0f, 365.7f, 4.70f },
    { 2767.0f, 7128.0f, 365.4f, 3.45f },
    { 2736.0f, 7067.0f, 365.6f, 2.20f }
};
Position const ShartuulTestPossessSpawn[] =
{
    { 2683.00f, 7069.00f, 365.10f, 1.05f },
    { 2690.00f, 7064.00f, 365.20f, 1.12f },
    { 2697.00f, 7060.00f, 365.30f, 1.22f }
};
Position const ShartuulTestWaveSpawn[] =
{
    { 2718.00f, 7092.00f, 365.10f, 2.20f },
    { 2725.00f, 7094.00f, 365.10f, 2.35f },
    { 2732.00f, 7096.00f, 365.10f, 2.55f },
    { 2742.65f, 7101.85f, 365.76f, 2.48f },
    { 2746.00f, 7102.00f, 365.40f, 2.95f },
    { 2683.7046f, 7133.1290f, 364.54816f, 5.8972664f },
    { 2690.00f, 7139.00f, 365.10f, 5.60f },
    { 2656.2249f, 7143.5090f, 365.08762f, 5.8933425f },
    { 2648.00f, 7136.00f, 365.10f, 5.70f }
};
Position const ShartuulTestPortalVisualSpawn[] =
{
    { 2658.00f, 7080.00f, 365.00f, 0.0f }, // 45994
    { 2664.00f, 7080.00f, 365.00f, 0.0f }, // 45977
    { 2670.00f, 7080.00f, 365.00f, 0.0f }, // 45978
    { 2676.00f, 7080.00f, 365.00f, 0.0f }, // 45989
    { 2682.00f, 7080.00f, 365.00f, 0.0f }, // 46178
    { 2688.00f, 7080.00f, 365.00f, 0.0f }  // 46208
};
uint32 const ShartuulTestPortalVisualSpells[] =
{
    SPELL_SHARTUUL_MURU_OPEN_PORTAL_PERIODIC,
    SPELL_SHARTUUL_MURU_OPEN_PORTAL,
    SPELL_SHARTUUL_MURU_SUMMON_FLOOR,
    SPELL_SHARTUUL_MURU_SUMMON_VISUAL,
    SPELL_SHARTUUL_MURU_TRANSFORM_MISSILE_A,
    SPELL_SHARTUUL_MURU_TRANSFORM_MISSILE_B
};
Position const ShartuulDegraderPatrol[] =
{
    { 2676.0222f, 7075.2988f, 364.6187f, -2.9844513f },
    { 2662.2496f, 7077.5854f, 365.13208f, 2.0481472f },
    { 2654.0441f, 7093.3918f, 364.64902f, 2.0481472f },
    { 2652.5465f, 7103.8216f, 364.54333f, 1.0371598f },
    { 2656.0668f, 7112.9140f, 364.7572f, 1.0371598f },
    { 2659.2871f, 7120.9921f, 364.89465f, 1.0371598f },
    { 2760.8708f, 7127.5840f, 365.44998f, 5.393791f },
    { 2760.9583f, 7113.5030f, 364.79028f, 4.453669f },
    { 2754.9020f, 7096.3670f, 365.9043f, 4.020129f },
    { 2743.0390f, 7083.7285f, 366.2397f, 3.7444544f },
    { 2729.0906f, 7078.0015f, 365.04428f, 3.1530492f }
};
Position const ShartuulRingRopeDummySpawn[] =
{
    { 2642.4958f, 7090.0107f, 365.18173f, 3.4004683f },
    { 2699.4377f, 7182.3228f, 367.30870f, 5.0301676f },
    { 2797.3530f, 7137.7207f, 365.53452f, 2.3048348f },
    { 2740.6013f, 7048.5190f, 365.51410f, 4.7160090f }
};

static Position GetShartuulBoundaryBasePosition(uint8 side, uint8 point)
{
    Position const& from = ShartuulRingRopeDummySpawn[side];
    Position const& to = ShartuulRingRopeDummySpawn[(side + 1) % std::size(ShartuulRingRopeDummySpawn)];
    float const pct = std::clamp(float(point) / float(SHARTUUL_BOUNDARY_POINTS_PER_SIDE - 1), 0.0f, 1.0f);
    float const x = from.GetPositionX() + (to.GetPositionX() - from.GetPositionX()) * pct;
    float const y = from.GetPositionY() + (to.GetPositionY() - from.GetPositionY()) * pct;
    float const z = from.GetPositionZ() + (to.GetPositionZ() - from.GetPositionZ()) * pct;
    float const o = std::atan2(to.GetPositionY() - from.GetPositionY(), to.GetPositionX() - from.GetPositionX());
    return { x, y, z, o };
}

static Position GetShartuulBoundaryAnchorPosition(uint8 side, uint8 point)
{
    Position pos = GetShartuulBoundaryBasePosition(side, point);
    pos.m_positionZ += ShartuulBoundaryBeamZOffset;
    return pos;
}

static float Cross2D(float ax, float ay, float bx, float by)
{
    return ax * by - ay * bx;
}

static bool IsInsideShartuulBoundary(float x, float y)
{
    bool hasPositive = false;
    bool hasNegative = false;

    for (uint8 i = 0; i < std::size(ShartuulRingRopeDummySpawn); ++i)
    {
        Position const& a = ShartuulRingRopeDummySpawn[i];
        Position const& b = ShartuulRingRopeDummySpawn[(i + 1) % std::size(ShartuulRingRopeDummySpawn)];
        float const cross = Cross2D(b.GetPositionX() - a.GetPositionX(), b.GetPositionY() - a.GetPositionY(), x - a.GetPositionX(), y - a.GetPositionY());
        if (cross > 0.0f)
            hasPositive = true;
        else if (cross < 0.0f)
            hasNegative = true;

        if (hasPositive && hasNegative)
            return false;
    }

    return true;
}

static Position GetClosestPointInsideShartuulBoundary(WorldObject const* object, float inset)
{
    float const px = object->GetPositionX();
    float const py = object->GetPositionY();
    float bestDistSq = std::numeric_limits<float>::max();
    float bestX = ShartuulCenter.GetPositionX();
    float bestY = ShartuulCenter.GetPositionY();

    for (uint8 i = 0; i < std::size(ShartuulRingRopeDummySpawn); ++i)
    {
        Position const& a = ShartuulRingRopeDummySpawn[i];
        Position const& b = ShartuulRingRopeDummySpawn[(i + 1) % std::size(ShartuulRingRopeDummySpawn)];
        float const ax = a.GetPositionX();
        float const ay = a.GetPositionY();
        float const bx = b.GetPositionX();
        float const by = b.GetPositionY();
        float const dx = bx - ax;
        float const dy = by - ay;
        float const lenSq = dx * dx + dy * dy;
        if (lenSq <= 0.0f)
            continue;

        float const t = std::clamp(((px - ax) * dx + (py - ay) * dy) / lenSq, 0.0f, 1.0f);
        float candidateX = ax + dx * t;
        float candidateY = ay + dy * t;
        float toCenterX = ShartuulCenter.GetPositionX() - candidateX;
        float toCenterY = ShartuulCenter.GetPositionY() - candidateY;
        float const toCenterLen = std::sqrt(toCenterX * toCenterX + toCenterY * toCenterY);
        if (toCenterLen > 0.0f)
        {
            candidateX += (toCenterX / toCenterLen) * inset;
            candidateY += (toCenterY / toCenterLen) * inset;
        }

        float const distSq = (px - candidateX) * (px - candidateX) + (py - candidateY) * (py - candidateY);
        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            bestX = candidateX;
            bestY = candidateY;
        }
    }

    float z = ShartuulCenter.GetPositionZ();
    if (Map* map = object->GetMap())
        z = map->GetHeight(object->GetPhaseMask(), bestX, bestY, object->GetPositionZ() + 5.0f);

    return { bestX, bestY, z, object->GetOrientation() };
}

static Creature* GetShartuulController(WorldObject* searcher)
{
    return searcher ? GetClosestCreatureWithEntry(searcher, NPC_SHARTUUL_EVENT_CONTROLLER, SHARTUUL_CONTROLLER_SEARCH_RANGE) : nullptr;
}

static void SendShartuulCastBar(Unit* caster, Unit* target, uint32 spellId, uint32 durationMs)
{
    if (!caster || !target)
        return;

    SpellCastTargets targets;
    targets.SetUnitTarget(target);

    WorldPacket data(SMSG_SPELL_START, 32);
    data << caster->GetPackGUID();
    data << caster->GetPackGUID();
    data << uint8(0);
    data << uint32(spellId);
    data << uint32(CAST_FLAG_NONE);
    data << int32(durationMs);
    targets.Write(data);
    caster->SendMessageToSet(&data, true);
}

static bool IsShartuulWaveDemon(Unit* unit)
{
    if (!unit)
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_FELHOUND_DEFENDER
        || entry == NPC_FEL_IMP_DEFENDER
        || entry == NPC_SHARTUUL_WAVE_FELHOUND
        || entry == NPC_SHARTUUL_WAVE_IMP
        || entry == NPC_DOOMGUARD_PUNISHER
        || entry == NPC_SHARTUUL_GANARG_UNDERLING
        || entry == NPC_SHARTUUL_MOARG_TORMENTER
        || entry == NPC_SHARTUUL_PORTABLE_FEL_CANNON
        || entry == NPC_SHARTUUL_SHIVAN_ASSASSIN
        || entry == NPC_EYE_OF_SHARTUUL_TRANSFORM
        || entry == NPC_EYE_OF_SHARTUUL
        || entry == NPC_DREADMAW
        || entry == NPC_EREDAR_SHARTUUL
        || entry == NPC_EYE_TENTACLE;
}

static bool IsShartuulProtectedAmbient(Unit const* unit)
{
    return unit && unit->GetEntry() == NPC_AETHER_RAY;
}

static void ApplySniffedPhaseThreeCreatureState(Creature* creature)
{
    if (!creature)
        return;

    creature->SetPhaseMask(PHASEMASK_NORMAL, true);
    creature->SetFaction(FactionShartuulWaveDemon);
    creature->SetRegeneratingHealth(false);
    creature->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE);

    switch (creature->GetEntry())
    {
        case NPC_EYE_OF_SHARTUUL_TRANSFORM:
        case NPC_EYE_OF_SHARTUUL:
            creature->SetDisplayId(MODEL_EYE_OF_SHARTUUL);
            creature->SetNativeDisplayId(MODEL_EYE_OF_SHARTUUL);
            creature->SetCanFly(true);
            creature->SetDisableGravity(true);
            creature->SetHover(true);
            creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, 1772);
            break;
        case NPC_DREADMAW:
            creature->SetDisplayId(MODEL_DREADMAW);
            creature->SetNativeDisplayId(MODEL_DREADMAW);
            creature->SetCanFly(false);
            creature->SetDisableGravity(false);
            creature->SetHover(false);
            creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
            break;
        case NPC_EREDAR_SHARTUUL:
            creature->SetDisplayId(MODEL_EREDAR_SHARTUUL);
            creature->SetNativeDisplayId(MODEL_EREDAR_SHARTUUL);
            creature->SetCanFly(false);
            creature->SetDisableGravity(false);
            creature->SetHover(false);
            creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
            break;
        default:
            break;
    }
}

static bool IsPossessedShartuulControlledDemon(Unit const* unit)
{
    if (!unit || !unit->IsAlive() || !unit->isPossessed())
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_FELGUARD_DEGRADER
        || entry == NPC_DOOMGUARD_PUNISHER
        || entry == NPC_SHARTUUL_SHIVAN_ASSASSIN;
}

static std::vector<Creature*> GetDoomguardFelFlamesTargets(Unit* doomguard)
{
    std::vector<Creature*> targets;
    if (!doomguard)
        return targets;

    uint32 const entries[] =
    {
        NPC_SHARTUUL_GANARG_UNDERLING,
        NPC_SHARTUUL_MOARG_TORMENTER,
        NPC_SHARTUUL_PORTABLE_FEL_CANNON,
        NPC_SHARTUUL_SHIVAN_ASSASSIN,
        NPC_SHARTUUL_WAVE_FELHOUND,
        NPC_SHARTUUL_WAVE_IMP,
        NPC_FELHOUND_DEFENDER,
        NPC_FEL_IMP_DEFENDER
    };

    for (uint32 entry : entries)
    {
        std::list<Creature*> creatures;
        GetCreatureListWithEntryInGrid(creatures, doomguard, entry, 18.0f);
        for (Creature* creature : creatures)
        {
            if (!creature || !creature->IsAlive() || IsShartuulProtectedAmbient(creature))
                continue;

            if (!doomguard->isInFront(creature, float(M_PI) / 2.0f))
                continue;

            targets.push_back(creature);
        }
    }

    return targets;
}

void ShartuulHandleDoomguardFelFlames(Unit* doomguard, Unit* target)
{
    if (!doomguard || !doomguard->IsAlive())
    {
        LOG_INFO("scripts", "Shartuul Fel Flames helper rejected: doomguard={} target={}", doomguard ? doomguard->GetGUID().ToString() : "none", target ? target->GetGUID().ToString() : "none");
        return;
    }

    std::vector<Creature*> targets = GetDoomguardFelFlamesTargets(doomguard);
    if (target && target->IsAlive() && IsShartuulWaveDemon(target) && !IsShartuulProtectedAmbient(target))
        if (Creature* creatureTarget = target->ToCreature())
            if (std::find(targets.begin(), targets.end(), creatureTarget) == targets.end())
                targets.push_back(creatureTarget);

    LOG_INFO("scripts", "Shartuul Fel Flames helper start: doomguard={} targetCount={}", doomguard->GetGUID().ToString(), targets.size());

    doomguard->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, doomguard->GetGUID());
    doomguard->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES);
    doomguard->ClearUnitState(UNIT_STATE_CASTING);

    Creature* helper = nullptr;
    if (Creature* creature = doomguard->ToCreature())
    {
        helper = creature->SummonCreature(NPC_WORLD_EVENT_GENERATOR, doomguard->GetPositionX(), doomguard->GetPositionY(), doomguard->GetPositionZ() + 1.0f, doomguard->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 6000);
        if (helper)
        {
            helper->SetDisplayId(MODEL_INVISIBLE);
            helper->SetReactState(REACT_PASSIVE);
            helper->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            helper->SetFacingToObject(doomguard);
            helper->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, doomguard->GetGUID());
            helper->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES);

            ShartuulFelFlamesVisualCasters.insert(helper->GetGUID());
            helper->CastSpell(doomguard, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES, false);
        }
    }

    ShartuulFelFlamesVisualCasters.insert(doomguard->GetGUID());
    doomguard->CastSpell(doomguard, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES, false);
    doomguard->ClearUnitState(UNIT_STATE_CASTING);

    ObjectGuid doomguardGuid = doomguard->GetGUID();
    ObjectGuid helperGuid = helper ? helper->GetGUID() : ObjectGuid::Empty;
    for (Creature* victim : targets)
        Unit::DealDamage(doomguard, victim, 1000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE);
    LOG_INFO("scripts", "Shartuul Fel Flames helper tick: doomguard={} targets={} tick=1", doomguard->GetGUID().ToString(), targets.size());

    for (uint8 i = 1; i < SHARTUUL_DOOMGUARD_FEL_FLAMES_TICKS; ++i)
    {
        doomguard->m_Events.AddEventAtOffset([doomguard, doomguardGuid, helperGuid]
        {
            Unit* owner = ObjectAccessor::GetUnit(*doomguard, doomguardGuid);
            if (!owner || !owner->IsAlive())
                return;

            owner->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, owner->GetGUID());
            owner->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES);
            owner->ClearUnitState(UNIT_STATE_CASTING);

            if (Creature* visual = ObjectAccessor::GetCreature(*owner, helperGuid))
            {
                visual->NearTeleportTo(owner->GetPositionX(), owner->GetPositionY(), owner->GetPositionZ() + 1.0f, owner->GetOrientation());
                visual->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, owner->GetGUID());
                visual->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES);
            }

            std::vector<Creature*> victims = GetDoomguardFelFlamesTargets(owner);
            for (Creature* victim : victims)
                Unit::DealDamage(owner, victim, 1000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE);
            LOG_INFO("scripts", "Shartuul Fel Flames helper tick: doomguard={} targets={}", owner->GetGUID().ToString(), victims.size());
        }, Milliseconds(1000 * i));
    }

    doomguard->m_Events.AddEventAtOffset([doomguard, doomguardGuid, helperGuid]
    {
        Unit* owner = ObjectAccessor::GetUnit(*doomguard, doomguardGuid);
        if (!owner)
            return;

        owner->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, ObjectGuid::Empty);
        owner->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
        ShartuulFelFlamesVisualCasters.erase(owner->GetGUID());

        if (Creature* visual = ObjectAccessor::GetCreature(*owner, helperGuid))
        {
            ShartuulFelFlamesVisualCasters.erase(visual->GetGUID());
            visual->DespawnOrUnsummon();
        }
    }, Milliseconds(5500));
}

struct ShartuulWaveCreatureAI : public ScriptedAI
{
    ShartuulWaveCreatureAI(Creature* creature) : ScriptedAI(creature) { }

    ObjectGuid targetGuid;
    uint32 eventStunTimer = 0;
    uint32 eventStunHealth = 0;
    bool testHarnessCreature = false;
    bool phaseThreeSummoner = false;

    void Reset() override
    {
        if (!phaseThreeSummoner)
            targetGuid.Clear();

        eventStunTimer = 0;
        eventStunHealth = 0;
        me->SetFaction(FactionShartuulWaveDemon);
        me->SetReactState(phaseThreeSummoner ? REACT_PASSIVE : REACT_AGGRESSIVE);
        me->RemoveUnitFlag(UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_STUNNED);
        if (!phaseThreeSummoner)
            me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
        me->SetControlled(false, UNIT_STATE_ROOT);
        me->SetControlled(false, UNIT_STATE_STUNNED);
        me->ClearUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_CASTING | UNIT_STATE_EVADE);
    }

    void SetGUID(ObjectGuid const& guid, int32 id) override
    {
        if (id == DATA_SHARTUUL_DEGRADER)
            targetGuid = guid;
    }

    void SetData(uint32 id, uint32 value) override
    {
        if (id == DATA_SHARTUUL_TEST_HARNESS_CREATURE)
        {
            testHarnessCreature = value != 0;
            me->SetReactState(testHarnessCreature ? REACT_DEFENSIVE : REACT_AGGRESSIVE);
            return;
        }

        if (id == DATA_SHARTUUL_PHASE_THREE_SUMMONER)
        {
            phaseThreeSummoner = value != 0;
            if (phaseThreeSummoner)
            {
                targetGuid.Clear();
                me->AttackStop();
                me->CombatStop(true);
                me->SetReactState(REACT_PASSIVE);
                me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MoveIdle();
            }
            else
            {
                me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
                me->SetReactState(REACT_AGGRESSIVE);
            }
            return;
        }

        if (id != DATA_SHARTUUL_EVENT_STUN_TIMER)
            return;

        eventStunTimer = std::max(eventStunTimer, value);
        eventStunHealth = me->GetHealth();
        ApplyEventStunVisual();
    }

    bool CanAIAttack(Unit const* target) const override
    {
        if (testHarnessCreature && !me->IsInCombat())
            return false;

        if (phaseThreeSummoner)
            return false;

        return IsPossessedShartuulControlledDemon(target);
    }

    Unit* GetEventTarget() const
    {
        if (phaseThreeSummoner)
            return nullptr;

        if (!targetGuid.IsEmpty())
            if (Unit* target = ObjectAccessor::GetUnit(*me, targetGuid))
                if (IsPossessedShartuulControlledDemon(target))
                    return target;

        if (testHarnessCreature)
            return nullptr;

        std::list<Creature*> demons;
        GetCreatureListWithEntryInGrid(demons, me, NPC_FELGUARD_DEGRADER, 120.0f);
        GetCreatureListWithEntryInGrid(demons, me, NPC_DOOMGUARD_PUNISHER, 120.0f);
        GetCreatureListWithEntryInGrid(demons, me, NPC_SHARTUUL_SHIVAN_ASSASSIN, 120.0f);
        for (Creature* demon : demons)
            if (IsPossessedShartuulControlledDemon(demon))
                return demon;

        return nullptr;
    }

    void EngageEventTarget(Unit* target)
    {
        if (!target || !target->IsAlive())
            return;

        if (phaseThreeSummoner)
            return;

        targetGuid = target->GetGUID();
        me->SetFaction(FactionShartuulWaveDemon);
        me->SetReactState(REACT_AGGRESSIVE);
        me->SetInCombatWith(target);
        target->SetInCombatWith(me);
        me->AddThreat(target, 100000.0f);
        AttackStart(target);
    }

    void ApplyEventStunVisual()
    {
        me->InterruptNonMeleeSpells(false);
        me->StopMoving();
        me->GetMotionMaster()->Clear();
        me->SetReactState(REACT_AGGRESSIVE);
        if (Unit* target = GetEventTarget())
        {
            me->SetInCombatWith(target);
            target->SetInCombatWith(me);
            me->AddThreat(target, 100000.0f);
        }
        me->SetControlled(true, UNIT_STATE_ROOT);
        me->SetControlled(true, UNIT_STATE_STUNNED);
        me->SetUnitFlag(UNIT_FLAG_STUNNED);
        me->AddAura(SPELL_SHARTUUL_STUN_VISUAL, me);
    }

    bool UpdateEventStun(uint32 diff)
    {
        if (!eventStunTimer)
            return false;

        if (eventStunHealth && me->GetHealth() > eventStunHealth)
            me->SetHealth(eventStunHealth);

        if (eventStunTimer > diff)
        {
            eventStunTimer -= diff;
            return true;
        }

        eventStunTimer = 0;
        eventStunHealth = 0;
        me->SetControlled(false, UNIT_STATE_ROOT);
        me->SetControlled(false, UNIT_STATE_STUNNED);
        me->RemoveUnitFlag(UNIT_FLAG_STUNNED);
        me->ClearUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_CASTING);
        me->RemoveAurasDueToSpell(SPELL_SHARTUUL_WAR_STOMP_VISUAL);
        me->RemoveAurasDueToSpell(SPELL_SHARTUUL_STUN_VISUAL);
        me->RemoveAurasDueToSpell(25900);
        me->SetReactState(REACT_AGGRESSIVE);
        me->GetMotionMaster()->Clear();

        if (Unit* target = GetEventTarget())
            EngageEventTarget(target);

        return false;
    }

    void DamageTaken(Unit* attacker, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (!testHarnessCreature || !IsPossessedShartuulControlledDemon(attacker))
            return;

        targetGuid = attacker->GetGUID();
        me->SetReactState(REACT_AGGRESSIVE);
        me->SetInCombatWith(attacker);
        attacker->SetInCombatWith(me);
        me->AddThreat(attacker, 100000.0f);
    }

    bool PrepareEventCombatUpdate(uint32 diff)
    {
        if (UpdateEventStun(diff))
            return false;

        Unit* target = GetEventTarget();
        if (!target)
        {
            me->AttackStop();
            me->CombatStop(true);
            return false;
        }

        if (!me->GetVictim())
            EngageEventTarget(target);

        return true;
    }
};

static void ApplyShartuulEventStun(Creature* creature, uint32 duration)
{
    if (!creature || !creature->IsAlive() || !IsShartuulWaveDemon(creature) || IsShartuulProtectedAmbient(creature))
        return;

    creature->AI()->SetData(DATA_SHARTUUL_EVENT_STUN_TIMER, duration);
}

static void PrepareShartuulTestPossessDemon(Creature* creature)
{
    if (!creature)
        return;

    creature->SetPhaseMask(PHASEMASK_NORMAL, true);
    creature->SetFaction(FactionShartuulIdle);
    creature->SetReactState(REACT_PASSIVE);
    creature->SetFullHealth();
    creature->CombatStop(true);
    creature->RemoveCharmAuras();
    creature->SetControlled(false, UNIT_STATE_ROOT);
    creature->ClearUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING);
    creature->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
    creature->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
    creature->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
    creature->GetMotionMaster()->Clear();
}

enum class ShartuulShivanAspect : uint8
{
    Shadow,
    Flame,
    Ice
};

static std::unordered_map<ObjectGuid, ShartuulShivanAspect> ShartuulShivanAspectState;
static std::unordered_map<ObjectGuid, uint32> ShartuulShivanIceBlockEndTime;

static ShartuulShivanAspect GetShartuulShivanAspect(Unit const* shivan)
{
    if (!shivan)
        return ShartuulShivanAspect::Shadow;

    auto itr = ShartuulShivanAspectState.find(shivan->GetGUID());
    return itr != ShartuulShivanAspectState.end() ? itr->second : ShartuulShivanAspect::Shadow;
}

static bool IsShartuulShivanIceBlocked(Unit const* shivan)
{
    if (!shivan)
        return false;

    auto itr = ShartuulShivanIceBlockEndTime.find(shivan->GetGUID());
    return itr != ShartuulShivanIceBlockEndTime.end() && itr->second > GameTime::GetGameTimeMS().count();
}

static void SetShartuulShivanSpells(Creature* shivan, ShartuulShivanAspect aspect, bool resendBar = true)
{
    if (!shivan || shivan->GetEntry() != NPC_SHARTUUL_SHIVAN_ASSASSIN)
        return;

    ShartuulShivanAspectState[shivan->GetGUID()] = aspect;

    uint32 spell0 = 0;
    uint32 spell1 = 0;
    uint32 spell2 = 0;
    uint32 aspect0 = 0;
    uint32 aspect1 = 0;

    switch (aspect)
    {
        case ShartuulShivanAspect::Shadow:
            spell0 = SPELL_SHARTUUL_SHIVAN_DEATH_BLAST;
            spell1 = SPELL_SHARTUUL_SHIVAN_SIPHON_LIFE;
            spell2 = SPELL_SHARTUUL_SHIVAN_SHADOW_NOVA;
            aspect0 = SPELL_SHARTUUL_SHIVAN_ASPECT_FLAME;
            aspect1 = SPELL_SHARTUUL_SHIVAN_ASPECT_ICE;
            break;
        case ShartuulShivanAspect::Flame:
            spell0 = SPELL_SHARTUUL_SHIVAN_PYROBLAST;
            spell1 = SPELL_SHARTUUL_SHIVAN_FLAME_BUFFET;
            spell2 = SPELL_SHARTUUL_SHIVAN_CLEANSING_FLAME;
            aspect0 = SPELL_SHARTUUL_SHIVAN_ASPECT_SHADOW;
            aspect1 = SPELL_SHARTUUL_SHIVAN_ASPECT_ICE;
            break;
        case ShartuulShivanAspect::Ice:
            spell0 = SPELL_SHARTUUL_SHIVAN_ICEBLAST;
            spell1 = SPELL_SHARTUUL_SHIVAN_ICY_LEAP;
            spell2 = SPELL_SHARTUUL_SHIVAN_ICE_BLOCK;
            aspect0 = SPELL_SHARTUUL_SHIVAN_ASPECT_FLAME;
            aspect1 = SPELL_SHARTUUL_SHIVAN_ASPECT_SHADOW;
            break;
    }

    uint32 const actionBar[10] =
    {
        spell0,
        spell1,
        spell2,
        0,
        0,
        aspect0,
        aspect1,
        0,
        0,
        SPELL_SHARTUUL_SHIVAN_CHAOS_STRIKE
    };

    for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
        shivan->m_spells[i] = actionBar[i];

    if (CharmInfo* charmInfo = shivan->GetCharmInfo())
        for (uint8 i = 0; i < 10; ++i)
            charmInfo->SetActionBar(i, actionBar[i], actionBar[i] ? ACT_PASSIVE : ACT_DISABLED);

    if (!resendBar)
        return;

    if (Player* player = ObjectAccessor::GetPlayer(*shivan, shivan->GetCharmerGUID()))
        player->PossessSpellInitialize();
}

static void PossessShartuulTestDemon(Player* player, Creature* creature)
{
    if (!player || !creature)
        return;

    player->RemoveCharmAuras();
    creature->RemoveCharmAuras();
    creature->CombatStop(true);
    creature->SetFaction(FactionShartuulControlled);
    creature->SetReactState(REACT_PASSIVE);
    creature->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
    player->CastSpell(creature, SPELL_POSSESS, true);
}

class npc_shartuul_felguard_degrader : public CreatureScript
{
public:
    npc_shartuul_felguard_degrader() : CreatureScript("npc_shartuul_felguard_degrader") { }

    struct npc_shartuul_felguard_degraderAI : public ScriptedAI
    {
        npc_shartuul_felguard_degraderAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetFaction(FactionShartuulIdle);
            me->SetReactState(REACT_PASSIVE);
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
            me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE);
        }

        void OnPossess(bool apply)
        {
            if (apply)
            {
                me->SetFaction(FactionShartuulControlled);
                me->SetReactState(REACT_PASSIVE);
                SetShartuulShivanSpells(me, ShartuulShivanAspect::Shadow);
                return;
            }

            if (Creature* controller = GetShartuulController(me))
                controller->AI()->DoAction(ACTION_SHARTUUL_RESET);
        }

        bool CanAIAttack(Unit const* target) const override
        {
            return !IsShartuulProtectedAmbient(target);
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
        {
            if (!me->isPossessed() || (IsShartuulWaveDemon(victim) && !IsShartuulProtectedAmbient(victim)))
                return;

            damage = 0;
            me->AttackStop();
            me->CombatStop(true);
        }

        void SpellHitTarget(Unit* target, SpellInfo const* /*spellInfo*/) override
        {
            if (!me->isPossessed() || (IsShartuulWaveDemon(target) && !IsShartuulProtectedAmbient(target)))
                return;

            me->AttackStop();
            me->CombatStop(true);
        }

        void UpdateAI(uint32 /*diff*/) override
        {
            if (!me->isPossessed())
                return;

            me->SetFaction(FactionShartuulControlled);

            if (!me->IsInCombat())
                return;

            Unit* victim = me->GetVictim();
            if (!IsShartuulWaveDemon(victim) || IsShartuulProtectedAmbient(victim))
            {
                me->AttackStop();
                me->CombatStop(true);
                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shartuul_felguard_degraderAI(creature);
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Possess this Felguard Degrader", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
        if (action == GOSSIP_ACTION_INFO_DEF + 1)
            PossessShartuulTestDemon(player, creature);

        CloseGossipMenuFor(player);
        return true;
    }
};

class npc_shartuul_doomguard_punisher : public CreatureScript
{
public:
    npc_shartuul_doomguard_punisher() : CreatureScript("npc_shartuul_doomguard_punisher") { }

    struct npc_shartuul_doomguard_punisherAI : public ScriptedAI
    {
        npc_shartuul_doomguard_punisherAI(Creature* creature) : ScriptedAI(creature) { }

        EventMap events;
        ObjectGuid superJumpTargetGuid;
        ObjectGuid felFlamesTargetGuid;
        ObjectGuid felFlamesHelperGuid;
        Position superJumpLandingPosition;
        uint32 felFlamesDurationTimer = 0;
        uint32 felFlamesTickTimer = 0;
        uint32 felFlamesVisualTimer = 0;
        uint8 felFlamesTicksDone = 0;

        void Reset() override
        {
            events.Reset();
            superJumpTargetGuid.Clear();
            felFlamesTargetGuid.Clear();
            felFlamesHelperGuid.Clear();
            felFlamesDurationTimer = 0;
            felFlamesTickTimer = 0;
            felFlamesVisualTimer = 0;
            felFlamesTicksDone = 0;
        }

        void OnPossess(bool apply)
        {
            if (apply)
            {
                me->SetFaction(FactionShartuulControlled);
                me->SetReactState(REACT_PASSIVE);
                return;
            }

            StopFelFlames();

            if (Creature* controller = GetShartuulController(me))
                controller->AI()->DoAction(ACTION_SHARTUUL_RESET);
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType damageType, SpellSchoolMask damageSchoolMask) override
        {
            if (me->isPossessed() && (!IsShartuulWaveDemon(victim) || IsShartuulProtectedAmbient(victim)))
            {
                damage = 0;
                me->AttackStop();
                me->CombatStop(true);
                return;
            }

            if (damageType == DIRECT_DAMAGE && damageSchoolMask == SPELL_SCHOOL_MASK_NORMAL)
                damage = std::min<uint32>(damage, urand(125, 250));
        }

        bool CanAIAttack(Unit const* target) const override
        {
            return !IsShartuulProtectedAmbient(target);
        }

        void SetGUID(ObjectGuid const& guid, int32 id) override
        {
            if (id == DATA_SHARTUUL_DOOMGUARD_SUPER_JUMP_TARGET)
                superJumpTargetGuid = guid;
            else if (id == DATA_SHARTUUL_DOOMGUARD_FEL_FLAMES_TARGET)
                felFlamesTargetGuid = guid;
        }

        void DoAction(int32 action) override
        {
            switch (action)
            {
                case ACTION_SHARTUUL_DOOMGUARD_SUPER_JUMP:
                    if (Unit* target = ObjectAccessor::GetUnit(*me, superJumpTargetGuid))
                        StartSuperJump(target);
                    break;
                case ACTION_SHARTUUL_DOOMGUARD_FEL_FLAMES:
                    if (Unit* target = ObjectAccessor::GetUnit(*me, felFlamesTargetGuid))
                        StartControlledFelFlames(target);
                    break;
                default:
                    break;
            }
        }

        void SpellHitTarget(Unit* target, SpellInfo const* spellInfo) override
        {
            if (!me->isPossessed())
                return;

            if (spellInfo && spellInfo->Id == SPELL_SHARTUUL_DOOMGUARD_SUPER_JUMP_IMPACT)
                return;

            if (IsShartuulWaveDemon(target) && !IsShartuulProtectedAmbient(target))
                return;

            me->AttackStop();
            me->CombatStop(true);
        }

        void DoSuperJumpImpact()
        {
            uint32 const entries[] =
            {
                NPC_SHARTUUL_WAVE_FELHOUND,
                NPC_SHARTUUL_WAVE_IMP,
                NPC_SHARTUUL_GANARG_UNDERLING,
                NPC_SHARTUUL_MOARG_TORMENTER,
                NPC_SHARTUUL_PORTABLE_FEL_CANNON,
                NPC_SHARTUUL_SHIVAN_ASSASSIN
            };

            me->CastSpell(me, 45413, true);

            for (uint32 entry : entries)
            {
                std::list<Creature*> creatures;
                GetCreatureListWithEntryInGrid(creatures, me, entry, 8.0f);
                for (Creature* creature : creatures)
                {
                    if (!creature->IsAlive() || IsShartuulProtectedAmbient(creature))
                        continue;

                    ApplyShartuulEventStun(creature, 5000);
                    Unit::DealDamage(me, creature, urand(750, 1100), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL);
                }
            }
        }

        void DoSuperJumpTakeoffImpact()
        {
            uint32 const entries[] =
            {
                NPC_SHARTUUL_WAVE_FELHOUND,
                NPC_SHARTUUL_WAVE_IMP,
                NPC_SHARTUUL_GANARG_UNDERLING,
                NPC_SHARTUUL_MOARG_TORMENTER,
                NPC_SHARTUUL_PORTABLE_FEL_CANNON,
                NPC_SHARTUUL_SHIVAN_ASSASSIN
            };

            for (uint32 entry : entries)
            {
                std::list<Creature*> creatures;
                GetCreatureListWithEntryInGrid(creatures, me, entry, 5.0f);
                for (Creature* creature : creatures)
                {
                    if (!creature->IsAlive() || IsShartuulProtectedAmbient(creature))
                        continue;

                    creature->CastSpell(creature, 45413, true);
                    Unit::DealDamage(me, creature, urand(350, 550), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL);
                }
            }
        }

        void StartSuperJump(Unit* target)
        {
            if (!target || !target->IsAlive())
                return;

            superJumpTargetGuid = target->GetGUID();
            superJumpLandingPosition = target->GetPosition();
            events.CancelEvent(EVENT_SHARTUUL_SUPER_JUMP_START);
            events.CancelEvent(EVENT_SHARTUUL_SUPER_JUMP_LAND);
            me->AttackStop();
            me->StopMoving();
            me->ClearUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING);
            me->SetControlled(false, UNIT_STATE_ROOT);
            me->GetMotionMaster()->Clear();
            BeginSuperJumpMovement();
        }

        void BeginSuperJumpMovement()
        {
            if (superJumpTargetGuid.IsEmpty())
                return;

            Unit* target = ObjectAccessor::GetUnit(*me, superJumpTargetGuid);
            if (target && target->IsAlive())
            {
                float const dxToMe = me->GetPositionX() - target->GetPositionX();
                float const dyToMe = me->GetPositionY() - target->GetPositionY();
                float const distToMe = std::sqrt(dxToMe * dxToMe + dyToMe * dyToMe);
                float const landingOffset = target->GetCombatReach() + me->GetCombatReach() + 0.75f;
                superJumpLandingPosition = target->GetPosition();
                if (distToMe > 0.1f)
                {
                    superJumpLandingPosition.m_positionX += (dxToMe / distToMe) * landingOffset;
                    superJumpLandingPosition.m_positionY += (dyToMe / distToMe) * landingOffset;
                }
            }

            if (me->GetExactDist2d(superJumpLandingPosition.GetPositionX(), superJumpLandingPosition.GetPositionY()) <= 0.1f)
            {
                events.ScheduleEvent(EVENT_SHARTUUL_SUPER_JUMP_LAND, 100ms);
                return;
            }

            float const dx = superJumpLandingPosition.GetPositionX() - me->GetPositionX();
            float const dy = superJumpLandingPosition.GetPositionY() - me->GetPositionY();
            float const dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= 0.1f)
            {
                events.ScheduleEvent(EVENT_SHARTUUL_SUPER_JUMP_LAND, 100ms);
                return;
            }

            DoSuperJumpTakeoffImpact();
            me->GetMotionMaster()->MoveJump(superJumpLandingPosition.GetPositionX(), superJumpLandingPosition.GetPositionY(), superJumpLandingPosition.GetPositionZ(), 24.0f, 12.0f, POINT_SHARTUUL_DOOMGUARD_SUPER_JUMP);
            events.ScheduleEvent(EVENT_SHARTUUL_SUPER_JUMP_LAND, Milliseconds(std::clamp<uint32>(uint32(dist / 24.0f * 1000.0f) + 250, 700, 1800)));
        }

        void StartThrowAxe(Unit* target)
        {
            if (!target || !target->IsAlive())
                return;

            me->SetFacingToObject(target);
            me->CastSpell(target, 40564, true);
            target->CastSpell(target, 45413, true);
            Unit::DealDamage(me, target, urand(1800, 2600), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL);
            me->AddThreat(target, 100000.0f);
            me->AI()->AttackStart(target);
        }

        Unit* GetSelectedTargetForPossessedDoomguard()
        {
            if (Player* player = ObjectAccessor::GetPlayer(*me, me->GetCharmerGUID()))
                if (Unit* target = player->GetSelectedUnit())
                    return target;

            if (Unit* target = ObjectAccessor::GetUnit(*me, me->GetGuidValue(UNIT_FIELD_TARGET)))
                return target;

            return me->GetVictim();
        }

        void StartControlledFelFlames(Unit* target)
        {
            if (!target || !target->IsAlive())
                return;

            felFlamesTargetGuid = target->GetGUID();
            felFlamesDurationTimer = 5500;
            felFlamesTickTimer = 1000;
            felFlamesVisualTimer = 0;
            felFlamesTicksDone = 0;

            if (Creature* oldHelper = ObjectAccessor::GetCreature(*me, felFlamesHelperGuid))
                oldHelper->DespawnOrUnsummon();

            Creature* channelHelper = me->SummonCreature(NPC_WORLD_EVENT_GENERATOR, me->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 6000);
            felFlamesHelperGuid = channelHelper ? channelHelper->GetGUID() : ObjectGuid::Empty;
            if (channelHelper)
            {
                channelHelper->SetDisplayId(MODEL_INVISIBLE);
                channelHelper->SetReactState(REACT_PASSIVE);
                channelHelper->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                channelHelper->SetFacingToObject(target);
            }

            if (Player* player = ObjectAccessor::GetPlayer(*me, me->GetCharmerGUID()))
            {
                ShartuulPendingFelFlamesCasts[player->GetGUID()] = { me->GetGUID(), target->GetGUID(), felFlamesHelperGuid, 5500, 1000, 0, 0 };
                felFlamesDurationTimer = 0;
                felFlamesTickTimer = 0;
                felFlamesVisualTimer = 0;
                felFlamesTicksDone = 0;
                return;
            }

            RefreshFelFlamesVisual(true);
        }

        void RefreshFelFlamesVisual(bool castVisual)
        {
            Unit* target = ObjectAccessor::GetUnit(*me, felFlamesTargetGuid);
            if (!target || !target->IsAlive())
                return;

            me->SetFacingToObject(target);
            me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, target->GetGUID());
            me->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES);
            me->ClearUnitState(UNIT_STATE_CASTING);

            if (Creature* helper = ObjectAccessor::GetCreature(*me, felFlamesHelperGuid))
            {
                helper->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 1.0f, me->GetOrientation());
                helper->SetFacingToObject(target);
                helper->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, target->GetGUID());
                helper->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES);

                if (castVisual)
                {
                    ShartuulFelFlamesVisualCasters.insert(helper->GetGUID());
                    helper->CastSpell(target, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES, false);
                }
            }

            if (castVisual)
            {
                ShartuulFelFlamesVisualCasters.insert(me->GetGUID());
                me->CastSpell(target, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES, false);
                me->ClearUnitState(UNIT_STATE_CASTING);
            }
        }

        void StopFelFlames()
        {
            felFlamesDurationTimer = 0;
            felFlamesTickTimer = 0;
            felFlamesVisualTimer = 0;
            felFlamesTicksDone = 0;
            me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, ObjectGuid::Empty);
            me->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
            me->ClearUnitState(UNIT_STATE_CASTING);

            if (Creature* helper = ObjectAccessor::GetCreature(*me, felFlamesHelperGuid))
            {
                ShartuulFelFlamesVisualCasters.erase(helper->GetGUID());
                helper->DespawnOrUnsummon();
            }

            ShartuulFelFlamesVisualCasters.erase(me->GetGUID());
            felFlamesTargetGuid.Clear();
            felFlamesHelperGuid.Clear();
        }

        void UpdateControlledFelFlames(uint32 diff)
        {
            if (!felFlamesDurationTimer)
                return;

            Unit* target = ObjectAccessor::GetUnit(*me, felFlamesTargetGuid);
            if (!target || !target->IsAlive())
            {
                StopFelFlames();
                return;
            }

            if (felFlamesVisualTimer <= diff)
            {
                RefreshFelFlamesVisual(false);
                felFlamesVisualTimer = 250;
            }
            else
                felFlamesVisualTimer -= diff;

            if (felFlamesTickTimer <= diff)
            {
                if (felFlamesTicksDone < SHARTUUL_DOOMGUARD_FEL_FLAMES_TICKS && IsShartuulWaveDemon(target) && !IsShartuulProtectedAmbient(target))
                {
                    Unit::DealDamage(me, target, 1000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE);
                    ++felFlamesTicksDone;
                }

                felFlamesTickTimer = 1000;
            }
            else
                felFlamesTickTimer -= diff;

            if (felFlamesDurationTimer <= diff)
                StopFelFlames();
            else
                felFlamesDurationTimer -= diff;
        }

        void FinishSuperJump()
        {
            if (superJumpTargetGuid.IsEmpty())
                return;

            events.CancelEvent(EVENT_SHARTUUL_SUPER_JUMP_LAND);
            if (me->GetExactDist2d(superJumpLandingPosition.GetPositionX(), superJumpLandingPosition.GetPositionY()) > 5.0f)
                me->NearTeleportTo(superJumpLandingPosition.GetPositionX(), superJumpLandingPosition.GetPositionY(), superJumpLandingPosition.GetPositionZ(), me->GetOrientation());

            DoSuperJumpImpact();

            if (Unit* target = ObjectAccessor::GetUnit(*me, superJumpTargetGuid))
            {
                me->SetFacingToObject(target);
                if (!me->GetVictim())
                    me->AI()->AttackStart(target);
            }

            superJumpTargetGuid.Clear();
        }

        void UpdateAI(uint32 diff) override
        {
            UpdateControlledFelFlames(diff);

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SHARTUUL_SUPER_JUMP_START:
                        BeginSuperJumpMovement();
                        break;
                    case EVENT_SHARTUUL_SUPER_JUMP_LAND:
                        FinishSuperJump();
                        break;
                    default:
                        break;
                }
            }
        }

        void OnSpellCast(SpellInfo const* spellInfo) override
        {
            if (!me->isPossessed() || !spellInfo)
                return;

            if (spellInfo->Id == SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES)
                StartControlledFelFlames(GetSelectedTargetForPossessedDoomguard());
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != EFFECT_MOTION_TYPE || id != POINT_SHARTUUL_DOOMGUARD_SUPER_JUMP)
                return;

            FinishSuperJump();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shartuul_doomguard_punisherAI(creature);
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Possess this Doomguard Punisher", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
        if (action == GOSSIP_ACTION_INFO_DEF + 1)
            PossessShartuulTestDemon(player, creature);

        CloseGossipMenuFor(player);
        return true;
    }
};

class npc_shartuul_wave_melee : public CreatureScript
{
public:
    npc_shartuul_wave_melee(char const* scriptName) : CreatureScript(scriptName) { }

    struct npc_shartuul_wave_meleeAI : public ShartuulWaveCreatureAI
    {
        npc_shartuul_wave_meleeAI(Creature* creature) : ShartuulWaveCreatureAI(creature) { }

        void UpdateAI(uint32 diff) override
        {
            if (!PrepareEventCombatUpdate(diff))
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shartuul_wave_meleeAI(creature);
    }
};

class npc_shartuul_wave_imp : public CreatureScript
{
public:
    npc_shartuul_wave_imp() : CreatureScript("npc_shartuul_wave_imp") { }

    struct npc_shartuul_wave_impAI : public ShartuulWaveCreatureAI
    {
        npc_shartuul_wave_impAI(Creature* creature) : ShartuulWaveCreatureAI(creature) { }

        uint32 fireboltTimer = 1500;
        uint32 fireboltCastTimer = 0;
        ObjectGuid fireboltTargetGuid;

        void Reset() override
        {
            ShartuulWaveCreatureAI::Reset();
            fireboltTimer = 1500;
            fireboltCastTimer = 0;
            fireboltTargetGuid.Clear();
        }

        void UpdateAI(uint32 diff) override
        {
            if (UpdateEventStun(diff))
            {
                fireboltCastTimer = 0;
                fireboltTargetGuid.Clear();
                me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, ObjectGuid::Empty);
                me->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
                return;
            }

            if (fireboltCastTimer)
            {
                if (fireboltCastTimer > diff)
                {
                    fireboltCastTimer -= diff;
                    return;
                }

                Unit* target = ObjectAccessor::GetUnit(*me, fireboltTargetGuid);
                fireboltCastTimer = 0;
                fireboltTargetGuid.Clear();
                me->ClearUnitState(UNIT_STATE_CASTING);
                me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, ObjectGuid::Empty);
                me->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);

                if (target && target->IsAlive())
                {
                    me->SetFacingToObject(target);
                    me->CastSpell(target, SPELL_SHARTUUL_IMP_FIREBOLT, true);
                    Unit::DealDamage(me, target, urand(900, 1200), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE);
                }

                return;
            }

            if (!PrepareEventCombatUpdate(diff))
                return;

            Unit* target = GetEventTarget();
            if (!target)
                return;

            float const distance = me->GetExactDist2d(target);
            if (distance > 28.0f)
            {
                me->GetMotionMaster()->MoveChase(target, 24.0f);
                return;
            }

            me->StopMoving();
            me->GetMotionMaster()->Clear();
            me->SetFacingToObject(target);

            if (fireboltTimer > diff)
            {
                fireboltTimer -= diff;
                return;
            }

            fireboltTimer = 2200;
            fireboltCastTimer = 1500;
            fireboltTargetGuid = target->GetGUID();
            me->AddUnitState(UNIT_STATE_CASTING);
            me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, target->GetGUID());
            me->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_IMP_FIREBOLT);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shartuul_wave_impAI(creature);
    }
};

class npc_shartuul_portable_fel_cannon : public CreatureScript
{
public:
    npc_shartuul_portable_fel_cannon() : CreatureScript("npc_shartuul_portable_fel_cannon") { }

    struct npc_shartuul_portable_fel_cannonAI : public ShartuulWaveCreatureAI
    {
        npc_shartuul_portable_fel_cannonAI(Creature* creature) : ShartuulWaveCreatureAI(creature) { }

        uint32 fireTimer = 2000;

        void Reset() override
        {
            ShartuulWaveCreatureAI::Reset();
            fireTimer = 2000;
            me->SetObjectScale(0.5f);
            me->SetControlled(true, UNIT_STATE_ROOT);
        }

        void UpdateAI(uint32 diff) override
        {
            if (UpdateEventStun(diff))
                return;

            if (!PrepareEventCombatUpdate(diff))
                return;

            Unit* target = GetEventTarget();
            if (!target)
                return;

            me->SetControlled(true, UNIT_STATE_ROOT);
            me->StopMoving();
            me->GetMotionMaster()->Clear();
            me->SetFacingToObject(target);

            if (fireTimer > diff)
            {
                fireTimer -= diff;
                return;
            }

            fireTimer = 3000;
            me->CastSpell(target, SPELL_SHARTUUL_FEL_CANNON_BLAST, true);
            me->CastSpell(target, SPELL_SHARTUUL_FEL_CANNON_TRACER, true);
            me->CastSpell(target, SPELL_SHARTUUL_FEL_CANNON_BOLT, true);
            Unit::DealDamage(me, target, 2500, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shartuul_portable_fel_cannonAI(creature);
    }
};

class npc_shartuul_shivan_assassin : public CreatureScript
{
public:
    npc_shartuul_shivan_assassin() : CreatureScript("npc_shartuul_shivan_assassin") { }

    struct npc_shartuul_shivan_assassinAI : public ScriptedAI
    {
        npc_shartuul_shivan_assassinAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            if (!me->isPossessed())
                PrepareShartuulTestPossessDemon(me);
        }

        void OnPossess(bool apply)
        {
            if (apply)
            {
                me->SetFaction(FactionShartuulControlled);
                me->SetReactState(REACT_PASSIVE);
                return;
            }

            if (Creature* controller = GetShartuulController(me))
                controller->AI()->DoAction(ACTION_SHARTUUL_RESET);
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
        {
            if (!me->isPossessed() || (IsShartuulWaveDemon(victim) && !IsShartuulProtectedAmbient(victim)))
                return;

            damage = 0;
            me->AttackStop();
            me->CombatStop(true);
        }

        bool CanAIAttack(Unit const* target) const override
        {
            return !IsShartuulProtectedAmbient(target);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shartuul_shivan_assassinAI(creature);
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Possess this Shivan Assassin", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Start Shartuul Eye phase intro test", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
        if (action == GOSSIP_ACTION_INFO_DEF + 1)
            PossessShartuulTestDemon(player, creature);
        else if (action == GOSSIP_ACTION_INFO_DEF + 2)
        {
            if (Creature* controller = GetShartuulController(creature))
            {
                controller->AI()->SetGUID(player->GetGUID(), DATA_SHARTUUL_PLAYER);
                controller->AI()->SetGUID(creature->GetGUID(), DATA_SHARTUUL_DEGRADER);
                controller->AI()->DoAction(ACTION_SHARTUUL_TEST_EYE_INTRO);
            }
        }

        CloseGossipMenuFor(player);
        return true;
    }
};

class npc_shartuul_moarg_tormenter : public CreatureScript
{
public:
    npc_shartuul_moarg_tormenter() : CreatureScript("npc_shartuul_moarg_tormenter") { }

    struct npc_shartuul_moarg_tormenterAI : public ShartuulWaveCreatureAI
    {
        npc_shartuul_moarg_tormenterAI(Creature* creature) : ShartuulWaveCreatureAI(creature) { }

        EventMap events;
        ObjectGuid acidGeyserTargetGuid;
        bool acidGeyserActive = false;
        bool acidGeyserScheduled = false;

        void Reset() override
        {
            ShartuulWaveCreatureAI::Reset();
            events.Reset();
            acidGeyserTargetGuid.Clear();
            acidGeyserActive = false;
            acidGeyserScheduled = false;
            me->RemoveUnitFlag(UNIT_FLAG_DISABLE_MOVE);
            me->SetControlled(false, UNIT_STATE_ROOT);
            me->ClearUnitState(UNIT_STATE_CASTING | UNIT_STATE_ROOT | UNIT_STATE_STUNNED);
            me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, ObjectGuid::Empty);
            me->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
            me->SetObjectScale(0.5f);
            me->SetRegeneratingHealth(false);
            me->SetHomePosition(me->GetPosition());
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            if (!acidGeyserScheduled && !acidGeyserActive)
            {
                acidGeyserScheduled = true;
                events.ScheduleEvent(EVENT_SHARTUUL_MOARG_ACID_GEYSER, 3s);
            }
        }

        bool IsValidTormenterTarget(Unit* target) const
        {
            return target && target->IsAlive() && target->GetEntry() == NPC_DOOMGUARD_PUNISHER && target->isPossessed();
        }

        void ForceEngage(Unit* target)
        {
            if (!IsValidTormenterTarget(target))
                return;

            me->SetFaction(FactionShartuulWaveDemon);
            me->SetRegeneratingHealth(false);
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            me->SetControlled(false, UNIT_STATE_ROOT);
            me->SetControlled(false, UNIT_STATE_STUNNED);
            me->ClearUnitState(UNIT_STATE_CASTING | UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_EVADE);
            me->SetHomePosition(me->GetPosition());
            me->SetInCombatWith(target);
            target->SetInCombatWith(me);
            me->AddThreat(target, 100000.0f);
            targetGuid = target->GetGUID();
            AttackStart(target);
            if (!acidGeyserScheduled && !acidGeyserActive)
            {
                acidGeyserScheduled = true;
                events.ScheduleEvent(EVENT_SHARTUUL_MOARG_ACID_GEYSER, 3s);
            }
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (!me->GetVictim() && me->IsWithinDistInMap(who, 45.0f))
                ForceEngage(who);
        }

        void DamageTaken(Unit* attacker, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
        {
            if (!me->GetVictim())
                ForceEngage(attacker);
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            acidGeyserActive = false;
            acidGeyserScheduled = false;
            acidGeyserTargetGuid.Clear();
            me->RemoveUnitFlag(UNIT_FLAG_DISABLE_MOVE);
            me->SetControlled(false, UNIT_STATE_ROOT);
            me->SetControlled(false, UNIT_STATE_STUNNED);
            me->ClearUnitState(UNIT_STATE_CASTING | UNIT_STATE_ROOT | UNIT_STATE_STUNNED);
            me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, ObjectGuid::Empty);
            me->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
            me->SetHomePosition(me->GetPosition());
            me->SetRegeneratingHealth(false);
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveIdle();
            me->SetReactState(REACT_AGGRESSIVE);
            events.Reset();

            std::list<Creature*> targets;
            GetCreatureListWithEntryInGrid(targets, me, NPC_DOOMGUARD_PUNISHER, 80.0f);
            for (Creature* target : targets)
            {
                if (!target->IsAlive() || !target->isPossessed())
                    continue;

                ForceEngage(target);
                break;
            }
        }

        void SetGUID(ObjectGuid const& guid, int32 id) override
        {
            if (id == DATA_SHARTUUL_MOARG_ACID_GEYSER_TARGET)
            {
                acidGeyserTargetGuid = guid;
                return;
            }

            ShartuulWaveCreatureAI::SetGUID(guid, id);
        }

        void DoAction(int32 action) override
        {
            if (action != ACTION_SHARTUUL_MOARG_ACID_GEYSER)
                return;

            Unit* target = ObjectAccessor::GetUnit(*me, acidGeyserTargetGuid);
            if (!target)
                target = me->GetVictim();

            StartAcidGeyser(target);
        }

        void StartAcidGeyser(Unit* victim)
        {
            if (eventStunTimer || acidGeyserActive)
                return;

            if (!victim || !victim->IsAlive())
                return;

            acidGeyserActive = true;
            acidGeyserTargetGuid = victim->GetGUID();
            targetGuid = victim->GetGUID();
            events.CancelEvent(EVENT_SHARTUUL_MOARG_ACID_GEYSER_FINISH);
            me->InterruptNonMeleeSpells(false);
            me->AttackStop();
            me->SetHomePosition(me->GetPosition());
            me->StopMovingOnCurrentPos();
            me->StopMoving();
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveIdle();
            me->SetControlled(true, UNIT_STATE_ROOT);
            me->SetControlled(false, UNIT_STATE_STUNNED);
            me->SetUnitFlag(UNIT_FLAG_DISABLE_MOVE);
            me->AddUnitState(UNIT_STATE_ROOT);
            me->SetReactState(REACT_PASSIVE);
            me->SetFacingToObject(victim);
            SendShartuulCastBar(me, victim, SPELL_SHARTUUL_MOARG_ACID_GEYSER_CASTBAR, 5000);
            me->CastSpell(victim, SPELL_SHARTUUL_MOARG_ACID_GEYSER_VISUAL, true);
            me->SendPlaySpellVisual(179);
            me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, victim->GetGUID());
            me->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_MOARG_ACID_GEYSER_VISUAL);

            Unit::DealDamage(me, victim, 2200, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NATURE);
            events.ScheduleEvent(EVENT_SHARTUUL_MOARG_ACID_GEYSER_FINISH, 5s);
        }

        void MaintainAcidGeyserLock()
        {
            if (!acidGeyserActive)
                return;

            Unit* victim = ObjectAccessor::GetUnit(*me, acidGeyserTargetGuid);
            me->AttackStop();
            me->StopMovingOnCurrentPos();
            me->StopMoving();
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveIdle();
            me->SetControlled(true, UNIT_STATE_ROOT);
            me->SetUnitFlag(UNIT_FLAG_DISABLE_MOVE);
            me->AddUnitState(UNIT_STATE_ROOT);
            me->SetReactState(REACT_PASSIVE);
            if (victim && victim->IsAlive())
            {
                me->SetFacingToObject(victim);
                me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, victim->GetGUID());
                me->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_MOARG_ACID_GEYSER_VISUAL);
            }
        }

        void FinishAcidGeyser()
        {
            acidGeyserActive = false;
            Unit* victim = ObjectAccessor::GetUnit(*me, acidGeyserTargetGuid);

            me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, ObjectGuid::Empty);
            me->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
            me->ClearUnitState(UNIT_STATE_CASTING | UNIT_STATE_ROOT | UNIT_STATE_STUNNED);
            me->SetControlled(false, UNIT_STATE_ROOT);
            me->SetControlled(false, UNIT_STATE_STUNNED);
            me->RemoveUnitFlag(UNIT_FLAG_DISABLE_MOVE);
            me->SetRegeneratingHealth(false);
            me->SetReactState(REACT_AGGRESSIVE);

            if (victim && victim->IsAlive())
            {
                me->SetInCombatWith(victim);
                victim->SetInCombatWith(me);
                me->AddThreat(victim, 100000.0f);
                AttackStart(victim);
                me->GetMotionMaster()->MoveChase(victim);
                acidGeyserScheduled = true;
                events.ScheduleEvent(EVENT_SHARTUUL_MOARG_ACID_GEYSER, 15s);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            me->SetRegeneratingHealth(false);

            if (UpdateEventStun(diff))
                return;

            MaintainAcidGeyserLock();

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_SHARTUUL_MOARG_ACID_GEYSER)
                {
                    acidGeyserScheduled = false;
                    Unit* target = me->GetVictim();
                    if (!target)
                        target = GetEventTarget();
                    if (target)
                        ForceEngage(target);
                    StartAcidGeyser(target);
                }
                else if (eventId == EVENT_SHARTUUL_MOARG_ACID_GEYSER_FINISH)
                    FinishAcidGeyser();
            }

            if (!acidGeyserActive && !me->HasUnitFlag(UNIT_FLAG_DISABLE_MOVE))
            {
                if (!me->GetVictim())
                    if (Unit* target = GetEventTarget())
                        EngageEventTarget(target);

                if (me->GetVictim() && !acidGeyserScheduled)
                {
                    acidGeyserScheduled = true;
                    events.ScheduleEvent(EVENT_SHARTUUL_MOARG_ACID_GEYSER, 3s);
                }

                DoMeleeAttackIfReady();
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shartuul_moarg_tormenterAI(creature);
    }
};

enum ShartuulPhaseThreeEvents
{
    EVENT_SHARTUUL_EYE_FIREBALL = 1,
    EVENT_SHARTUUL_EYE_DISRUPTION,
    EVENT_SHARTUUL_EYE_TONGUE_LASH,
    EVENT_SHARTUUL_EYE_DARK_GLARE_START,
    EVENT_SHARTUUL_EYE_DARK_GLARE_HIT,
    EVENT_SHARTUUL_DREADMAW_GROWTH,
    EVENT_SHARTUUL_DREADMAW_CHARGE,
    EVENT_SHARTUUL_DREADMAW_CHARGE_HIT,
    EVENT_SHARTUUL_EREDAR_SHADOW_BOLT,
    EVENT_SHARTUUL_EREDAR_SHADOW_RESONANCE,
    EVENT_SHARTUUL_EREDAR_IMMOLATE,
    EVENT_SHARTUUL_EREDAR_INCINERATE_START,
    EVENT_SHARTUUL_EREDAR_INCINERATE_HIT,
    EVENT_SHARTUUL_EREDAR_MAGNETIC_PULL,
    EVENT_SHARTUUL_EYE_CHAIN,
    EVENT_SHARTUUL_EYE_FINISH_CAST,
    EVENT_SHARTUUL_PHASE_THREE_SUMMON_NEXT
};

Position const ShartuulBlinkPositions[] =
{
    { 2647.0f, 7097.0f, 365.1f, 0.0f },
    { 2699.0f, 7171.0f, 367.3f, 0.0f },
    { 2784.0f, 7132.0f, 365.4f, 0.0f },
    { 2735.0f, 7060.0f, 365.5f, 0.0f }
};

struct ShartuulPhaseThreeBossAI : public ShartuulWaveCreatureAI
{
    ShartuulPhaseThreeBossAI(Creature* creature) : ShartuulWaveCreatureAI(creature) { }

    EventMap bossEvents;
    ObjectGuid delayedTargetGuid;
    bool bossStarted = false;

    void Reset() override
    {
        ShartuulWaveCreatureAI::Reset();
        bossEvents.Reset();
        delayedTargetGuid.Clear();
        bossStarted = false;
        me->SetRegeneratingHealth(false);
    }

    void SetGUID(ObjectGuid const& guid, int32 id) override
    {
        ShartuulWaveCreatureAI::SetGUID(guid, id);
        if (id == DATA_SHARTUUL_DEGRADER && !bossStarted)
            StartBossEvents();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        StartBossEvents();
    }

    void StartBossEvents()
    {
        if (bossStarted)
            return;

        bossStarted = true;
        ScheduleBossEvents();
    }

    virtual void ScheduleBossEvents() { }

    Unit* GetBossTarget()
    {
        Unit* target = GetEventTarget();
        if (target && target->IsAlive())
            return target;

        return nullptr;
    }

    void KeepBossCombat()
    {
        if (Unit* target = GetBossTarget())
        {
            if (!me->GetVictim())
                EngageEventTarget(target);
        }
    }

    void CastVisual(Unit* target, uint32 spellId, bool triggered = true)
    {
        if (target && target->IsAlive())
        {
            me->SetFacingToObject(target);
            me->CastSpell(target, spellId, triggered);
        }
    }

    void DamageTarget(Unit* target, uint32 amount, SpellSchoolMask school)
    {
        if (target && target->IsAlive())
            Unit::DealDamage(me, target, amount, nullptr, SPELL_DIRECT_DAMAGE, school);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!PrepareEventCombatUpdate(diff))
            return;

        if (!bossStarted)
            StartBossEvents();

        bossEvents.Update(diff);
        while (uint32 eventId = bossEvents.ExecuteEvent())
            ExecuteBossEvent(eventId);

        DoMeleeAttackIfReady();
    }

    virtual void ExecuteBossEvent(uint32 /*eventId*/) { }
};

class npc_shartuul_eye_of_shartuul : public CreatureScript
{
public:
    npc_shartuul_eye_of_shartuul() : CreatureScript("npc_shartuul_eye_of_shartuul") { }

    struct npc_shartuul_eye_of_shartuulAI : public ShartuulPhaseThreeBossAI
    {
        npc_shartuul_eye_of_shartuulAI(Creature* creature) : ShartuulPhaseThreeBossAI(creature) { }

        ObjectGuid disruptionZoneGuid;
        ObjectGuid pendingDisruptionZoneGuid;
        Position disruptionZonePosition;
        uint32 disruptionZoneTimer = 0;
        uint32 disruptionPulseTimer = 0;
        uint8 eyeChainStep = 0;
        uint8 activeEyeCast = 0;

        void Reset() override
        {
            ShartuulPhaseThreeBossAI::Reset();
            ApplySniffedPhaseThreeCreatureState(me);
            me->SetReactState(REACT_PASSIVE);
            disruptionZoneGuid.Clear();
            pendingDisruptionZoneGuid.Clear();
            disruptionZoneTimer = 0;
            disruptionPulseTimer = 0;
            eyeChainStep = 0;
            activeEyeCast = 0;
            me->SetSpeedRate(MOVE_WALK, 0.45f);
            me->SetSpeedRate(MOVE_RUN, 0.45f);
            me->SetSpeedRate(MOVE_FLIGHT, 0.45f);
        }

        void ScheduleBossEvents() override
        {
            bossEvents.ScheduleEvent(EVENT_SHARTUUL_EYE_CHAIN, 2s);
            bossEvents.ScheduleEvent(EVENT_SHARTUUL_EYE_TONGUE_LASH, 1s);
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType damageType, SpellSchoolMask damageSchoolMask) override
        {
            ShartuulPhaseThreeBossAI::DamageDealt(victim, damage, damageType, damageSchoolMask);
            if (damageType == DIRECT_DAMAGE && (damageSchoolMask & SPELL_SCHOOL_MASK_NORMAL) && IsPossessedShartuulControlledDemon(victim))
                damage = std::min<uint32>(damage, urand(15000, 18000));
        }

        void HoldCasterPosition(Unit* target)
        {
            me->AttackStop();
            me->SetGuidValue(UNIT_FIELD_TARGET, target ? target->GetGUID() : ObjectGuid::Empty);
            me->StopMoving();
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveIdle();
            me->SetReactState(REACT_PASSIVE);

            if (!target || !target->IsAlive())
                return;

            me->SetInCombatWith(target);
            target->SetInCombatWith(me);
            me->AddThreat(target, 100000.0f);
            me->SetFacingToObject(target);
        }

        void MoveSlowlyTowardTarget(Unit* target)
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetGuidValue(UNIT_FIELD_TARGET, target ? target->GetGUID() : ObjectGuid::Empty);

            if (!target || !target->IsAlive())
                return;

            me->SetInCombatWith(target);
            target->SetInCombatWith(me);
            me->AddThreat(target, 100000.0f);
            me->SetSpeedRate(MOVE_WALK, 0.45f);
            me->SetSpeedRate(MOVE_RUN, 0.45f);
            me->SetSpeedRate(MOVE_FLIGHT, 0.45f);

            if (me->GetDistance(target) > 6.0f)
                me->GetMotionMaster()->MoveChase(target, 4.5f);
            else
            {
                me->StopMoving();
                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MoveIdle();
                me->SetFacingToObject(target);
            }
        }

        Creature* CreateDisruptionGroundVisual(Unit* target, uint32 despawnMs)
        {
            if (!target)
                return nullptr;

            Position pos = target->GetPosition();
            pos.m_positionZ = target->GetPositionZ();
            Creature* zone = me->SummonCreature(NPC_WORLD_EVENT_GENERATOR, pos, TEMPSUMMON_TIMED_DESPAWN, despawnMs);
            if (!zone)
                return nullptr;

            zone->SetDisplayId(MODEL_INVISIBLE);
            zone->SetObjectScale(0.1f);
            zone->SetPhaseMask(PHASEMASK_NORMAL, true);
            zone->SetFaction(FactionShartuulWaveDemon);
            zone->SetReactState(REACT_PASSIVE);
            zone->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
            return zone;
        }

        void StartEyeCast(Unit* target, uint8 castType, uint32 castBarSpell, uint32 durationMs, char const* warning = nullptr)
        {
            if (!target)
                return;

            activeEyeCast = castType;
            delayedTargetGuid = target->GetGUID();
            pendingDisruptionZoneGuid.Clear();
            HoldCasterPosition(target);
            me->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST);
            SendShartuulCastBar(me, target, castBarSpell, durationMs);
            me->InterruptNonMeleeSpells(false);
            me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, ObjectGuid::Empty);
            me->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);

            switch (castType)
            {
                case 1:
                    me->CastSpell(target, SPELL_SHARTUUL_FEL_CANNON_TRACER, true);
                    me->CastSpell(target, SPELL_SHARTUUL_FEL_CANNON_BOLT, false);
                    break;
                case 2:
                    if (Creature* zone = CreateDisruptionGroundVisual(target, durationMs + 11000))
                    {
                        pendingDisruptionZoneGuid = zone->GetGUID();
                        zone->CastSpell(zone, SPELL_SHARTUUL_EYE_DISRUPTION, true);
                        me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, zone->GetGUID());
                        me->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_BOUNDARY_DRAIN_LIFE);
                    }
                    break;
                case 3:
                    me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, target->GetGUID());
                    me->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_BOUNDARY_DRAIN_LIFE);
                    me->CastSpell(target, SPELL_SHARTUUL_GENERIC_GLARE, true);
                    break;
                default:
                    break;
            }

            if (warning)
                if (Player* player = ObjectAccessor::GetPlayer(*me, target->GetCharmerGUID()))
                    player->GetSession()->SendAreaTriggerMessage(warning);

            bossEvents.ScheduleEvent(EVENT_SHARTUUL_EYE_FINISH_CAST, Milliseconds(durationMs));
        }

        void ActivateDisruptionZone(Unit* target)
        {
            if (!target)
                return;

            Creature* zone = ObjectAccessor::GetCreature(*me, pendingDisruptionZoneGuid);
            if (!zone)
                zone = CreateDisruptionGroundVisual(target, 10000);

            if (zone)
            {
                disruptionZoneGuid = zone->GetGUID();
                disruptionZonePosition = zone->GetPosition();
                zone->CastSpell(zone, SPELL_SHARTUUL_GREEN_LIGHTNING_THIN, true);
                zone->CastSpell(zone, SPELL_SHARTUUL_EYE_DISRUPTION, true);
            }
            else
            {
                disruptionZonePosition = target->GetPosition();
                disruptionZonePosition.m_positionZ = target->GetPositionZ();
            }

            pendingDisruptionZoneGuid.Clear();

            disruptionZoneTimer = 10000;
            disruptionPulseTimer = 500;
            if (Player* player = ObjectAccessor::GetPlayer(*me, target->GetCharmerGUID()))
                player->GetSession()->SendAreaTriggerMessage("Disruption Ray destabilizes the ground. Move out!");
        }

        void UpdateDisruptionZone(uint32 diff)
        {
            if (!disruptionZoneTimer)
                return;

            if (disruptionZoneTimer <= diff)
            {
                disruptionZoneTimer = 0;
                disruptionPulseTimer = 0;
                if (Creature* zone = ObjectAccessor::GetCreature(*me, disruptionZoneGuid))
                    zone->DespawnOrUnsummon();
                disruptionZoneGuid.Clear();
                return;
            }

            disruptionZoneTimer -= diff;
            if (disruptionPulseTimer > diff)
            {
                disruptionPulseTimer -= diff;
                return;
            }

            disruptionPulseTimer = 1000;
            Creature* zone = ObjectAccessor::GetCreature(*me, disruptionZoneGuid);
            if (zone)
                zone->CastSpell(zone, SPELL_SHARTUUL_GREEN_LIGHTNING_THIN, true);

            Unit* target = GetBossTarget();
            if (!target || target->GetExactDist2d(disruptionZonePosition.GetPositionX(), disruptionZonePosition.GetPositionY()) > 5.0f)
                return;

            target->CastSpell(target, SPELL_SHARTUUL_GENERIC_STUN, true);
            if (Creature* creature = target->ToCreature())
                ApplyShartuulEventStun(creature, 2500);
        }

        void UpdateAI(uint32 diff) override
        {
            UpdateDisruptionZone(diff);

            if (UpdateEventStun(diff))
                return;

            Unit* target = GetBossTarget();
            if (!target)
            {
                me->AttackStop();
                me->CombatStop(true);
                return;
            }

            if (activeEyeCast)
                HoldCasterPosition(target);
            else
                MoveSlowlyTowardTarget(target);

            if (!bossStarted)
                StartBossEvents();

            bossEvents.Update(diff);
            while (uint32 eventId = bossEvents.ExecuteEvent())
                ExecuteBossEvent(eventId);
        }

        void FinishEyeCast()
        {
            Unit* target = ObjectAccessor::GetUnit(*me, delayedTargetGuid);
            uint8 castType = activeEyeCast;
            activeEyeCast = 0;
            delayedTargetGuid.Clear();
            me->ClearUnitState(UNIT_STATE_CASTING);
            me->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, ObjectGuid::Empty);
            me->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);

            if (!target || !target->IsAlive())
                return;

            switch (castType)
            {
                case 1:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST);
                    me->CastSpell(target, SPELL_SHARTUUL_FEL_CANNON_TRACER, true);
                    me->CastSpell(target, SPELL_SHARTUUL_FEL_CANNON_BOLT, true);
                    DamageTarget(target, urand(26000, 32000), SPELL_SCHOOL_MASK_FIRE);
                    break;
                case 2:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST);
                    ActivateDisruptionZone(target);
                    break;
                case 3:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST);
                    if (GetShartuulShivanAspect(target) == ShartuulShivanAspect::Ice && IsShartuulShivanIceBlocked(target))
                    {
                        target->CastSpell(me, SPELL_SHARTUUL_BOUNDARY_DRAIN_LIFE, true);
                        Unit::DealDamage(target, me, 50000, nullptr, SPELL_DIRECT_DAMAGE, SPELL_SCHOOL_MASK_SHADOW);
                        if (Player* player = ObjectAccessor::GetPlayer(*me, target->GetCharmerGUID()))
                            player->GetSession()->SendAreaTriggerMessage("Ice Block reflects Dark Glare!");
                    }
                    else
                        DamageTarget(target, 100000, SPELL_SCHOOL_MASK_SHADOW);
                    break;
                default:
                    break;
            }
        }

        void ExecuteBossEvent(uint32 eventId) override
        {
            Unit* target = GetBossTarget();
            if (!target)
                return;

            switch (eventId)
            {
                case EVENT_SHARTUUL_EYE_CHAIN:
                    switch (eyeChainStep)
                    {
                        case 0:
                        case 2:
                        case 5:
                            StartEyeCast(target, 1, SPELL_SHARTUUL_FEL_CANNON_BOLT, 2500);
                            ++eyeChainStep;
                            bossEvents.ScheduleEvent(EVENT_SHARTUUL_EYE_CHAIN, 5s);
                            break;
                        case 1:
                        case 4:
                            StartEyeCast(target, 2, SPELL_SHARTUUL_EYE_DISRUPTION, 1500, "The Eye of Shartuul targets the ground with Disruption Ray!");
                            ++eyeChainStep;
                            bossEvents.ScheduleEvent(EVENT_SHARTUUL_EYE_CHAIN, 7s);
                            break;
                        case 3:
                            StartEyeCast(target, 3, SPELL_SHARTUUL_EYE_DARK_GLARE, 7000, "The Eye of Shartuul focuses intently!");
                            ++eyeChainStep;
                            bossEvents.ScheduleEvent(EVENT_SHARTUUL_EYE_CHAIN, 11s);
                            break;
                        default:
                            eyeChainStep = 0;
                            bossEvents.ScheduleEvent(EVENT_SHARTUUL_EYE_CHAIN, 3s);
                            break;
                    }
                    break;
                case EVENT_SHARTUUL_EYE_FINISH_CAST:
                    FinishEyeCast();
                    break;
                case EVENT_SHARTUUL_EYE_FIREBALL:
                    if (Unit* delayedTarget = ObjectAccessor::GetUnit(*me, delayedTargetGuid))
                        DamageTarget(delayedTarget, urand(28000, 32000), SPELL_SCHOOL_MASK_FIRE);
                    delayedTargetGuid.Clear();
                    break;
                case EVENT_SHARTUUL_EYE_DISRUPTION:
                    ActivateDisruptionZone(target);
                    bossEvents.ScheduleEvent(EVENT_SHARTUUL_EYE_DISRUPTION, 15s);
                    break;
                case EVENT_SHARTUUL_EYE_TONGUE_LASH:
                    if (me->GetDistance(target) < 7.0f)
                    {
                        CastVisual(target, SPELL_SHARTUUL_SHADOW_RESONANCE, true);
                        DamageTarget(target, urand(70000, 85000), SPELL_SCHOOL_MASK_NORMAL);
                    }
                    bossEvents.ScheduleEvent(EVENT_SHARTUUL_EYE_TONGUE_LASH, 3s);
                    break;
                case EVENT_SHARTUUL_EYE_DARK_GLARE_START:
                    if (Player* player = ObjectAccessor::GetPlayer(*me, target->GetCharmerGUID()))
                        player->GetSession()->SendAreaTriggerMessage("The Eye of Shartuul focuses intently!");
                    SendShartuulCastBar(me, target, SPELL_SHARTUUL_EYE_DARK_GLARE, 7000);
                    CastVisual(target, SPELL_SHARTUUL_EYE_DARK_GLARE, true);
                    delayedTargetGuid = target->GetGUID();
                    bossEvents.ScheduleEvent(EVENT_SHARTUUL_EYE_DARK_GLARE_HIT, 7s);
                    bossEvents.ScheduleEvent(EVENT_SHARTUUL_EYE_DARK_GLARE_START, 26s);
                    break;
                case EVENT_SHARTUUL_EYE_DARK_GLARE_HIT:
                    if (Unit* delayedTarget = ObjectAccessor::GetUnit(*me, delayedTargetGuid))
                    {
                        if (GetShartuulShivanAspect(delayedTarget) == ShartuulShivanAspect::Ice && IsShartuulShivanIceBlocked(delayedTarget))
                        {
                            delayedTarget->CastSpell(me, SPELL_SHARTUUL_EYE_DARK_GLARE, true);
                            Unit::DealDamage(delayedTarget, me, 50000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_SHADOW);
                            if (Player* player = ObjectAccessor::GetPlayer(*me, delayedTarget->GetCharmerGUID()))
                                player->GetSession()->SendAreaTriggerMessage("Ice Block reflects Dark Glare!");
                        }
                        else
                            DamageTarget(delayedTarget, 100000, SPELL_SCHOOL_MASK_SHADOW);
                    }
                    delayedTargetGuid.Clear();
                    break;
                default:
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shartuul_eye_of_shartuulAI(creature);
    }
};

class npc_shartuul_dreadmaw : public CreatureScript
{
public:
    npc_shartuul_dreadmaw() : CreatureScript("npc_shartuul_dreadmaw") { }

    struct npc_shartuul_dreadmawAI : public ShartuulPhaseThreeBossAI
    {
        npc_shartuul_dreadmawAI(Creature* creature) : ShartuulPhaseThreeBossAI(creature) { }

        void Reset() override
        {
            ShartuulPhaseThreeBossAI::Reset();
            ApplySniffedPhaseThreeCreatureState(me);
            me->SetObjectScale(1.0f);
        }

        void ScheduleBossEvents() override
        {
            bossEvents.ScheduleEvent(EVENT_SHARTUUL_DREADMAW_GROWTH, 10s);
            bossEvents.ScheduleEvent(EVENT_SHARTUUL_DREADMAW_CHARGE, 6s);
        }

        void UpdateAI(uint32 diff) override
        {
            if (UpdateEventStun(diff))
                return;

            Unit* target = GetBossTarget();
            if (!target)
            {
                me->AttackStop();
                me->CombatStop(true);
                return;
            }

            if (!bossStarted)
                StartBossEvents();

            me->SetReactState(REACT_AGGRESSIVE);
            me->SetInCombatWith(target);
            target->SetInCombatWith(me);
            me->AddThreat(target, 100000.0f);

            float const dist = me->GetExactDist2d(target);
            if (dist > 5.0f)
                me->GetMotionMaster()->MoveChase(target);

            bossEvents.Update(diff);
            while (uint32 eventId = bossEvents.ExecuteEvent())
                ExecuteBossEvent(eventId);

            if (dist <= 7.0f)
                DoMeleeAttackIfReady();
        }

        void ExecuteBossEvent(uint32 eventId) override
        {
            Unit* target = GetBossTarget();
            if (!target)
                return;

            switch (eventId)
            {
                case EVENT_SHARTUUL_DREADMAW_GROWTH:
                    me->CastSpell(me, SPELL_SHARTUUL_DREADMAW_GROWTH, true);
                    me->SetObjectScale(std::min(2.2f, me->GetObjectScale() * 1.15f));
                    bossEvents.ScheduleEvent(EVENT_SHARTUUL_DREADMAW_GROWTH, 18s);
                    break;
                case EVENT_SHARTUUL_DREADMAW_CHARGE:
                    CastVisual(target, SPELL_SHARTUUL_GENERIC_CHARGE, true);
                    delayedTargetGuid = target->GetGUID();
                    me->GetMotionMaster()->MoveCharge(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 24.0f);
                    bossEvents.ScheduleEvent(EVENT_SHARTUUL_DREADMAW_CHARGE_HIT, 900ms);
                    bossEvents.ScheduleEvent(EVENT_SHARTUUL_DREADMAW_CHARGE, 16s);
                    break;
                case EVENT_SHARTUUL_DREADMAW_CHARGE_HIT:
                    if (Unit* delayedTarget = ObjectAccessor::GetUnit(*me, delayedTargetGuid))
                    {
                        DamageTarget(delayedTarget, urand(14000, 18000), SPELL_SCHOOL_MASK_NORMAL);
                        me->AddAura(SPELL_SHARTUUL_GENERIC_IMMOLATE, delayedTarget);
                    }
                    delayedTargetGuid.Clear();
                    KeepBossCombat();
                    break;
                default:
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shartuul_dreadmawAI(creature);
    }
};

class npc_shartuul_eredar : public CreatureScript
{
public:
    npc_shartuul_eredar() : CreatureScript("npc_shartuul_eredar") { }

    struct npc_shartuul_eredarAI : public ShartuulPhaseThreeBossAI
    {
        npc_shartuul_eredarAI(Creature* creature) : ShartuulPhaseThreeBossAI(creature) { }

        void Reset() override
        {
            ShartuulPhaseThreeBossAI::Reset();
            ApplySniffedPhaseThreeCreatureState(me);
        }

        void ScheduleBossEvents() override
        {
            bossEvents.ScheduleEvent(EVENT_SHARTUUL_EREDAR_SHADOW_BOLT, 3s);
            bossEvents.ScheduleEvent(EVENT_SHARTUUL_EREDAR_SHADOW_RESONANCE, 9s);
            bossEvents.ScheduleEvent(EVENT_SHARTUUL_EREDAR_IMMOLATE, 14s);
            bossEvents.ScheduleEvent(EVENT_SHARTUUL_EREDAR_MAGNETIC_PULL, 19s);
        }

        void ExecuteBossEvent(uint32 eventId) override
        {
            Unit* target = GetBossTarget();
            if (!target)
                return;

            switch (eventId)
            {
                case EVENT_SHARTUUL_EREDAR_SHADOW_BOLT:
                    CastVisual(target, SPELL_SHARTUUL_GENERIC_SHADOW_BOLT, false);
                    DamageTarget(target, urand(3200, 4000), SPELL_SCHOOL_MASK_SHADOW);
                    bossEvents.ScheduleEvent(EVENT_SHARTUUL_EREDAR_SHADOW_BOLT, 4s);
                    break;
                case EVENT_SHARTUUL_EREDAR_SHADOW_RESONANCE:
                    CastVisual(target, SPELL_SHARTUUL_SHADOW_RESONANCE, true);
                    bossEvents.ScheduleEvent(EVENT_SHARTUUL_EREDAR_SHADOW_RESONANCE, 18s);
                    break;
                case EVENT_SHARTUUL_EREDAR_IMMOLATE:
                    CastVisual(target, SPELL_SHARTUUL_GENERIC_IMMOLATE, false);
                    bossEvents.ScheduleEvent(EVENT_SHARTUUL_EREDAR_INCINERATE_START, 4s);
                    bossEvents.ScheduleEvent(EVENT_SHARTUUL_EREDAR_IMMOLATE, 28s);
                    break;
                case EVENT_SHARTUUL_EREDAR_INCINERATE_START:
                    if (Player* player = ObjectAccessor::GetPlayer(*me, target->GetCharmerGUID()))
                        player->GetSession()->SendAreaTriggerMessage("Shartuul begins casting Incinerate!");
                    SendShartuulCastBar(me, target, SPELL_SHARTUUL_EREDAR_INCINERATE, 7000);
                    CastVisual(target, SPELL_SHARTUUL_EREDAR_INCINERATE, true);
                    delayedTargetGuid = target->GetGUID();
                    bossEvents.ScheduleEvent(EVENT_SHARTUUL_EREDAR_INCINERATE_HIT, 7s);
                    break;
                case EVENT_SHARTUUL_EREDAR_INCINERATE_HIT:
                    if (Unit* delayedTarget = ObjectAccessor::GetUnit(*me, delayedTargetGuid))
                        DamageTarget(delayedTarget, std::max<uint32>(15000, delayedTarget->GetMaxHealth() * 40 / 100), SPELL_SCHOOL_MASK_FIRE);
                    delayedTargetGuid.Clear();
                    break;
                case EVENT_SHARTUUL_EREDAR_MAGNETIC_PULL:
                {
                    Position const& blink = ShartuulBlinkPositions[urand(0, std::size(ShartuulBlinkPositions) - 1)];
                    Position old = me->GetPosition();
                    target->NearTeleportTo(old.GetPositionX(), old.GetPositionY(), old.GetPositionZ(), target->GetOrientation());
                    me->NearTeleportTo(blink.GetPositionX(), blink.GetPositionY(), blink.GetPositionZ(), blink.GetOrientation());
                    KeepBossCombat();
                    bossEvents.ScheduleEvent(EVENT_SHARTUUL_EREDAR_MAGNETIC_PULL, 24s);
                    break;
                }
                default:
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shartuul_eredarAI(creature);
    }
};

class npc_shartuul_event_controller : public CreatureScript
{
public:
    npc_shartuul_event_controller() : CreatureScript("npc_shartuul_event_controller") { }

    struct npc_shartuul_event_controllerAI : public ScriptedAI
    {
        npc_shartuul_event_controllerAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            Reset();
        }

        EventMap events;
        SummonList summons;
        ObjectGuid playerGuid;
        ObjectGuid degraderGuid;
        ObjectGuid captiveDemonGuid;
        ObjectGuid nextCaptiveDemonGuid;
        ObjectGuid shieldHelperGuids[SHARTUUL_SHIELD_HELPER_COUNT];
        ObjectGuid testPossessGuids[3];
        ObjectGuid testWaveGuids[9];
        ObjectGuid testPortalVisualGuids[std::size(ShartuulTestPortalVisualSpells)];
        std::vector<ObjectGuid> eventGameObjectGuids;
        std::vector<ObjectGuid> activeWaveGuids;
        bool eventActive;
        bool phaseOneStarted;
        bool phaseTwoStarted;
        bool phaseTwoRewarded;
        bool doomguardControlledPhaseStarted;
        bool doomguardControlledPhaseComplete;
        bool shivanControlledPhaseStarted;
        bool forceFreshDegrader;
        bool waveTransitionPending;
        bool transferringDemonControl;
        bool shivanReleased;
        ObjectGuid phaseThreeBossGuid;
        ObjectGuid phaseThreeSummonerGuid;
        ObjectGuid pendingCannonSummonerGuid;
        ObjectGuid pendingMoargShieldCasterGuid;
        ObjectGuid pendingMoargShieldTargetGuid;
        uint8 patrolPoint;
        uint8 currentWave;
        uint8 doomguardControlledWave;
        uint8 phaseThreeBossStage;
        uint8 shieldRemainingPct;
        uint8 doomguardFelFlamesTicks;

        void Reset() override
        {
            events.Reset();
            summons.DespawnAll();
            DespawnEventGameObjects();
            activeWaveGuids.clear();
            playerGuid.Clear();
            degraderGuid.Clear();
            captiveDemonGuid.Clear();
            nextCaptiveDemonGuid.Clear();
            for (ObjectGuid& guid : shieldHelperGuids)
                guid.Clear();
            for (ObjectGuid& guid : testPossessGuids)
                guid.Clear();
            for (ObjectGuid& guid : testWaveGuids)
                guid.Clear();
            for (ObjectGuid& guid : testPortalVisualGuids)
                guid.Clear();
            eventActive = false;
            phaseOneStarted = false;
            phaseTwoStarted = false;
            phaseTwoRewarded = false;
            doomguardControlledPhaseStarted = false;
            doomguardControlledPhaseComplete = false;
            shivanControlledPhaseStarted = false;
            forceFreshDegrader = false;
            waveTransitionPending = false;
            transferringDemonControl = false;
            shivanReleased = false;
            phaseThreeBossGuid.Clear();
            phaseThreeSummonerGuid.Clear();
            pendingCannonSummonerGuid.Clear();
            pendingMoargShieldCasterGuid.Clear();
            pendingMoargShieldTargetGuid.Clear();
            patrolPoint = 0;
            currentWave = 0;
            doomguardControlledWave = 0;
            phaseThreeBossStage = 0;
            shieldRemainingPct = 100;
            doomguardFelFlamesTicks = 0;
            me->SetReactState(REACT_PASSIVE);
            me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
            SpawnIdleState();
        }

        void JustSummoned(Creature* summon) override
        {
            summons.Summon(summon);
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            if (summon->GetEntry() == NPC_DOOMGUARD_PUNISHER && !summon->IsAlive())
                CompletePhaseTwo();
            else if (summon->GetEntry() == NPC_SHARTUUL_SHIVAN_ASSASSIN && shivanReleased && !summon->IsAlive())
                CompleteShivanPhase();
            else if (summon->GetEntry() == NPC_EYE_OF_SHARTUUL_TRANSFORM || summon->GetEntry() == NPC_EYE_OF_SHARTUUL || summon->GetEntry() == NPC_DREADMAW || summon->GetEntry() == NPC_EREDAR_SHARTUUL)
                CompletePhaseThreeBoss(summon->GetEntry());

            summons.Despawn(summon);
            activeWaveGuids.erase(std::remove(activeWaveGuids.begin(), activeWaveGuids.end(), summon->GetGUID()), activeWaveGuids.end());
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            if (summon->GetEntry() == NPC_DOOMGUARD_PUNISHER)
                CompletePhaseTwo();
            else if (summon->GetEntry() == NPC_SHARTUUL_SHIVAN_ASSASSIN && shivanReleased)
                CompleteShivanPhase();
            else if (summon->GetEntry() == NPC_EYE_OF_SHARTUUL_TRANSFORM || summon->GetEntry() == NPC_EYE_OF_SHARTUUL || summon->GetEntry() == NPC_DREADMAW || summon->GetEntry() == NPC_EREDAR_SHARTUUL)
                CompletePhaseThreeBoss(summon->GetEntry());
        }

        void TrackEventGameObject(GameObject* gameObject)
        {
            if (gameObject)
                eventGameObjectGuids.push_back(gameObject->GetGUID());
        }

        void DespawnEventGameObjects()
        {
            for (ObjectGuid const& guid : eventGameObjectGuids)
                if (GameObject* gameObject = ObjectAccessor::GetGameObject(*me, guid))
                    gameObject->Delete();

            eventGameObjectGuids.clear();
        }

        void SetGUID(ObjectGuid const& guid, int32 id) override
        {
            if (id == DATA_SHARTUUL_PLAYER)
                playerGuid = guid;
            else if (id == DATA_SHARTUUL_DEGRADER)
                degraderGuid = guid;
        }

        void DoAction(int32 action) override
        {
            if (action == ACTION_SHARTUUL_START_PHASE_ONE)
                StartPhaseOne();
            else if (action == ACTION_SHARTUUL_RESET && !transferringDemonControl)
                FailEvent();
            else if (action == ACTION_SHARTUUL_TEST_EYE_INTRO)
                StartEyePhaseIntroTest();
        }

        void SpawnIdleState()
        {
            eventActive = false;
            phaseOneStarted = false;
            phaseTwoStarted = false;
            phaseTwoRewarded = false;
            doomguardControlledPhaseStarted = false;
            doomguardControlledPhaseComplete = false;
            shivanControlledPhaseStarted = false;
            waveTransitionPending = false;
            transferringDemonControl = false;
            shivanReleased = false;
            phaseThreeBossGuid.Clear();
            phaseThreeSummonerGuid.Clear();
            pendingCannonSummonerGuid.Clear();
            pendingMoargShieldCasterGuid.Clear();
            pendingMoargShieldTargetGuid.Clear();
            currentWave = 0;
            doomguardControlledWave = 0;
            phaseThreeBossStage = 0;
            shieldRemainingPct = 100;
            doomguardFelFlamesTicks = 0;

            if (Creature* next = ObjectAccessor::GetCreature(*me, nextCaptiveDemonGuid))
                next->DespawnOrUnsummon();

            nextCaptiveDemonGuid.Clear();

            if (EnsureIdleDegrader())
                StartIdlePatrol();

            Creature* captive = GetOrCreateCaptiveDemon();
            if (captive)
            {
                captiveDemonGuid = captive->GetGUID();
                MakeCaptive(captive);
            }

            EnsureShieldHelpers();
            SpawnTestHarness(true);
            SpawnPortalVisualTestHarness();
            events.CancelEvent(EVENT_SHARTUUL_ARENA_VISUALS);
            events.ScheduleEvent(EVENT_SHARTUUL_ARENA_VISUALS, 1s);
            events.CancelEvent(EVENT_SHARTUUL_TEST_BOSS_ABILITIES);
        }

        void PrepareTestWaveDemon(Creature* demon, float scale = 1.0f)
        {
            if (!demon)
                return;

            demon->SetPhaseMask(PHASEMASK_NORMAL, true);
            demon->SetFaction(FactionShartuulWaveDemon);
            demon->SetObjectScale(scale);
            demon->SetReactState(REACT_DEFENSIVE);
            demon->SetUInt32Value(UNIT_FIELD_BYTES_1, 0);
            demon->AI()->SetGUID(ObjectGuid::Empty, DATA_SHARTUUL_DEGRADER);
            demon->CombatStop(true);
            demon->AttackStop();
            demon->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
            demon->AI()->SetData(DATA_SHARTUUL_TEST_HARNESS_CREATURE, 1);

            switch (demon->GetEntry())
            {
                case NPC_SHARTUUL_GANARG_UNDERLING:
                    demon->SetDisplayId(demon->GetNativeDisplayId());
                    break;
                case NPC_SHARTUUL_MOARG_TORMENTER:
                    demon->SetObjectScale(0.5f);
                    demon->SetMaxHealth(std::max<uint32>(demon->GetMaxHealth(), 45000));
                    demon->SetHealth(demon->GetMaxHealth());
                    demon->SetHomePosition(demon->GetPosition());
                    break;
                case NPC_SHARTUUL_PORTABLE_FEL_CANNON:
                    demon->SetDisplayId(MODEL_SHARTUUL_PORTABLE_FEL_CANNON);
                    demon->SetObjectScale(0.5f);
                    demon->SetMaxHealth(3500);
                    demon->SetHealth(3500);
                    demon->SetControlled(true, UNIT_STATE_ROOT);
                    break;
                case NPC_EYE_OF_SHARTUUL:
                    ApplySniffedPhaseThreeCreatureState(demon);
                    demon->SetMaxHealth(120000);
                    demon->SetHealth(120000);
                    break;
                case NPC_DREADMAW:
                    ApplySniffedPhaseThreeCreatureState(demon);
                    demon->SetObjectScale(1.0f);
                    demon->SetMaxHealth(130000);
                    demon->SetHealth(130000);
                    break;
                case NPC_EREDAR_SHARTUUL:
                    ApplySniffedPhaseThreeCreatureState(demon);
                    demon->SetMaxHealth(160000);
                    demon->SetHealth(160000);
                    break;
                case NPC_EYE_TENTACLE:
                    demon->SetMaxHealth(12000);
                    demon->SetHealth(12000);
                    demon->SetControlled(true, UNIT_STATE_ROOT);
                    break;
                default:
                    demon->SetDisplayId(demon->GetNativeDisplayId());
                    break;
            }

            demon->GetMotionMaster()->Clear();
            demon->GetMotionMaster()->MoveIdle();
            demon->CombatStop(true);
            demon->AttackStop();
            demon->SetReactState(REACT_DEFENSIVE);
            demon->SetHomePosition(demon->GetPosition());
            demon->SetRegeneratingHealth(false);
        }

        Creature* GetAliveManagedTestCreature(ObjectGuid& guid)
        {
            if (Creature* creature = ObjectAccessor::GetCreature(*me, guid))
            {
                if (creature->IsAlive())
                    return creature;

                creature->DespawnOrUnsummon();
            }

            guid.Clear();
            return nullptr;
        }

        void CleanupDuplicateTestCreatures(uint32 entry, ObjectGuid keepGuid, Position const& pos, float range)
        {
            std::list<Creature*> creatures;
            GetCreatureListWithEntryInGrid(creatures, me, entry, range);
            for (Creature* creature : creatures)
            {
                if (creature->GetGUID() != keepGuid && creature->GetExactDist2d(pos.GetPositionX(), pos.GetPositionY()) <= range)
                    creature->DespawnOrUnsummon();
            }
        }

        void SpawnTestHarness(bool includePossessDemons)
        {
            if (includePossessDemons)
            {
                uint32 const possessEntries[] =
                {
                    NPC_FELGUARD_DEGRADER,
                    NPC_DOOMGUARD_PUNISHER,
                    NPC_SHARTUUL_SHIVAN_ASSASSIN
                };

                for (uint8 i = 0; i < std::size(possessEntries); ++i)
                {
                    if (Creature* keep = GetAliveManagedTestCreature(testPossessGuids[i]))
                    {
                        CleanupDuplicateTestCreatures(possessEntries[i], keep->GetGUID(), ShartuulTestPossessSpawn[i], 25.0f);
                        continue;
                    }

                    if (Creature* demon = me->SummonCreature(possessEntries[i], ShartuulTestPossessSpawn[i], TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        testPossessGuids[i] = demon->GetGUID();
                        PrepareShartuulTestPossessDemon(demon);
                    }
                }
            }

            uint32 const waveEntries[] =
            {
                NPC_SHARTUUL_WAVE_FELHOUND,
                NPC_SHARTUUL_WAVE_IMP,
                NPC_SHARTUUL_GANARG_UNDERLING,
                NPC_SHARTUUL_MOARG_TORMENTER,
                NPC_SHARTUUL_PORTABLE_FEL_CANNON
            };

            for (uint8 i = 0; i < std::size(waveEntries); ++i)
            {
                if (Creature* keep = GetAliveManagedTestCreature(testWaveGuids[i]))
                {
                    PrepareTestWaveDemon(keep, waveEntries[i] == NPC_SHARTUUL_MOARG_TORMENTER ? 0.5f : 1.0f);
                    CleanupDuplicateTestCreatures(waveEntries[i], keep->GetGUID(), ShartuulTestWaveSpawn[i], 40.0f);
                    continue;
                }

                if (waveEntries[i] == NPC_SHARTUUL_MOARG_TORMENTER || waveEntries[i] == NPC_EYE_OF_SHARTUUL_TRANSFORM || waveEntries[i] == NPC_DREADMAW || waveEntries[i] == NPC_EREDAR_SHARTUUL || waveEntries[i] == NPC_EYE_TENTACLE)
                    CleanupDuplicateTestCreatures(waveEntries[i], ObjectGuid::Empty, ShartuulTestWaveSpawn[i], 40.0f);

                if (Creature* wave = me->SummonCreature(waveEntries[i], ShartuulTestWaveSpawn[i], TEMPSUMMON_MANUAL_DESPAWN))
                {
                    testWaveGuids[i] = wave->GetGUID();
                    PrepareTestWaveDemon(wave, waveEntries[i] == NPC_SHARTUUL_MOARG_TORMENTER ? 0.5f : 1.0f);
                }
            }

            SpawnExplicitTestBoss(5, NPC_EYE_OF_SHARTUUL, ShartuulTestWaveSpawn[5], 1.0f);
            SpawnExplicitTestBoss(6, NPC_DREADMAW, ShartuulTestWaveSpawn[6], 1.35f);
            SpawnExplicitTestBoss(7, NPC_EREDAR_SHARTUUL, ShartuulTestWaveSpawn[7], 1.0f);
            SpawnExplicitTestBoss(8, NPC_EYE_TENTACLE, ShartuulTestWaveSpawn[8], 1.0f);
        }

        void SpawnExplicitTestBoss(uint8 slot, uint32 entry, Position const& pos, float scale)
        {
            if (slot >= std::size(testWaveGuids))
                return;

            if (Creature* keep = GetAliveManagedTestCreature(testWaveGuids[slot]))
            {
                if (keep->GetEntry() == entry)
                {
                    PrepareTestWaveDemon(keep, scale);
                    CleanupDuplicateTestCreatures(entry, keep->GetGUID(), pos, 60.0f);
                    return;
                }

                keep->DespawnOrUnsummon();
                testWaveGuids[slot].Clear();
            }

            CleanupDuplicateTestCreatures(entry, ObjectGuid::Empty, pos, 60.0f);
            if (Creature* boss = me->SummonCreature(entry, pos, TEMPSUMMON_MANUAL_DESPAWN))
            {
                testWaveGuids[slot] = boss->GetGUID();
                PrepareTestWaveDemon(boss, scale);
            }
        }

        void SpawnPortalVisualTestHarness()
        {
            for (uint8 i = 0; i < std::size(ShartuulTestPortalVisualSpells); ++i)
            {
                if (GetAliveManagedTestCreature(testPortalVisualGuids[i]))
                    continue;

                if (Creature* visual = me->SummonCreature(NPC_WORLD_EVENT_GENERATOR, ShartuulTestPortalVisualSpawn[i], TEMPSUMMON_MANUAL_DESPAWN))
                {
                    testPortalVisualGuids[i] = visual->GetGUID();
                    PreparePortalVisualAnchor(visual);
                }
            }

            RefreshPortalVisualTestHarness();
        }

        void RefreshPortalVisualTestHarness()
        {
            for (uint8 i = 0; i < std::size(ShartuulTestPortalVisualSpells); ++i)
            {
                Creature* visual = ObjectAccessor::GetCreature(*me, testPortalVisualGuids[i]);
                if (!visual || !visual->IsAlive())
                    continue;

                visual->CastSpell(visual, ShartuulTestPortalVisualSpells[i], true);
            }

            events.ScheduleEvent(EVENT_SHARTUUL_TEST_PORTAL_VISUALS, 5s);
        }

        void UpdateTestBossAbilities()
        {
            for (uint8 slot = 5; slot < std::size(testWaveGuids); ++slot)
            {
                Creature* boss = ObjectAccessor::GetCreature(*me, testWaveGuids[slot]);
                if (!boss || !boss->IsAlive() || !boss->IsInCombat())
                    continue;

                Unit* target = boss->GetVictim();
                if (!IsPossessedShartuulControlledDemon(target))
                    continue;

                switch (boss->GetEntry())
                {
                    case NPC_EYE_OF_SHARTUUL_TRANSFORM:
                    case NPC_EYE_OF_SHARTUUL:
                    {
                        uint32 roll = urand(0, 2);
                        if (roll == 0)
                        {
                            boss->CastSpell(target, SPELL_SHARTUUL_GENERIC_FIREBALL, false);
                            Unit::DealDamage(boss, target, 3500, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE);
                        }
                        else if (roll == 1)
                        {
                            boss->CastSpell(target, SPELL_SHARTUUL_SHADOW_RESONANCE, true);
                            Unit::DealDamage(boss, target, 2500, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_SHADOW);
                        }
                        else
                        {
                            boss->SetFacingToObject(target);
                            boss->CastSpell(target, SPELL_SHARTUUL_EYE_DARK_GLARE, true);
                            Unit::DealDamage(boss, target, 7000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_SHADOW);
                        }
                        break;
                    }
                    case NPC_DREADMAW:
                        if (boss->GetDistance(target) > 12.0f)
                        {
                            boss->CastSpell(target, SPELL_SHARTUUL_GENERIC_CHARGE, true);
                            boss->GetMotionMaster()->MoveCharge(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 24.0f);
                        }
                        else
                            Unit::DealDamage(boss, target, urand(4500, 7000), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL);

                        if (urand(0, 3) == 0)
                            boss->CastSpell(boss, SPELL_SHARTUUL_DREADMAW_GROWTH, true);
                        break;
                    case NPC_EREDAR_SHARTUUL:
                    {
                        uint32 roll = urand(0, 4);
                        if (roll == 0)
                        {
                            SendShartuulCastBar(boss, target, SPELL_SHARTUUL_EREDAR_INCINERATE, 7000);
                            boss->CastSpell(target, SPELL_SHARTUUL_EREDAR_INCINERATE, true);
                            Unit::DealDamage(boss, target, std::max<uint32>(15000, target->GetMaxHealth() * 40 / 100), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE);
                        }
                        else if (roll == 1)
                            boss->CastSpell(target, SPELL_SHARTUUL_GENERIC_IMMOLATE, false);
                        else if (roll == 2)
                            boss->CastSpell(target, SPELL_SHARTUUL_SHADOW_RESONANCE, true);
                        else
                        {
                            boss->CastSpell(target, SPELL_SHARTUUL_GENERIC_SHADOW_BOLT, false);
                            Unit::DealDamage(boss, target, 3500, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_SHADOW);
                        }
                        break;
                    }
                    case NPC_EYE_TENTACLE:
                        boss->SetFacingToObject(target);
                        boss->CastSpell(target, SPELL_SHARTUUL_GENERIC_MIND_FLAY, false);
                        Unit::DealDamage(boss, target, 7500, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_SHADOW);
                        break;
                    default:
                        break;
                }
            }
        }

        Creature* EnsureIdleDegrader()
        {
            std::list<Creature*> degraders;
            GetCreatureListWithEntryInGrid(degraders, me, NPC_FELGUARD_DEGRADER, 90.0f);

            if (forceFreshDegrader)
            {
                for (Creature* degrader : degraders)
                    degrader->DespawnOrUnsummon();

                degraders.clear();
                forceFreshDegrader = false;
            }

            Creature* keep = nullptr;
            float keepDist = 0.0f;
            for (Creature* degrader : degraders)
            {
                if (!degrader->IsAlive())
                {
                    degrader->DespawnOrUnsummon();
                    continue;
                }

                float dist = degrader->GetExactDist2d(ShartuulDegraderSpawn.GetPositionX(), ShartuulDegraderSpawn.GetPositionY());
                if (!keep || dist < keepDist)
                {
                    if (keep)
                        keep->DespawnOrUnsummon();

                    keep = degrader;
                    keepDist = dist;
                }
                else
                    degrader->DespawnOrUnsummon();
            }

            if (keep)
            {
                degraderGuid = keep->GetGUID();
                PrepareIdleDegrader(keep);
                keep->NearTeleportTo(ShartuulDegraderSpawn.GetPositionX(), ShartuulDegraderSpawn.GetPositionY(), ShartuulDegraderSpawn.GetPositionZ(), ShartuulDegraderSpawn.GetOrientation());
                patrolPoint = 1;
                return keep;
            }

            if (Creature* degrader = me->SummonCreature(NPC_FELGUARD_DEGRADER, ShartuulDegraderSpawn, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000))
            {
                degraderGuid = degrader->GetGUID();
                PrepareIdleDegrader(degrader);
                patrolPoint = 1;
                return degrader;
            }

            return nullptr;
        }

        Creature* GetOrCreateCaptiveDemon(uint32 entry = NPC_DOOMGUARD_PUNISHER)
        {
            std::list<Creature*> captives;
            GetCreatureListWithEntryInGrid(captives, me, entry, 90.0f);

            Creature* keep = nullptr;
            float keepDist = 0.0f;
            for (Creature* captive : captives)
            {
                if (!keep)
                {
                    keep = captive;
                    keepDist = captive->GetExactDist2d(ShartuulCaptiveDemonSpawn.GetPositionX(), ShartuulCaptiveDemonSpawn.GetPositionY());
                    continue;
                }

                float dist = captive->GetExactDist2d(ShartuulCaptiveDemonSpawn.GetPositionX(), ShartuulCaptiveDemonSpawn.GetPositionY());
                if (dist < keepDist)
                {
                    keep->DespawnOrUnsummon();
                    keep = captive;
                    keepDist = dist;
                }
                else
                    captive->DespawnOrUnsummon();
            }

            return keep ? keep : me->SummonCreature(entry, ShartuulCaptiveDemonSpawn, TEMPSUMMON_MANUAL_DESPAWN);
        }

        Creature* GetOrCreateNextCaptiveDemon()
        {
            Creature* next = GetOrCreateCaptiveDemon(NPC_SHARTUUL_SHIVAN_ASSASSIN);
            if (!next)
                return nullptr;

            nextCaptiveDemonGuid = next->GetGUID();
            MakeCaptive(next);
            return next;
        }

        Position GetShieldHelperPosition(uint8 /*index*/) const
        {
            return ShartuulShieldCenter;
        }

        Creature* EnsureShieldHelper(uint8 index)
        {
            if (index >= SHARTUUL_SHIELD_HELPER_COUNT)
                return nullptr;

            if (Creature* helper = ObjectAccessor::GetCreature(*me, shieldHelperGuids[index]))
                return helper;

            Position pos = GetShieldHelperPosition(index);
            std::list<Creature*> helpers;
            GetCreatureListWithEntryInGrid(helpers, me, NPC_LEGION_RING_INVISMAN_LG, 40.0f);
            for (Creature* helper : helpers)
            {
                if (helper->GetExactDist2d(pos.GetPositionX(), pos.GetPositionY()) > 15.0f)
                    continue;

                if (shieldHelperGuids[index].IsEmpty())
                {
                    shieldHelperGuids[index] = helper->GetGUID();
                    helper->SetObjectScale(1.0f);
                    helper->SetPhaseMask(PHASEMASK_NORMAL, true);
                    helper->SetReactState(REACT_PASSIVE);
                    helper->SetControlled(true, UNIT_STATE_ROOT);
                    helper->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
                    helper->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
                    return helper;
                }

                helper->DespawnOrUnsummon();
            }

            if (Creature* helper = me->SummonCreature(NPC_LEGION_RING_INVISMAN_LG, pos, TEMPSUMMON_MANUAL_DESPAWN))
            {
                shieldHelperGuids[index] = helper->GetGUID();
                helper->SetObjectScale(1.0f);
                helper->SetPhaseMask(PHASEMASK_NORMAL, true);
                helper->SetReactState(REACT_PASSIVE);
                helper->SetControlled(true, UNIT_STATE_ROOT);
                helper->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
                helper->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
                return helper;
            }

            return nullptr;
        }

        void EnsureShieldHelpers()
        {
            for (uint8 i = 0; i < SHARTUUL_SHIELD_HELPER_COUNT; ++i)
                EnsureShieldHelper(i);
        }

        void DespawnEventSummonsExcept(ObjectGuid const& keepGuid)
        {
            SummonList::StorageType summonGuids;
            for (ObjectGuid const& guid : summons)
                if (guid != keepGuid)
                    summonGuids.push_back(guid);

            for (ObjectGuid const& guid : summonGuids)
                if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                    summon->DespawnOrUnsummon();
        }

        void DisplayShieldPercent(Player* player, bool showMessage)
        {
            if (!player)
                return;

            player->SendUpdateWorldState(WORLD_STATE_BLACK_MORASS, 1);
            player->SendUpdateWorldState(WORLD_STATE_BLACK_MORASS_SHIELD, shieldRemainingPct);
            if (showMessage)
                player->GetSession()->SendAreaTriggerMessage("Shartuul's shield: {}% remaining.", shieldRemainingPct);
        }

        void PrepareIdleDegrader(Creature* degrader)
        {
            degrader->RemoveCharmAuras();
            degrader->RemoveAurasDueToSpell(SPELL_SHARTUUL_VISUAL_SHELL_SHIELD);
            degrader->CombatStop(true);
            degrader->SetFullHealth();
            degrader->SetPhaseMask(PHASEMASK_NORMAL, true);
            degrader->SetFaction(FactionShartuulIdle);
            degrader->SetWalk(true);
            degrader->SetControlled(false, UNIT_STATE_ROOT);
            degrader->ClearUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING);
            degrader->SetReactState(REACT_PASSIVE);
            degrader->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
            degrader->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE);
            degrader->GetMotionMaster()->Clear();
        }

        void StartIdlePatrol()
        {
            if (eventActive)
                return;

            events.CancelEvent(EVENT_SHARTUUL_IDLE_PATROL);
            events.ScheduleEvent(EVENT_SHARTUUL_IDLE_PATROL, 1s);
        }

        void MoveIdleDegrader()
        {
            Creature* degrader = ObjectAccessor::GetCreature(*me, degraderGuid);
            if (!degrader || eventActive || degrader->isPossessed())
                return;

            if (!degrader->IsAlive())
            {
                degrader->DespawnOrUnsummon();
                degraderGuid.Clear();
                events.ScheduleEvent(EVENT_SHARTUUL_RESPAWN_IDLE, 2s);
                return;
            }

            if (degrader->IsInCombat())
            {
                if (degrader->GetExactDist2d(ShartuulCenter.GetPositionX(), ShartuulCenter.GetPositionY()) > ShartuulDegraderLeashRange)
                {
                    degrader->CombatStop(true);
                    PrepareIdleDegrader(degrader);
                    degrader->NearTeleportTo(ShartuulDegraderSpawn.GetPositionX(), ShartuulDegraderSpawn.GetPositionY(), ShartuulDegraderSpawn.GetPositionZ(), ShartuulDegraderSpawn.GetOrientation());
                    patrolPoint = 0;
                }

                events.ScheduleEvent(EVENT_SHARTUUL_IDLE_PATROL, 2s);
                return;
            }

            degrader->SetReactState(REACT_PASSIVE);
            degrader->SetWalk(true);
            degrader->GetMotionMaster()->MovePoint(0, ShartuulDegraderPatrol[patrolPoint]);
            patrolPoint = (patrolPoint + 1) % std::size(ShartuulDegraderPatrol);
            events.ScheduleEvent(EVENT_SHARTUUL_IDLE_PATROL, 8s);
        }

        void MakeCaptive(Creature* creature)
        {
            creature->SetReactState(REACT_PASSIVE);
            creature->CombatStop(true);
            creature->SetPhaseMask(PHASEMASK_NORMAL, true);
            creature->RemoveAurasDueToSpell(SPELL_SHARTUUL_VISUAL_SHELL_SHIELD);
            creature->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
            creature->SetUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
            creature->SetControlled(true, UNIT_STATE_ROOT);
            creature->NearTeleportTo(ShartuulCaptiveDemonSpawn.GetPositionX(), ShartuulCaptiveDemonSpawn.GetPositionY(), ShartuulCaptiveDemonSpawn.GetPositionZ(), ShartuulCaptiveDemonSpawn.GetOrientation());
        }

        void StartPhaseOne()
        {
            HandleHammerShieldHit();
        }

        void HandleHammerShieldHit()
        {
            if (!eventActive || phaseTwoStarted || phaseTwoRewarded)
                return;

            Creature* degrader = ObjectAccessor::GetCreature(*me, degraderGuid);
            Creature* shield = EnsureShieldHelper(0);
            Player* player = ObjectAccessor::GetPlayer(*me, playerGuid);
            if (!degrader || !shield)
                return;

            degrader->SetFacingToObject(shield);

            shieldRemainingPct = shieldRemainingPct > SHARTUUL_SHIELD_HAMMER_DAMAGE ? shieldRemainingPct - SHARTUUL_SHIELD_HAMMER_DAMAGE : 0;
            DisplayShieldPercent(player, true);

            SpawnGateVisuals();
            if (shieldRemainingPct == 0)
            {
                for (ObjectGuid const& guid : shieldHelperGuids)
                    if (Creature* helper = ObjectAccessor::GetCreature(*me, guid))
                        helper->RemoveAurasDueToSpell(SPELL_SHARTUUL_VISUAL_SHELL_SHIELD);

                StartPhaseTwo();
                return;
            }

            if (!phaseOneStarted)
            {
                phaseOneStarted = true;
                currentWave = 1;
                events.ScheduleEvent(EVENT_SHARTUUL_WAVE_ONE, 2s);
            }
        }

        void StartFromRune(Player* player, Unit* degrader)
        {
            if (!player || !degrader || degrader->GetEntry() != NPC_FELGUARD_DEGRADER)
                return;

            if (eventActive)
            {
                Creature* activeDegrader = ObjectAccessor::GetCreature(*me, degraderGuid);
                Player* activePlayer = ObjectAccessor::GetPlayer(*me, playerGuid);
                if (activePlayer && activeDegrader && activePlayer->isPossessing(activeDegrader))
                    return;

                FailEvent();
                degrader = EnsureIdleDegrader();
                if (!degrader)
                    return;
            }

            DespawnEventSummonsExcept(degrader->GetGUID());
            DespawnEventGameObjects();
            activeWaveGuids.clear();
            events.CancelEvent(EVENT_SHARTUUL_NEXT_WAVE);
            eventActive = true;
            phaseOneStarted = false;
            phaseTwoStarted = false;
            phaseTwoRewarded = false;
            currentWave = 0;
            shieldRemainingPct = 20;
            playerGuid = player->GetGUID();
            degraderGuid = degrader->GetGUID();
            DisplayShieldPercent(player, true);

            events.CancelEvent(EVENT_SHARTUUL_IDLE_PATROL);
            player->RemoveCharmAuras();
            degrader->RemoveCharmAuras();
            degrader->RemoveAurasDueToSpell(SPELL_SHARTUUL_VISUAL_SHELL_SHIELD);
            degrader->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
            degrader->SetPhaseMask(PHASEMASK_NORMAL, true);
            degrader->SetFaction(FactionShartuulControlled);
            degrader->SetWalk(false);
            player->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);

            degrader->SetFullHealth();
            degrader->ToCreature()->SetReactState(REACT_PASSIVE);
            degrader->GetMotionMaster()->Clear();
            player->CastSpell(degrader, SPELL_POSSESS, true);
            degrader->SetFaction(FactionShartuulControlled);

            if (Creature* captive = GetOrCreateCaptiveDemon())
            {
                captiveDemonGuid = captive->GetGUID();
                MakeCaptive(captive);
            }

            SpawnShieldVisuals();
            events.ScheduleEvent(EVENT_SHARTUUL_BOUNDARY_CHECK, 1s);
        }

        void SpawnShieldVisuals()
        {
            EnsureShieldHelpers();
            SpawnBoundaryLightning();
            events.CancelEvent(EVENT_SHARTUUL_ARENA_VISUALS);
            events.ScheduleEvent(EVENT_SHARTUUL_ARENA_VISUALS, 1s);
        }

        void SpawnBoundaryLightning()
        {
            for (uint8 side = 0; side < std::size(ShartuulRingRopeDummySpawn); ++side)
            {
                for (uint8 point = 0; point < SHARTUUL_BOUNDARY_POINTS_PER_SIDE; ++point)
                {
                    Position layerPos = GetShartuulBoundaryAnchorPosition(side, point);

                    if (point != 0 && point != SHARTUUL_BOUNDARY_POINTS_PER_SIDE - 1)
                    {
                        std::list<Creature*> dummies;
                        GetCreatureListWithEntryInGrid(dummies, me, NPC_SHARTUUL_BOUNDARY_ANCHOR, 140.0f);
                        for (Creature* dummy : dummies)
                            if (dummy->GetExactDist2d(layerPos.GetPositionX(), layerPos.GetPositionY()) < 1.0f
                                && std::fabs(dummy->GetPositionZ() - layerPos.GetPositionZ()) < 0.5f)
                                dummy->DespawnOrUnsummon();
                    }

                    bool alreadySpawned = false;
                    std::list<Creature*> dummies;
                    GetCreatureListWithEntryInGrid(dummies, me, NPC_SHARTUUL_BOUNDARY_ANCHOR, 140.0f);
                    for (Creature* dummy : dummies)
                    {
                        if (dummy->GetExactDist2d(layerPos.GetPositionX(), layerPos.GetPositionY()) < 1.0f
                            && std::fabs(dummy->GetPositionZ() - layerPos.GetPositionZ()) < 0.5f)
                        {
                            alreadySpawned = true;
                            break;
                        }
                    }

                    if (!alreadySpawned)
                    {
                        if (point != 0 && point != SHARTUUL_BOUNDARY_POINTS_PER_SIDE - 1)
                            continue;

                        if (Creature* dummy = me->SummonCreature(NPC_SHARTUUL_BOUNDARY_ANCHOR, layerPos, TEMPSUMMON_MANUAL_DESPAWN))
                        {
                            dummy->SetDisplayId(MODEL_INVISIBLE);
                            dummy->SetObjectScale(1.0f);
                            dummy->SetPhaseMask(PHASEMASK_NORMAL, true);
                            dummy->SetUInt32Value(UNIT_FIELD_BYTES_1, 16843008);
                            dummy->SetReactState(REACT_PASSIVE);
                            dummy->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
                            dummy->SetControlled(true, UNIT_STATE_ROOT);
                        }
                    }

                    if (point == 0 || point == SHARTUUL_BOUNDARY_POINTS_PER_SIDE - 1)
                        continue;

                    Position obeliskPos = GetShartuulBoundaryBasePosition(side, point);
                    std::list<GameObject*> obelisks;
                    me->GetGameObjectListWithEntryInGrid(obelisks, GO_SHARTUUL_LEGION_RING_OBELISK, 140.0f);
                    for (GameObject* obelisk : obelisks)
                        if (obelisk->GetExactDist2d(obeliskPos.GetPositionX(), obeliskPos.GetPositionY()) < 1.0f
                            && std::fabs(obelisk->GetPositionZ() - obeliskPos.GetPositionZ()) < 1.0f)
                            obelisk->Delete();
                }
            }
        }

        Creature* GetBoundaryLightningDummy(uint8 side, uint8 point) const
        {
            if (side >= std::size(ShartuulRingRopeDummySpawn))
                return nullptr;

            Position const pos = GetShartuulBoundaryAnchorPosition(side, point);
            std::list<Creature*> dummies;
            GetCreatureListWithEntryInGrid(dummies, me, NPC_SHARTUUL_BOUNDARY_ANCHOR, 140.0f);
            for (Creature* dummy : dummies)
                if (dummy->GetExactDist2d(pos.GetPositionX(), pos.GetPositionY()) < 1.0f
                    && std::fabs(dummy->GetPositionZ() - pos.GetPositionZ()) < 0.5f)
                    return dummy;

            return nullptr;
        }

        void RefreshBoundaryLightning()
        {
            for (uint8 side = 0; side < std::size(ShartuulRingRopeDummySpawn); ++side)
            {
                Creature* dummy = GetBoundaryLightningDummy(side, 0);
                Creature* target = GetBoundaryLightningDummy(side, SHARTUUL_BOUNDARY_POINTS_PER_SIDE - 1);
                if (!dummy || !target)
                    continue;

                dummy->SetPhaseMask(PHASEMASK_NORMAL, true);
                dummy->SetDisplayId(MODEL_INVISIBLE);
                dummy->SetUInt32Value(UNIT_FIELD_BYTES_1, 16843008);
                target->SetPhaseMask(PHASEMASK_NORMAL, true);
                target->SetDisplayId(MODEL_INVISIBLE);
                target->SetUInt32Value(UNIT_FIELD_BYTES_1, 16843008);

                dummy->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, target->GetGUID());
                dummy->SetUInt32Value(UNIT_CHANNEL_SPELL, side == 0 ? SPELL_SHARTUUL_BOUNDARY_TEST_40071 : SPELL_SHARTUUL_BOUNDARY_DRAIN_LIFE);
            }
        }

        void RefreshArenaVisuals()
        {
            for (uint8 i = 0; i < SHARTUUL_SHIELD_HELPER_COUNT; ++i)
            {
                Creature* helper = EnsureShieldHelper(i);
                if (!helper)
                    continue;

                if (!phaseTwoStarted && shieldRemainingPct > 0)
                    helper->CastSpell(helper, SPELL_SHARTUUL_VISUAL_SHELL_SHIELD, true);
                else
                    helper->RemoveAurasDueToSpell(SPELL_SHARTUUL_VISUAL_SHELL_SHIELD);
            }

            RefreshBoundaryLightning();

            if (eventActive)
                DisplayShieldPercent(ObjectAccessor::GetPlayer(*me, playerGuid), false);

            events.ScheduleEvent(EVENT_SHARTUUL_ARENA_VISUALS, 1s);
        }

        void PrepareVisualAnchor(Creature* anchor)
        {
            if (!anchor)
                return;

            anchor->SetDisplayId(MODEL_INVISIBLE);
            anchor->SetObjectScale(1.0f);
            anchor->SetPhaseMask(PHASEMASK_NORMAL, true);
            anchor->SetUInt32Value(UNIT_FIELD_BYTES_1, 16843008);
            anchor->SetReactState(REACT_PASSIVE);
            anchor->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
            anchor->SetControlled(true, UNIT_STATE_ROOT);
        }

        void PreparePortalVisualAnchor(Creature* anchor)
        {
            PrepareVisualAnchor(anchor);
            if (anchor)
                anchor->SetObjectScale(6.0f);
        }

        void SpawnGatePortalBeam(Position const& targetPos)
        {
            Position crystal = ShartuulGateCrystal;
            Position impact = targetPos;
            impact.m_positionZ += 0.75f;

            Creature* source = me->SummonCreature(NPC_WORLD_EVENT_GENERATOR, crystal, TEMPSUMMON_TIMED_DESPAWN, 2000);
            Creature* target = me->SummonCreature(NPC_WORLD_EVENT_GENERATOR, impact, TEMPSUMMON_TIMED_DESPAWN, 2000);
            if (!source || !target)
                return;

            PrepareVisualAnchor(source);
            PrepareVisualAnchor(target);
            source->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, target->GetGUID());
            source->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_GREEN_LIGHTNING);
        }

        void SpawnGatePortalVisual(Position const& pos)
        {
            SpawnGatePortalBeam(pos);
            if (GameObject* fog = me->SummonGameObject(GO_SHARTUUL_LEGION_RING_FOG, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), 0.0f, 0.0f, 0.0f, 1.0f, 9))
                TrackEventGameObject(fog);

            if (Creature* portal = me->SummonCreature(NPC_WORLD_EVENT_GENERATOR, pos, TEMPSUMMON_TIMED_DESPAWN, 7000))
            {
                PreparePortalVisualAnchor(portal);
                portal->CastSpell(portal, SPELL_SHARTUUL_OPEN_PORTAL_VISUAL, true);
            }
        }

        void SpawnShartuulSummonVisual(Creature* shartuul, Position const& targetPos)
        {
            if (!shartuul)
                return;

            Position impact = targetPos;
            impact.m_positionZ += 0.75f;
            if (Creature* target = me->SummonCreature(NPC_WORLD_EVENT_GENERATOR, impact, TEMPSUMMON_TIMED_DESPAWN, 3000))
            {
                PrepareVisualAnchor(target);
                shartuul->SetFacingTo(shartuul->GetAngle(impact.GetPositionX(), impact.GetPositionY()));
                shartuul->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST);
                shartuul->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, target->GetGUID());
                shartuul->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_GREEN_LIGHTNING);
            }

            if (Creature* portal = me->SummonCreature(NPC_WORLD_EVENT_GENERATOR, targetPos, TEMPSUMMON_TIMED_DESPAWN, 9000))
            {
                PreparePortalVisualAnchor(portal);
                portal->CastSpell(portal, SPELL_SHARTUUL_OPEN_PORTAL_VISUAL, true);
            }

            if (GameObject* fog = me->SummonGameObject(GO_SHARTUUL_LEGION_RING_FOG, targetPos.GetPositionX(), targetPos.GetPositionY(), targetPos.GetPositionZ(), targetPos.GetOrientation(), 0.0f, 0.0f, 0.0f, 1.0f, 9))
                TrackEventGameObject(fog);
        }

        void SpawnGateVisuals()
        {
        }

        void TrackWaveDemon(Creature* demon)
        {
            if (demon)
                activeWaveGuids.push_back(demon->GetGUID());
        }

        bool IsCurrentWaveCleared()
        {
            activeWaveGuids.erase(std::remove_if(activeWaveGuids.begin(), activeWaveGuids.end(),
                [this](ObjectGuid const& guid)
                {
                    Creature* demon = ObjectAccessor::GetCreature(*me, guid);
                    return !demon || !demon->IsAlive();
                }), activeWaveGuids.end());

            return activeWaveGuids.empty();
        }

        void CleanupDoomguardWaveTimers()
        {
            events.CancelEvent(EVENT_SHARTUUL_GANARG_CANNON_CHECK);
            events.CancelEvent(EVENT_SHARTUUL_GANARG_CANNON_FINISH);
            events.CancelEvent(EVENT_SHARTUUL_CANNON_FIRE);
            events.CancelEvent(EVENT_SHARTUUL_MOARG_ACID_GEYSER);
            events.CancelEvent(EVENT_SHARTUUL_GANARG_MOARG_SHIELD_CHECK);
            events.CancelEvent(EVENT_SHARTUUL_GANARG_MOARG_SHIELD_FINISH);
            RemoveMoargShield();
            pendingCannonSummonerGuid.Clear();
            pendingMoargShieldCasterGuid.Clear();
        }

        void ScheduleNextWaveIfCleared()
        {
            if (!eventActive || phaseTwoStarted || shieldRemainingPct == 0 || !IsCurrentWaveCleared())
                return;

            if (waveTransitionPending)
                return;

            waveTransitionPending = true;
            if (currentWave < SHARTUUL_PHASE_ONE_MAX_WAVE)
                ++currentWave;

            events.ScheduleEvent(EVENT_SHARTUUL_NEXT_WAVE, 5s);
        }

        void PrepareWaveDemon(Creature* demon, Creature* degrader, float scale = 1.0f)
        {
            if (!demon || !degrader)
                return;

            TrackWaveDemon(demon);
            demon->SetPhaseMask(PHASEMASK_NORMAL, true);
            demon->SetFaction(FactionShartuulWaveDemon);
            demon->SetObjectScale(scale);
            switch (demon->GetEntry())
            {
                case NPC_SHARTUUL_GANARG_UNDERLING:
                    demon->SetDisplayId(demon->GetNativeDisplayId());
                    break;
                case NPC_SHARTUUL_MOARG_TORMENTER:
                    demon->SetDisplayId(demon->GetNativeDisplayId());
                    break;
                case NPC_SHARTUUL_PORTABLE_FEL_CANNON:
                    demon->SetDisplayId(MODEL_SHARTUUL_PORTABLE_FEL_CANNON);
                    break;
                default:
                    demon->SetDisplayId(demon->GetNativeDisplayId());
                    break;
            }
            demon->SetReactState(REACT_AGGRESSIVE);
            demon->SetUInt32Value(UNIT_FIELD_BYTES_1, 0);
            if (demon->GetEntry() == NPC_SHARTUUL_MOARG_TORMENTER)
            {
                demon->SetRegeneratingHealth(false);
                demon->SetObjectScale(0.5f);
                demon->SetMaxHealth(std::max<uint32>(demon->GetMaxHealth(), 45000));
                demon->SetHealth(demon->GetMaxHealth());
            }
            demon->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
            if (demon->GetEntry() == NPC_SHARTUUL_PORTABLE_FEL_CANNON)
            {
                demon->SetObjectScale(0.5f);
                demon->SetMaxHealth(3500);
                demon->SetHealth(3500);
                demon->SetControlled(true, UNIT_STATE_ROOT);
            }
            demon->SetInCombatWith(degrader);
            degrader->SetInCombatWith(demon);
            demon->AddThreat(degrader, 100000.0f);
            demon->AI()->SetGUID(degrader->GetGUID(), DATA_SHARTUUL_DEGRADER);
            demon->AI()->AttackStart(degrader);
            demon->GetMotionMaster()->MoveChase(degrader);
        }

        void PrepareWaveImp(Creature* imp, Creature* degrader)
        {
            PrepareWaveDemon(imp, degrader);
            imp->GetMotionMaster()->MoveChase(degrader, 28.0f);
        }

        void UpdateWaveImps()
        {
            Creature* degrader = ObjectAccessor::GetCreature(*me, degraderGuid);
            if (!eventActive || !degrader || !degrader->IsAlive())
                return;

            for (ObjectGuid const& guid : summons)
            {
                Creature* imp = ObjectAccessor::GetCreature(*me, guid);
                if (!imp || !imp->IsAlive() || imp->GetEntry() != NPC_SHARTUUL_WAVE_IMP)
                    continue;

                imp->SetFaction(FactionShartuulWaveDemon);
                imp->SetInCombatWith(degrader);
                degrader->SetInCombatWith(imp);
                imp->AddThreat(degrader, 100000.0f);

                if (imp->HasUnitState(UNIT_STATE_CASTING))
                    continue;

                float const distance = imp->GetExactDist2d(degrader);
                if (distance > 28.0f)
                {
                    imp->GetMotionMaster()->MoveChase(degrader, 28.0f);
                    continue;
                }

                imp->StopMoving();
                imp->SetFacingToObject(degrader);
                if (imp->CastCustomSpell(SPELL_SHARTUUL_IMP_FIREBOLT, SPELLVALUE_BASE_POINT0, urand(1100, 1300), degrader, false) != SPELL_CAST_OK)
                    imp->GetMotionMaster()->MoveChase(degrader, 28.0f);
            }
        }

        void SpawnWaveHound(Creature* degrader, Position const& pos, float scale = 1.0f)
        {
            if (Creature* hound = me->SummonCreature(NPC_SHARTUUL_WAVE_FELHOUND, pos, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 120000))
                PrepareWaveDemon(hound, degrader, scale);
        }

        void SpawnWaveImp(Creature* degrader, Position const& pos)
        {
            if (Creature* imp = me->SummonCreature(NPC_SHARTUUL_WAVE_IMP, pos, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 120000))
                PrepareWaveImp(imp, degrader);
        }

        Creature* GetControlledDemon() const
        {
            return ObjectAccessor::GetCreature(*me, degraderGuid);
        }

        void BounceControlledDemonIntoArena(Creature* demon)
        {
            if (!demon)
                return;

            Position const bounce = GetClosestPointInsideShartuulBoundary(demon, ShartuulArenaBoundaryInset);
            demon->SetFaction(FactionShartuulControlled);
            demon->GetMotionMaster()->MoveJump(bounce.GetPositionX(), bounce.GetPositionY(), bounce.GetPositionZ(), 18.0f, 8.0f);
        }

        Creature* SpawnGanarg(Creature* controlled, Position const& pos)
        {
            SpawnGatePortalVisual(pos);
            Creature* ganarg = me->SummonCreature(NPC_SHARTUUL_GANARG_UNDERLING, pos, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 180000);
            if (ganarg)
                PrepareWaveDemon(ganarg, controlled);

            return ganarg;
        }

        Creature* SpawnMoarg(Creature* controlled, Position const& pos)
        {
            SpawnGatePortalVisual(pos);
            Creature* moarg = me->SummonCreature(NPC_SHARTUUL_MOARG_TORMENTER, pos, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 180000);
            if (moarg)
                PrepareWaveDemon(moarg, controlled, 0.5f);

            return moarg;
        }

        void SpawnDoomguardControlledWave(uint8 wave)
        {
            Creature* controlled = GetControlledDemon();
            if (!controlled || !controlled->IsAlive())
            {
                FailEvent();
                return;
            }

            activeWaveGuids.clear();
            waveTransitionPending = false;
            CleanupDoomguardWaveTimers();
            events.CancelEvent(EVENT_SHARTUUL_DOOMGUARD_PHASE_NEXT_WAVE);

            switch (wave)
            {
                case 1:
                    SpawnGanarg(controlled, ShartuulDoomguardWaveSpawn[0]);
                    SpawnGanarg(controlled, ShartuulDoomguardWaveSpawn[1]);
                    break;
                case 2:
                    SpawnGanarg(controlled, ShartuulDoomguardWaveSpawn[2]);
                    SpawnGanarg(controlled, ShartuulDoomguardWaveSpawn[3]);
                    SpawnGanarg(controlled, ShartuulDoomguardWaveSpawn[4]);
                    break;
                case 3:
                    SpawnGanarg(controlled, ShartuulDoomguardWaveSpawn[5]);
                    SpawnGanarg(controlled, ShartuulDoomguardWaveSpawn[6]);
                    SpawnGanarg(controlled, ShartuulDoomguardWaveSpawn[7]);
                    break;
                case 4:
                case 5:
                case 6:
                {
                    uint8 const moargIndex = wave - 4;
                    SpawnMoarg(controlled, ShartuulMoargWaveSpawn[moargIndex]);
                    for (uint8 i = 0; i < std::size(ShartuulDoomguardWaveSpawn) - 1; ++i)
                        SpawnGanarg(controlled, ShartuulDoomguardWaveSpawn[(i + moargIndex) % std::size(ShartuulDoomguardWaveSpawn)]);
                    break;
                }
                case 7:
                    SpawnGanarg(controlled, ShartuulDoomguardWaveSpawn[0]);
                    SpawnGanarg(controlled, ShartuulDoomguardWaveSpawn[4]);
                    doomguardControlledPhaseComplete = true;
                    if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                        player->GetSession()->SendAreaTriggerMessage("Two Gan'arg Underlings remain. The next demon is about to come out.");
                    break;
                default:
                    break;
            }

            if (wave < SHARTUUL_DOOMGUARD_PHASE_MAX_WAVE)
                doomguardControlledWave = wave + 1;

            events.ScheduleEvent(EVENT_SHARTUUL_GANARG_CANNON_CHECK, 6s);
            events.ScheduleEvent(EVENT_SHARTUUL_CANNON_FIRE, 2s);
            if (GetAliveWaveCreature(NPC_SHARTUUL_MOARG_TORMENTER))
            {
                events.ScheduleEvent(EVENT_SHARTUUL_GANARG_MOARG_SHIELD_CHECK, 5s);
            }
        }

        void ScheduleDoomguardControlledWaveIfCleared()
        {
            if (!doomguardControlledPhaseStarted || !IsCurrentWaveCleared() || waveTransitionPending)
                return;

            CleanupDoomguardWaveTimers();
            if (doomguardControlledPhaseComplete)
            {
                ReleaseNextCaptiveDemon();
                return;
            }

            waveTransitionPending = true;
            events.ScheduleEvent(EVENT_SHARTUUL_DOOMGUARD_PHASE_NEXT_WAVE, 5s);
        }

        void ReleaseNextCaptiveDemon()
        {
            if (shivanReleased)
                return;

            Creature* controlled = GetControlledDemon();
            Creature* shivan = ObjectAccessor::GetCreature(*me, nextCaptiveDemonGuid);
            if (!controlled || !controlled->IsAlive() || !shivan)
                return;

            shivanReleased = true;
            activeWaveGuids.clear();
            shivan->SetControlled(false, UNIT_STATE_ROOT);
            shivan->ClearUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING);
            shivan->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
            shivan->RemoveAurasDueToSpell(SPELL_SHARTUUL_VISUAL_SHELL_SHIELD);
            PrepareWaveDemon(shivan, controlled, 1.0f);

            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                player->GetSession()->SendAreaTriggerMessage("The Shivan Assassin is released!");
        }

        Creature* GetAliveWaveCreature(uint32 entry) const
        {
            for (ObjectGuid const& guid : activeWaveGuids)
            {
                Creature* creature = ObjectAccessor::GetCreature(*me, guid);
                if (creature && creature->IsAlive() && creature->GetEntry() == entry)
                    return creature;
            }

            return nullptr;
        }

        std::vector<Creature*> GetAliveWaveCreatures(uint32 entry) const
        {
            std::vector<Creature*> creatures;
            for (ObjectGuid const& guid : activeWaveGuids)
            {
                Creature* creature = ObjectAccessor::GetCreature(*me, guid);
                if (creature && creature->IsAlive() && creature->GetEntry() == entry)
                    creatures.push_back(creature);
            }

            return creatures;
        }

        void BeginGanargCannonSummon()
        {
            if (!doomguardControlledPhaseStarted || !pendingCannonSummonerGuid.IsEmpty())
                return;

            std::vector<Creature*> ganargs = GetAliveWaveCreatures(NPC_SHARTUUL_GANARG_UNDERLING);
            if (ganargs.empty())
                return;

            Creature* controlled = GetControlledDemon();
            Creature* ganarg = ganargs[urand(0, ganargs.size() - 1)];
            if (!controlled || !ganarg)
                return;

            pendingCannonSummonerGuid = ganarg->GetGUID();
            ganarg->AttackStop();
            ganarg->StopMoving();
            ganarg->SetReactState(REACT_PASSIVE);
            float const angle = controlled->GetAngle(ganarg);
            Position runPos = ganarg->GetPosition();
            runPos.m_positionX += std::cos(angle) * 8.0f;
            runPos.m_positionY += std::sin(angle) * 8.0f;
            ganarg->GetMotionMaster()->MovePoint(1, runPos);

            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                player->GetSession()->SendAreaTriggerMessage("A Gan'arg Underling begins summoning a Portable Fel Cannon!");

            events.ScheduleEvent(EVENT_SHARTUUL_GANARG_CANNON_FINISH, 7s);
        }

        void FinishGanargCannonSummon()
        {
            Creature* controlled = GetControlledDemon();
            Creature* ganarg = ObjectAccessor::GetCreature(*me, pendingCannonSummonerGuid);
            pendingCannonSummonerGuid.Clear();
            if (!controlled || !ganarg || !ganarg->IsAlive())
                return;

            if (ganarg->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING))
            {
                PrepareWaveDemon(ganarg, controlled);
                return;
            }

            Position pos = ganarg->GetPosition();
            pos.m_positionZ += 0.2f;
            SpawnGatePortalVisual(pos);
            if (Creature* cannon = me->SummonCreature(NPC_SHARTUUL_PORTABLE_FEL_CANNON, pos, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 120000))
                PrepareWaveDemon(cannon, controlled);

            PrepareWaveDemon(ganarg, controlled);
        }

        void FirePortableFelCannons()
        {
            Creature* controlled = GetControlledDemon();
            if (!controlled || !controlled->IsAlive())
                return;

            for (Creature* cannon : GetAliveWaveCreatures(NPC_SHARTUUL_PORTABLE_FEL_CANNON))
            {
                cannon->SetFacingToObject(controlled);
                if (cannon->CastSpell(controlled, SPELL_SHARTUUL_FEL_CANNON_BLAST, false) != SPELL_CAST_OK)
                {
                    cannon->CastSpell(controlled, SPELL_SHARTUUL_FEL_CANNON_TRACER, true);
                    cannon->CastSpell(controlled, SPELL_SHARTUUL_FEL_CANNON_BOLT, true);
                }

                Unit::DealDamage(cannon, controlled, 2800, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE);
            }
        }

        void CastMoargAcidGeyser()
        {
            Creature* controlled = GetControlledDemon();
            Creature* moarg = GetAliveWaveCreature(NPC_SHARTUUL_MOARG_TORMENTER);
            if (!controlled || !controlled->IsAlive() || !moarg)
                return;

            moarg->AI()->SetGUID(controlled->GetGUID(), DATA_SHARTUUL_MOARG_ACID_GEYSER_TARGET);
            moarg->AI()->DoAction(ACTION_SHARTUUL_MOARG_ACID_GEYSER);

            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                player->GetSession()->SendAreaTriggerMessage("Mo'arg Tormenter uses Acid Geyser!");
        }

        void BeginMoargShieldChannel()
        {
            if (!doomguardControlledPhaseStarted || !pendingMoargShieldCasterGuid.IsEmpty() || !pendingMoargShieldTargetGuid.IsEmpty())
                return;

            Creature* moarg = GetAliveWaveCreature(NPC_SHARTUUL_MOARG_TORMENTER);
            if (!moarg)
                return;

            std::vector<Creature*> ganargs = GetAliveWaveCreatures(NPC_SHARTUUL_GANARG_UNDERLING);
            if (ganargs.empty())
                return;

            Creature* ganarg = ganargs[urand(0, ganargs.size() - 1)];
            pendingMoargShieldCasterGuid = ganarg->GetGUID();
            pendingMoargShieldTargetGuid = moarg->GetGUID();
            ganarg->AttackStop();
            ganarg->StopMoving();
            ganarg->SetReactState(REACT_PASSIVE);

            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                player->GetSession()->SendAreaTriggerMessage("A Gan'arg Underling begins shielding the Mo'arg Tormenter!");

            events.ScheduleEvent(EVENT_SHARTUUL_GANARG_MOARG_SHIELD_FINISH, 6s);
        }

        void FinishMoargShieldChannel()
        {
            if (pendingMoargShieldCasterGuid.IsEmpty())
            {
                RemoveMoargShield();
                return;
            }

            Creature* controlled = GetControlledDemon();
            Creature* ganarg = ObjectAccessor::GetCreature(*me, pendingMoargShieldCasterGuid);
            Creature* moarg = ObjectAccessor::GetCreature(*me, pendingMoargShieldTargetGuid);
            pendingMoargShieldCasterGuid.Clear();
            pendingMoargShieldTargetGuid.Clear();
            if (!controlled || !ganarg || !ganarg->IsAlive() || !moarg || !moarg->IsAlive())
                return;

            if (!ganarg->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING))
            {
                moarg->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                events.ScheduleEvent(EVENT_SHARTUUL_GANARG_MOARG_SHIELD_FINISH, 8s);
                pendingMoargShieldTargetGuid = moarg->GetGUID();
                if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                    player->GetSession()->SendAreaTriggerMessage("The Mo'arg Tormenter is protected!");
            }

            PrepareWaveDemon(ganarg, controlled);
        }

        void RemoveMoargShield()
        {
            Creature* moarg = ObjectAccessor::GetCreature(*me, pendingMoargShieldTargetGuid);
            pendingMoargShieldTargetGuid.Clear();
            if (moarg)
                moarg->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        }

        void SpawnPhaseOneWave(uint8 wave)
        {
            Creature* degrader = ObjectAccessor::GetCreature(*me, degraderGuid);
            if (!degrader)
            {
                FailEvent();
                return;
            }

            activeWaveGuids.clear();
            waveTransitionPending = false;
            events.CancelEvent(EVENT_SHARTUUL_NEXT_WAVE);
            SpawnGateVisuals();
            switch (wave)
            {
                case 1:
                    SpawnGatePortalVisual(ShartuulWaveOneSpawn[0]);
                    SpawnGatePortalVisual(ShartuulWaveOneSpawn[1]);
                    SpawnWaveHound(degrader, ShartuulWaveOneSpawn[0]);
                    SpawnWaveHound(degrader, ShartuulWaveOneSpawn[1], 0.55f);
                    SpawnWaveImp(degrader, ShartuulWaveOneSpawn[1]);
                    break;
                case 2:
                    SpawnGatePortalVisual(ShartuulWaveOneSpawn[0]);
                    SpawnGatePortalVisual(ShartuulWaveOneSpawn[1]);
                    SpawnWaveHound(degrader, ShartuulWaveOneSpawn[0]);
                    SpawnWaveHound(degrader, ShartuulWaveOneSpawn[1]);
                    SpawnWaveHound(degrader, ShartuulWaveSpawn[2]);
                    SpawnWaveImp(degrader, ShartuulWaveOneSpawn[1]);
                    break;
                case 3:
                    SpawnGatePortalVisual(ShartuulWaveOneSpawn[0]);
                    SpawnGatePortalVisual(ShartuulWaveOneSpawn[1]);
                    SpawnWaveHound(degrader, ShartuulWaveOneSpawn[0]);
                    SpawnWaveHound(degrader, ShartuulWaveOneSpawn[1]);
                    SpawnWaveImp(degrader, ShartuulWaveOneSpawn[1]);
                    break;
                case 4:
                case 5:
                case 6:
                    for (Position const& pos : ShartuulWaveSpawn)
                        SpawnGatePortalVisual(pos);

                    for (uint8 i = 0; i < 8; ++i)
                        SpawnWaveHound(degrader, ShartuulWaveSpawn[i % std::size(ShartuulWaveSpawn)]);

                    SpawnWaveImp(degrader, ShartuulWaveSpawn[1]);
                    SpawnWaveImp(degrader, ShartuulWaveSpawn[3]);
                    break;
                default:
                    break;
            }

            if (eventActive && !phaseTwoStarted && shieldRemainingPct > 0)
            {
                currentWave = currentWave < SHARTUUL_PHASE_ONE_MAX_WAVE ? currentWave + 1 : SHARTUUL_PHASE_ONE_MAX_WAVE;
                events.ScheduleEvent(EVENT_SHARTUUL_NEXT_WAVE, 20s);
            }

            events.CancelEvent(EVENT_SHARTUUL_IMP_FIREBOLT);
            events.ScheduleEvent(EVENT_SHARTUUL_IMP_FIREBOLT, 2s);
        }

        void DespawnMinorDemons()
        {
            events.CancelEvent(EVENT_SHARTUUL_NEXT_WAVE);
            events.CancelEvent(EVENT_SHARTUUL_WAVE_ONE);
            events.CancelEvent(EVENT_SHARTUUL_IMP_FIREBOLT);
            waveTransitionPending = false;
            activeWaveGuids.clear();

            SummonList::StorageType despawnGuids;
            for (ObjectGuid const& guid : summons)
            {
                Creature* summon = ObjectAccessor::GetCreature(*me, guid);
                if (!summon)
                    continue;

                switch (summon->GetEntry())
                {
                    case NPC_SHARTUUL_WAVE_FELHOUND:
                    case NPC_SHARTUUL_WAVE_IMP:
                    case NPC_FELHOUND_DEFENDER:
                    case NPC_FEL_IMP_DEFENDER:
                        despawnGuids.push_back(guid);
                        break;
                    default:
                        break;
                }
            }

            for (ObjectGuid const& guid : despawnGuids)
                if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                    summon->DespawnOrUnsummon();
        }

        void PrepareReleasedDoomguard(Creature* doomguard, Creature* degrader)
        {
            if (!doomguard || !degrader)
                return;

            doomguard->SetPhaseMask(PHASEMASK_NORMAL, true);
            doomguard->SetFaction(FactionShartuulWaveDemon);
            doomguard->SetObjectScale(1.0f);
            doomguard->SetDisplayId(doomguard->GetNativeDisplayId());
            doomguard->SetReactState(REACT_AGGRESSIVE);
            doomguard->RemoveAurasDueToSpell(SPELL_SHARTUUL_VISUAL_SHELL_SHIELD);
            doomguard->SetControlled(false, UNIT_STATE_ROOT);
            doomguard->ClearUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING);
            doomguard->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
            doomguard->SetFullHealth();
            doomguard->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, 125.0f);
            doomguard->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, 250.0f);
            doomguard->UpdateDamagePhysical(BASE_ATTACK);
            doomguard->CombatStop(true);
            doomguard->SetInCombatWith(degrader);
            degrader->SetInCombatWith(doomguard);
            doomguard->AddThreat(degrader, 100000.0f);
            doomguard->AI()->AttackStart(degrader);
            doomguard->GetMotionMaster()->MoveChase(degrader);
        }

        void StartPhaseTwo()
        {
            if (phaseTwoStarted)
                return;

            Creature* degrader = ObjectAccessor::GetCreature(*me, degraderGuid);
            Creature* doomguard = GetOrCreateCaptiveDemon();
            if (!degrader || !doomguard)
            {
                FailEvent();
                return;
            }

            phaseTwoStarted = true;
            phaseTwoRewarded = false;
            phaseOneStarted = false;
            currentWave = 0;
            doomguardFelFlamesTicks = 0;
            DespawnMinorDemons();
            captiveDemonGuid = doomguard->GetGUID();
            PrepareReleasedDoomguard(doomguard, degrader);

            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                player->GetSession()->SendAreaTriggerMessage("The Doomguard Punisher is released!");

            events.ScheduleEvent(EVENT_SHARTUUL_DOOMGUARD_PUNISHING_BLOW, 5s);
            events.ScheduleEvent(EVENT_SHARTUUL_DOOMGUARD_FEL_FLAMES, 12s);
        }

        void CompletePhaseTwo()
        {
            if (!phaseTwoStarted || phaseTwoRewarded)
                return;

            Creature* doomguard = ObjectAccessor::GetCreature(*me, captiveDemonGuid);
            Creature* oldDegrader = ObjectAccessor::GetCreature(*me, degraderGuid);
            Player* player = ObjectAccessor::GetPlayer(*me, playerGuid);
            if (!doomguard || !player)
            {
                FailEvent();
                return;
            }

            phaseTwoRewarded = true;
            phaseTwoStarted = false;
            doomguardFelFlamesTicks = 0;
            events.CancelEvent(EVENT_SHARTUUL_DOOMGUARD_FEL_FLAMES);
            events.CancelEvent(EVENT_SHARTUUL_DOOMGUARD_PUNISHING_BLOW);
            events.CancelEvent(EVENT_SHARTUUL_DOOMGUARD_FEL_FLAMES_TICK);

            if (player)
            {
                if (FactionEntry const* faction = sFactionStore.LookupEntry(FACTION_OGRILA))
                    player->GetReputationMgr().ModifyReputation(faction, 100.0f);

                player->GetSession()->SendAreaTriggerMessage("Doomguard Punisher defeated. Ogri'la reputation +100.");
            }

            TransferControlToDoomguard(player, oldDegrader, doomguard);
        }

        void PrepareControlledDoomguard(Creature* doomguard)
        {
            if (!doomguard)
                return;

            Position revivePos = doomguard->GetPosition();
            if (!doomguard->IsAlive())
                doomguard->Respawn(true);

            if (!doomguard->IsAlive())
                doomguard->setDeathState(DeathState::Alive);

            doomguard->NearTeleportTo(revivePos.GetPositionX(), revivePos.GetPositionY(), revivePos.GetPositionZ(), revivePos.GetOrientation());
            doomguard->CombatStop(true);
            doomguard->SetFullHealth();
            doomguard->SetPhaseMask(PHASEMASK_NORMAL, true);
            doomguard->SetFaction(FactionShartuulControlled);
            doomguard->SetReactState(REACT_PASSIVE);
            doomguard->SetWalk(false);
            doomguard->SetControlled(false, UNIT_STATE_ROOT);
            doomguard->ClearUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING);
            doomguard->RemoveAurasDueToSpell(SPELL_SHARTUUL_VISUAL_SHELL_SHIELD);
            doomguard->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
            doomguard->GetMotionMaster()->Clear();
        }

        void TransferControlToDoomguard(Player* player, Creature* oldDegrader, Creature* doomguard)
        {
            if (!player || !doomguard)
                return;

            PrepareControlledDoomguard(doomguard);

            transferringDemonControl = true;
            player->RemoveCharmAuras();
            if (oldDegrader)
            {
                oldDegrader->RemoveCharmAuras();
                oldDegrader->CombatStop(true);
                oldDegrader->SetControlled(false, UNIT_STATE_ROOT);
                oldDegrader->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
                Unit::DealDamage(doomguard, oldDegrader, oldDegrader->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
                oldDegrader->DespawnOrUnsummon(5s);
            }

            degraderGuid = doomguard->GetGUID();
            captiveDemonGuid = doomguard->GetGUID();
            player->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            player->CastSpell(doomguard, SPELL_POSSESS, true);
            doomguard->SetFaction(FactionShartuulControlled);
            transferringDemonControl = false;
            StartDoomguardControlledPhase();
        }

        void CompleteShivanPhase()
        {
            if (!eventActive || !shivanReleased || transferringDemonControl)
                return;

            Creature* shivan = ObjectAccessor::GetCreature(*me, nextCaptiveDemonGuid);
            Creature* oldDoomguard = ObjectAccessor::GetCreature(*me, degraderGuid);
            Player* player = ObjectAccessor::GetPlayer(*me, playerGuid);
            if (!player || !shivan)
            {
                FailEvent();
                return;
            }

            PrepareControlledDoomguard(shivan);
            transferringDemonControl = true;
            player->RemoveCharmAuras();

            if (oldDoomguard && oldDoomguard != shivan)
            {
                oldDoomguard->RemoveCharmAuras();
                oldDoomguard->CombatStop(true);
                oldDoomguard->SetControlled(false, UNIT_STATE_ROOT);
                oldDoomguard->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
                Unit::DealDamage(shivan, oldDoomguard, oldDoomguard->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
                oldDoomguard->DespawnOrUnsummon(5s);
            }

            degraderGuid = shivan->GetGUID();
            captiveDemonGuid = shivan->GetGUID();
            nextCaptiveDemonGuid.Clear();
            activeWaveGuids.clear();
            doomguardControlledPhaseStarted = false;
            doomguardControlledPhaseComplete = false;
            shivanReleased = false;
            player->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            player->CastSpell(shivan, SPELL_POSSESS, true);
            shivan->SetFaction(FactionShartuulControlled);
            transferringDemonControl = false;

            player->GetSession()->SendAreaTriggerMessage("Shivan Assassin defeated. You now control it.");
            events.ScheduleEvent(EVENT_SHARTUUL_PHASE_THREE_START, 3s);
        }

        void StartDoomguardControlledPhase()
        {
            doomguardControlledPhaseStarted = true;
            doomguardControlledPhaseComplete = false;
            waveTransitionPending = false;
            pendingCannonSummonerGuid.Clear();
            pendingMoargShieldCasterGuid.Clear();
            pendingMoargShieldTargetGuid.Clear();
            activeWaveGuids.clear();
            doomguardControlledWave = 1;

            GetOrCreateNextCaptiveDemon();

            events.CancelEvent(EVENT_SHARTUUL_DOOMGUARD_PHASE_NEXT_WAVE);
            events.ScheduleEvent(EVENT_SHARTUUL_DOOMGUARD_PHASE_NEXT_WAVE, 3s);
        }

        void CastDoomguardSpell(uint32 spellId)
        {
            if (!phaseTwoStarted)
                return;

            Creature* doomguard = ObjectAccessor::GetCreature(*me, captiveDemonGuid);
            Creature* degrader = ObjectAccessor::GetCreature(*me, degraderGuid);
            if (!doomguard || !doomguard->IsAlive() || !degrader || !degrader->IsAlive())
                return;

            doomguard->SetFaction(FactionShartuulWaveDemon);
            doomguard->SetInCombatWith(degrader);
            doomguard->AddThreat(degrader, 100000.0f);
            if (!doomguard->GetVictim())
                doomguard->AI()->AttackStart(degrader);

            doomguard->CastSpell(degrader, spellId, false);
        }

        void StartDoomguardFelFlames()
        {
            CastDoomguardSpell(SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES);
            doomguardFelFlamesTicks = SHARTUUL_DOOMGUARD_FEL_FLAMES_TICKS;
            events.CancelEvent(EVENT_SHARTUUL_DOOMGUARD_FEL_FLAMES_TICK);
            events.ScheduleEvent(EVENT_SHARTUUL_DOOMGUARD_FEL_FLAMES_TICK, 1s);
        }

        void TickDoomguardFelFlames()
        {
            if (!phaseTwoStarted || doomguardFelFlamesTicks == 0)
                return;

            Creature* doomguard = ObjectAccessor::GetCreature(*me, captiveDemonGuid);
            Creature* degrader = ObjectAccessor::GetCreature(*me, degraderGuid);
            if (!doomguard || !doomguard->IsAlive() || !degrader || !degrader->IsAlive())
                return;

            Unit::DealDamage(doomguard, degrader, 1000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE);
            --doomguardFelFlamesTicks;

            if (doomguardFelFlamesTicks > 0)
                events.ScheduleEvent(EVENT_SHARTUUL_DOOMGUARD_FEL_FLAMES_TICK, 1s);
        }

        void CheckPhaseTwoBossState()
        {
            if (!phaseTwoStarted || phaseTwoRewarded)
                return;

            Creature* doomguard = ObjectAccessor::GetCreature(*me, captiveDemonGuid);
            if (doomguard && !doomguard->IsAlive())
                CompletePhaseTwo();
        }

        void PreparePhaseThreeBoss(Creature* boss, Creature* shivan, uint32 health, float scale = 1.0f, bool mainBoss = true)
        {
            if (!boss || !shivan)
                return;

            TrackWaveDemon(boss);
            boss->SetPhaseMask(PHASEMASK_NORMAL, true);
            boss->SetFaction(FactionShartuulWaveDemon);
            boss->SetObjectScale(scale);
            boss->SetReactState(REACT_AGGRESSIVE);
            boss->SetRegeneratingHealth(false);
            ApplySniffedPhaseThreeCreatureState(boss);
            boss->SetObjectScale(scale);
            boss->SetMaxHealth(health);
            boss->SetHealth(health);
            boss->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
            bool const activateNow = boss->GetEntry() != NPC_EREDAR_SHARTUUL || phaseThreeBossStage != 3;
            if (activateNow)
            {
                boss->SetInCombatWith(shivan);
                shivan->SetInCombatWith(boss);
                boss->AI()->SetGUID(shivan->GetGUID(), DATA_SHARTUUL_DEGRADER);
            }

            if (boss->GetEntry() == NPC_EYE_OF_SHARTUUL || boss->GetEntry() == NPC_EYE_OF_SHARTUUL_TRANSFORM)
            {
                boss->SetReactState(REACT_PASSIVE);
                boss->AttackStop();
                boss->GetMotionMaster()->Clear();
                boss->GetMotionMaster()->MoveIdle();
            }
            else if (boss->GetEntry() == NPC_EREDAR_SHARTUUL)
            {
                boss->SetReactState(REACT_PASSIVE);
                boss->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                boss->AttackStop();
                boss->GetMotionMaster()->Clear();
                if (phaseThreeBossStage == 3)
                    boss->GetMotionMaster()->MovePoint(0, ShartuulPhaseThreeBossSpawn.GetPositionX(), ShartuulPhaseThreeBossSpawn.GetPositionY(), ShartuulPhaseThreeBossSpawn.GetPositionZ());
                else
                    boss->GetMotionMaster()->MoveIdle();
            }
            else
            {
                boss->SetReactState(REACT_AGGRESSIVE);
                boss->AddThreat(shivan, 100000.0f);
                boss->AI()->AttackStart(shivan);
                boss->GetMotionMaster()->MoveChase(shivan);
            }

            if (mainBoss)
                phaseThreeBossGuid = boss->GetGUID();
        }

        void StartShivanControlledPhase()
        {
            Creature* shivan = GetControlledDemon();
            if (!eventActive || !shivan || !shivan->IsAlive())
                return;

            shivanControlledPhaseStarted = true;
            phaseThreeBossStage = 1;
            phaseThreeBossGuid.Clear();
            activeWaveGuids.clear();
            events.CancelEvent(EVENT_SHARTUUL_PHASE_THREE_START);
            events.CancelEvent(EVENT_SHARTUUL_PHASE_THREE_TENTACLES);
            events.CancelEvent(EVENT_SHARTUUL_PHASE_THREE_SHARTUUL_ACTIVATE);

            EnsurePhaseThreePortalCaster();
            SpawnPhaseThreeBoss();
        }

        void StartEyePhaseIntroTest()
        {
            Player* player = ObjectAccessor::GetPlayer(*me, playerGuid);
            Creature* shivan = ObjectAccessor::GetCreature(*me, degraderGuid);
            if (!player || !shivan || shivan->GetEntry() != NPC_SHARTUUL_SHIVAN_ASSASSIN)
                return;

            DespawnEventSummonsExcept(shivan->GetGUID());
            DespawnEventGameObjects();
            activeWaveGuids.clear();
            phaseThreeBossGuid.Clear();
            phaseThreeSummonerGuid.Clear();
            nextCaptiveDemonGuid.Clear();

            eventActive = true;
            phaseOneStarted = true;
            phaseTwoStarted = true;
            phaseTwoRewarded = true;
            doomguardControlledPhaseStarted = true;
            doomguardControlledPhaseComplete = true;
            shivanControlledPhaseStarted = true;
            shivanReleased = true;
            transferringDemonControl = true;
            phaseThreeBossStage = 1;

            player->RemoveCharmAuras();
            PrepareShartuulTestPossessDemon(shivan);
            shivan->SetFaction(FactionShartuulControlled);
            player->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            player->CastSpell(shivan, SPELL_POSSESS, true);
            transferringDemonControl = false;

            events.CancelEvent(EVENT_SHARTUUL_PHASE_THREE_START);
            events.CancelEvent(EVENT_SHARTUUL_PHASE_THREE_SHARTUUL_ACTIVATE);
            events.CancelEvent(EVENT_SHARTUUL_PHASE_THREE_TENTACLES);
            events.CancelEvent(EVENT_SHARTUUL_BOUNDARY_CHECK);
            events.CancelEvent(EVENT_SHARTUUL_TEST_EYE_INTRO_PORTAL);
            events.CancelEvent(EVENT_SHARTUUL_TEST_EYE_INTRO_SHARTUUL);
            events.CancelEvent(EVENT_SHARTUUL_TEST_EYE_INTRO_YELL);
            events.CancelEvent(EVENT_SHARTUUL_TEST_EYE_INTRO_LAUGH);
            events.CancelEvent(EVENT_SHARTUUL_TEST_EYE_INTRO_EYE);

            player->GetSession()->SendAreaTriggerMessage("Starting Shartuul Eye phase intro test.");
            events.ScheduleEvent(EVENT_SHARTUUL_TEST_EYE_INTRO_PORTAL, 250ms);
        }

        void TestEyeIntroOpenPortal()
        {
            SpawnGatePortalVisual(ShartuulPhaseThreePortalSpawn);
            events.ScheduleEvent(EVENT_SHARTUUL_TEST_EYE_INTRO_SHARTUUL, 1500ms);
        }

        void TestEyeIntroSpawnShartuul()
        {
            Creature* shartuul = EnsurePhaseThreePortalCaster();
            if (!shartuul)
                return;

            shartuul->NearTeleportTo(ShartuulPhaseThreeSummonerSpawn.GetPositionX(), ShartuulPhaseThreeSummonerSpawn.GetPositionY(), ShartuulPhaseThreeSummonerSpawn.GetPositionZ(), ShartuulPhaseThreeSummonerSpawn.GetOrientation());
            shartuul->SetReactState(REACT_PASSIVE);
            shartuul->SetFaction(FactionShartuulWaveDemon);
            shartuul->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
            shartuul->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
            shartuul->AttackStop();
            shartuul->CombatStop(true);
            shartuul->GetMotionMaster()->Clear();
            shartuul->GetMotionMaster()->MoveIdle();
            shartuul->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST);

            events.ScheduleEvent(EVENT_SHARTUUL_TEST_EYE_INTRO_YELL, 1500ms);
        }

        void TestEyeIntroYell()
        {
            if (Creature* shartuul = ObjectAccessor::GetCreature(*me, phaseThreeSummonerGuid))
                shartuul->Yell("Now you shall witness the meaning of true power!", LANG_UNIVERSAL, GetControlledDemon());

            events.ScheduleEvent(EVENT_SHARTUUL_TEST_EYE_INTRO_LAUGH, 2500ms);
        }

        void TestEyeIntroLaugh()
        {
            if (Creature* shartuul = ObjectAccessor::GetCreature(*me, phaseThreeSummonerGuid))
                shartuul->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);

            events.ScheduleEvent(EVENT_SHARTUUL_TEST_EYE_INTRO_EYE, 2000ms);
        }

        void TestEyeIntroSpawnEye()
        {
            SpawnPhaseThreeBoss();
        }

        Creature* EnsurePhaseThreePortalCaster()
        {
            if (Creature* caster = ObjectAccessor::GetCreature(*me, phaseThreeSummonerGuid))
                if (caster->IsAlive())
                    return caster;

            if (Creature* caster = me->SummonCreature(NPC_EREDAR_SHARTUUL, ShartuulPhaseThreeSummonerSpawn, TEMPSUMMON_MANUAL_DESPAWN))
            {
                phaseThreeSummonerGuid = caster->GetGUID();
                ApplySniffedPhaseThreeCreatureState(caster);
                caster->AI()->SetData(DATA_SHARTUUL_PHASE_THREE_SUMMONER, 1);
                caster->SetReactState(REACT_PASSIVE);
                caster->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                caster->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
                caster->GetMotionMaster()->Clear();
                caster->GetMotionMaster()->MoveIdle();
                caster->SetFacingTo(ShartuulPhaseThreeSummonerSpawn.GetOrientation());
                caster->CastSpell(caster, SPELL_SHARTUUL_SELF_VISUAL, true);
                return caster;
            }

            return nullptr;
        }

        void SpawnPhaseThreeBoss()
        {
            Creature* shivan = GetControlledDemon();
            if (!shivan || !shivan->IsAlive())
            {
                FailEvent();
                return;
            }

            uint32 entry = 0;
            uint32 health = 120000;
            float scale = 1.0f;
            char const* name = nullptr;
            Creature* portalCaster = EnsurePhaseThreePortalCaster();
            switch (phaseThreeBossStage)
            {
                case 1:
                    entry = NPC_EYE_OF_SHARTUUL;
                    health = 120000;
                    name = "Eye of Shartuul";
                    break;
                case 2:
                    entry = NPC_DREADMAW;
                    health = 130000;
                    scale = 1.0f;
                    name = "Dreadmaw";
                    break;
                case 3:
                    entry = NPC_EREDAR_SHARTUUL;
                    health = 160000;
                    name = "Shartuul";
                    break;
                default:
                    CompleteEvent();
                    return;
            }

            Position const& spawn = entry == NPC_EREDAR_SHARTUUL ? ShartuulPhaseThreeSummonerSpawn : (entry == NPC_EYE_OF_SHARTUUL ? ShartuulPhaseThreeEyeSpawn : ShartuulPhaseThreeBossSpawn);
            if (portalCaster)
            {
                if (entry == NPC_EREDAR_SHARTUUL)
                    portalCaster->InterruptNonMeleeSpells(false);
                else
                {
                    portalCaster->SetFacingTo(portalCaster->GetAngle(spawn.GetPositionX(), spawn.GetPositionY()));
                    portalCaster->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST);
                }
            }
            else
            {
                Creature* source = me->SummonCreature(NPC_WORLD_EVENT_GENERATOR, ShartuulPhaseThreePortalSpawn, TEMPSUMMON_TIMED_DESPAWN, 2500);
                Creature* target = me->SummonCreature(NPC_WORLD_EVENT_GENERATOR, spawn, TEMPSUMMON_TIMED_DESPAWN, 2500);
                if (source && target)
                {
                    PrepareVisualAnchor(source);
                    PrepareVisualAnchor(target);
                    source->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, target->GetGUID());
                    source->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_GREEN_LIGHTNING);
                }
            }

            Creature* boss = nullptr;
            if (entry == NPC_EREDAR_SHARTUUL)
            {
                boss = portalCaster ? portalCaster : me->SummonCreature(entry, spawn, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000);
                phaseThreeSummonerGuid.Clear();
                if (boss)
                {
                    boss->InterruptNonMeleeSpells(false);
                    boss->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                    boss->NearTeleportTo(spawn.GetPositionX(), spawn.GetPositionY(), spawn.GetPositionZ(), spawn.GetOrientation());
                }
            }
            else
                boss = me->SummonCreature(entry, spawn, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000);

            if (boss)
            {
                PreparePhaseThreeBoss(boss, shivan, health, scale);
                if (entry == NPC_DREADMAW && portalCaster)
                {
                    portalCaster->SetFacingToObject(boss);
                    portalCaster->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, boss->GetGUID());
                    portalCaster->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_DREADMAW_SUMMON_CHANNEL);
                    portalCaster->CastSpell(boss, SPELL_SHARTUUL_DREADMAW_SUMMON_CHANNEL, true);
                    boss->CastSpell(boss, SPELL_SHARTUUL_DREADMAW_INTRO, true);
                    boss->CastSpell(boss, SPELL_SHARTUUL_DREADMAW_AURA, true);
                }

                if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                    player->GetSession()->SendAreaTriggerMessage("%s enters the ring!", name);

                if (entry == NPC_EREDAR_SHARTUUL)
                {
                    SpawnShartuulSummonVisual(boss, ShartuulPhaseThreeBossSpawn);
                    events.ScheduleEvent(EVENT_SHARTUUL_PHASE_THREE_SHARTUUL_ACTIVATE, 4s);
                }
            }
        }

        void ActivateFinalShartuul()
        {
            if (!shivanControlledPhaseStarted || phaseThreeBossStage != 3)
                return;

            Creature* boss = GetPhaseThreeBoss();
            Creature* shivan = GetControlledDemon();
            if (!boss || !shivan || !shivan->IsAlive())
                return;

            boss->InterruptNonMeleeSpells(false);
            boss->AI()->SetData(DATA_SHARTUUL_PHASE_THREE_SUMMONER, 0);
            boss->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
            boss->SetReactState(REACT_AGGRESSIVE);
            boss->SetInCombatWith(shivan);
            shivan->SetInCombatWith(boss);
            boss->AddThreat(shivan, 100000.0f);
            boss->AI()->SetGUID(shivan->GetGUID(), DATA_SHARTUUL_DEGRADER);
            boss->AI()->AttackStart(shivan);
            events.ScheduleEvent(EVENT_SHARTUUL_PHASE_THREE_TENTACLES, 8s);
        }

        void CompletePhaseThreeBoss(uint32 entry)
        {
            if (!shivanControlledPhaseStarted)
                return;

            uint8 expectedStage = 0;
            uint32 reputation = 0;
            if (entry == NPC_EYE_OF_SHARTUUL_TRANSFORM || entry == NPC_EYE_OF_SHARTUUL)
            {
                expectedStage = 1;
                reputation = 500;
            }
            else if (entry == NPC_DREADMAW)
            {
                expectedStage = 2;
                reputation = 500;
            }
            else if (entry == NPC_EREDAR_SHARTUUL)
            {
                expectedStage = 3;
                reputation = 750;
            }
            else
                return;

            if (phaseThreeBossStage != expectedStage)
                return;

            if (entry == NPC_EREDAR_SHARTUUL)
            {
                events.CancelEvent(EVENT_SHARTUUL_PHASE_THREE_TENTACLES);
                events.CancelEvent(EVENT_SHARTUUL_PHASE_THREE_SHARTUUL_ACTIVATE);
            }

            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                if (FactionEntry const* faction = sFactionStore.LookupEntry(FACTION_OGRILA))
                    player->GetReputationMgr().ModifyReputation(faction, float(reputation));

            ++phaseThreeBossStage;
            phaseThreeBossGuid.Clear();
            activeWaveGuids.clear();

            if (entry == NPC_EREDAR_SHARTUUL)
            {
                CompleteEvent();
                return;
            }

            events.ScheduleEvent(EVENT_SHARTUUL_PHASE_THREE_START, 5s);
        }

        Creature* GetPhaseThreeBoss() const
        {
            Creature* boss = ObjectAccessor::GetCreature(*me, phaseThreeBossGuid);
            return boss && boss->IsAlive() ? boss : nullptr;
        }

        void CastPhaseThreeBossAbility()
        {
            if (!shivanControlledPhaseStarted)
                return;

            Creature* boss = GetPhaseThreeBoss();
            Creature* shivan = GetControlledDemon();
            if (!boss || !shivan || !shivan->IsAlive())
                return;

            boss->SetFaction(FactionShartuulWaveDemon);
            boss->SetInCombatWith(shivan);
            boss->AddThreat(shivan, 100000.0f);
            if (!boss->GetVictim())
                boss->AI()->AttackStart(shivan);

            switch (boss->GetEntry())
            {
                case NPC_EYE_OF_SHARTUUL_TRANSFORM:
                case NPC_EYE_OF_SHARTUUL:
                    if (urand(0, 2) == 0)
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                            player->GetSession()->SendAreaTriggerMessage("The Eye of Shartuul focuses intently!");
                        SendShartuulCastBar(boss, shivan, SPELL_SHARTUUL_EYE_DARK_GLARE, 7000);
                        boss->SetFacingToObject(shivan);
                        boss->CastSpell(shivan, SPELL_SHARTUUL_EYE_DARK_GLARE, true);
                        events.ScheduleEvent(EVENT_SHARTUUL_PHASE_THREE_DARK_GLARE_HIT, 7s);
                    }
                    else
                    {
                        boss->CastSpell(shivan, SPELL_SHARTUUL_GENERIC_FIREBALL, false);
                        Unit::DealDamage(boss, shivan, 8500, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE);
                    }
                    events.ScheduleEvent(EVENT_SHARTUUL_PHASE_THREE_BOSS_ABILITY, 9s);
                    break;
                case NPC_DREADMAW:
                    boss->CastSpell(boss, SPELL_SHARTUUL_DREADMAW_GROWTH, true);
                    boss->SetObjectScale(std::min(2.0f, boss->GetObjectScale() + 0.10f));
                    Unit::DealDamage(boss, shivan, 9000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL);
                    events.ScheduleEvent(EVENT_SHARTUUL_PHASE_THREE_BOSS_ABILITY, 12s);
                    break;
                case NPC_EREDAR_SHARTUUL:
                    if (urand(0, 3) == 0)
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                            player->GetSession()->SendAreaTriggerMessage("Shartuul begins casting Incinerate!");
                        SendShartuulCastBar(boss, shivan, SPELL_SHARTUUL_EREDAR_INCINERATE, 7000);
                        boss->SetFacingToObject(shivan);
                        boss->CastSpell(shivan, SPELL_SHARTUUL_EREDAR_INCINERATE, true);
                        events.ScheduleEvent(EVENT_SHARTUUL_PHASE_THREE_INCINERATE_HIT, 7s);
                    }
                    else
                    {
                        boss->CastSpell(shivan, SPELL_SHARTUUL_GENERIC_SHADOW_BOLT, false);
                        Unit::DealDamage(boss, shivan, 4500, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_SHADOW);
                    }
                    events.ScheduleEvent(EVENT_SHARTUUL_PHASE_THREE_BOSS_ABILITY, 8s);
                    break;
                default:
                    break;
            }
        }

        void ResolveEyeDarkGlare()
        {
            Creature* boss = GetPhaseThreeBoss();
            Creature* shivan = GetControlledDemon();
            if (!boss || !shivan || (boss->GetEntry() != NPC_EYE_OF_SHARTUUL_TRANSFORM && boss->GetEntry() != NPC_EYE_OF_SHARTUUL))
                return;

            Unit::DealDamage(boss, shivan, 100000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_SHADOW);
        }

        void ResolveShartuulIncinerate()
        {
            Creature* boss = GetPhaseThreeBoss();
            Creature* shivan = GetControlledDemon();
            if (!boss || !shivan || boss->GetEntry() != NPC_EREDAR_SHARTUUL)
                return;

            Unit::DealDamage(boss, shivan, std::max<uint32>(15000, shivan->GetMaxHealth() * 40 / 100), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE);
        }

        void SpawnPhaseThreeTentacles()
        {
            if (!shivanControlledPhaseStarted || phaseThreeBossStage != 3)
                return;

            Creature* shivan = GetControlledDemon();
            if (!shivan || !shivan->IsAlive())
                return;

            for (Position const& pos : ShartuulTentacleSpawn)
            {
                if (Creature* tentacle = me->SummonCreature(NPC_EYE_TENTACLE, pos, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 45000))
                {
                    PreparePhaseThreeBoss(tentacle, shivan, 12000, 0.8f, false);
                    tentacle->GetMotionMaster()->Clear();
                    tentacle->GetMotionMaster()->MoveIdle();
                    tentacle->SetControlled(true, UNIT_STATE_ROOT);
                    tentacle->CastSpell(shivan, SPELL_SHARTUUL_GENERIC_MIND_FLAY, false);
                }
            }

            events.ScheduleEvent(EVENT_SHARTUUL_PHASE_THREE_TENTACLES, 25s);
        }

        void CompleteEvent()
        {
            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
            {
                player->RemoveCharmAuras();
                player->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                player->SendUpdateWorldState(WORLD_STATE_BLACK_MORASS, 0);
                player->SendUpdateWorldState(WORLD_STATE_BLACK_MORASS_SHIELD, 0);
                player->GetSession()->SendAreaTriggerMessage("Shartuul is defeated.");
            }

            eventActive = false;
            shivanControlledPhaseStarted = false;
            phaseThreeBossStage = 0;
            phaseThreeBossGuid.Clear();
            phaseThreeSummonerGuid.Clear();
            events.Reset();
            summons.DespawnAll();
            DespawnEventGameObjects();
            activeWaveGuids.clear();
            forceFreshDegrader = true;
            events.ScheduleEvent(EVENT_SHARTUUL_RESPAWN_IDLE, 10s);
        }

        void FailEvent()
        {
            eventActive = false;
            phaseOneStarted = false;
            phaseTwoStarted = false;
            phaseTwoRewarded = false;
            waveTransitionPending = false;
            shivanReleased = false;
            shivanControlledPhaseStarted = false;
            phaseThreeBossGuid.Clear();
            phaseThreeSummonerGuid.Clear();
            phaseThreeBossStage = 0;
            currentWave = 0;
            doomguardFelFlamesTicks = 0;
            activeWaveGuids.clear();
            shieldRemainingPct = 100;

            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
            {
                player->RemoveCharmAuras();
                player->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                player->SendUpdateWorldState(WORLD_STATE_BLACK_MORASS, 0);
                player->SendUpdateWorldState(WORLD_STATE_BLACK_MORASS_SHIELD, 0);
            }

            if (Creature* degrader = ObjectAccessor::GetCreature(*me, degraderGuid))
                degrader->DespawnOrUnsummon();

            events.Reset();
            summons.DespawnAll();
            DespawnEventGameObjects();
            activeWaveGuids.clear();
            playerGuid.Clear();
            degraderGuid.Clear();
            captiveDemonGuid.Clear();
            nextCaptiveDemonGuid.Clear();
            for (ObjectGuid& guid : shieldHelperGuids)
                guid.Clear();
            forceFreshDegrader = true;
            events.ScheduleEvent(EVENT_SHARTUUL_RESPAWN_IDLE, 2s);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SHARTUUL_WAVE_ONE:
                        SpawnPhaseOneWave(1);
                        break;
                    case EVENT_SHARTUUL_NEXT_WAVE:
                        SpawnPhaseOneWave(currentWave);
                        break;
                    case EVENT_SHARTUUL_BOUNDARY_CHECK:
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, playerGuid);
                        Creature* degrader = ObjectAccessor::GetCreature(*me, degraderGuid);
                        if (!player || !degrader || !degrader->IsAlive())
                        {
                            FailEvent();
                            return;
                        }

                        bool const outsideArena = !IsInsideShartuulBoundary(degrader->GetPositionX(), degrader->GetPositionY());
                        if (!player->isPossessing(degrader))
                        {
                            if (outsideArena)
                            {
                                BounceControlledDemonIntoArena(degrader);
                                transferringDemonControl = true;
                                player->RemoveCharmAuras();
                                player->CastSpell(degrader, SPELL_POSSESS, true);
                                transferringDemonControl = false;
                            }
                            else
                            {
                                FailEvent();
                                return;
                            }
                        }

                        degrader->SetFaction(FactionShartuulControlled);

                        if (outsideArena)
                            BounceControlledDemonIntoArena(degrader);

                        CheckPhaseTwoBossState();
                        ScheduleDoomguardControlledWaveIfCleared();
                        events.ScheduleEvent(EVENT_SHARTUUL_BOUNDARY_CHECK, 1s);
                        break;
                    }
                    case EVENT_SHARTUUL_RESPAWN_IDLE:
                        SpawnIdleState();
                        break;
                    case EVENT_SHARTUUL_IDLE_PATROL:
                        MoveIdleDegrader();
                        break;
                    case EVENT_SHARTUUL_ARENA_VISUALS:
                        RefreshArenaVisuals();
                        break;
                    case EVENT_SHARTUUL_IMP_FIREBOLT:
                        UpdateWaveImps();
                        events.ScheduleEvent(EVENT_SHARTUUL_IMP_FIREBOLT, 2500ms);
                        break;
                    case EVENT_SHARTUUL_DOOMGUARD_PUNISHING_BLOW:
                        CastDoomguardSpell(SPELL_SHARTUUL_DOOMGUARD_PUNISHING_BLOW);
                        if (phaseTwoStarted)
                            events.ScheduleEvent(EVENT_SHARTUUL_DOOMGUARD_PUNISHING_BLOW, 8s);
                        break;
                    case EVENT_SHARTUUL_DOOMGUARD_FEL_FLAMES:
                        StartDoomguardFelFlames();
                        if (phaseTwoStarted)
                            events.ScheduleEvent(EVENT_SHARTUUL_DOOMGUARD_FEL_FLAMES, 18s);
                        break;
                    case EVENT_SHARTUUL_DOOMGUARD_FEL_FLAMES_TICK:
                        TickDoomguardFelFlames();
                        break;
                    case EVENT_SHARTUUL_DOOMGUARD_PHASE_NEXT_WAVE:
                        SpawnDoomguardControlledWave(doomguardControlledWave);
                        break;
                    case EVENT_SHARTUUL_GANARG_CANNON_CHECK:
                        BeginGanargCannonSummon();
                        if (doomguardControlledPhaseStarted && !doomguardControlledPhaseComplete)
                            events.ScheduleEvent(EVENT_SHARTUUL_GANARG_CANNON_CHECK, 12s);
                        break;
                    case EVENT_SHARTUUL_GANARG_CANNON_FINISH:
                        FinishGanargCannonSummon();
                        break;
                    case EVENT_SHARTUUL_CANNON_FIRE:
                        FirePortableFelCannons();
                        if (doomguardControlledPhaseStarted)
                            events.ScheduleEvent(EVENT_SHARTUUL_CANNON_FIRE, 2s);
                        break;
                    case EVENT_SHARTUUL_MOARG_ACID_GEYSER:
                        break;
                    case EVENT_SHARTUUL_GANARG_MOARG_SHIELD_CHECK:
                        BeginMoargShieldChannel();
                        if (doomguardControlledPhaseStarted && !doomguardControlledPhaseComplete && GetAliveWaveCreature(NPC_SHARTUUL_MOARG_TORMENTER))
                            events.ScheduleEvent(EVENT_SHARTUUL_GANARG_MOARG_SHIELD_CHECK, 14s);
                        break;
                    case EVENT_SHARTUUL_GANARG_MOARG_SHIELD_FINISH:
                        FinishMoargShieldChannel();
                        break;
                    case EVENT_SHARTUUL_PHASE_THREE_START:
                        if (shivanControlledPhaseStarted)
                            SpawnPhaseThreeBoss();
                        else
                            StartShivanControlledPhase();
                        break;
                    case EVENT_SHARTUUL_PHASE_THREE_SHARTUUL_ACTIVATE:
                        ActivateFinalShartuul();
                        break;
                    case EVENT_SHARTUUL_PHASE_THREE_TENTACLES:
                        SpawnPhaseThreeTentacles();
                        break;
                    case EVENT_SHARTUUL_TEST_PORTAL_VISUALS:
                        if (!eventActive)
                            RefreshPortalVisualTestHarness();
                        break;
                    case EVENT_SHARTUUL_TEST_EYE_INTRO_PORTAL:
                        TestEyeIntroOpenPortal();
                        break;
                    case EVENT_SHARTUUL_TEST_EYE_INTRO_SHARTUUL:
                        TestEyeIntroSpawnShartuul();
                        break;
                    case EVENT_SHARTUUL_TEST_EYE_INTRO_YELL:
                        TestEyeIntroYell();
                        break;
                    case EVENT_SHARTUUL_TEST_EYE_INTRO_LAUGH:
                        TestEyeIntroLaugh();
                        break;
                    case EVENT_SHARTUUL_TEST_EYE_INTRO_EYE:
                        TestEyeIntroSpawnEye();
                        break;
                    default:
                        break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shartuul_event_controllerAI(creature);
    }
};

class spell_item_crystalforged_darkrune : public SpellScript
{
    PrepareSpellScript(spell_item_crystalforged_darkrune);

    void HandleAfterHit()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        Unit* target = GetHitUnit();
        if (!player || !target || target->GetEntry() != NPC_FELGUARD_DEGRADER)
            return;

        if (Creature* controller = GetShartuulController(target))
            if (npc_shartuul_event_controller::npc_shartuul_event_controllerAI* ai = CAST_AI(npc_shartuul_event_controller::npc_shartuul_event_controllerAI, controller->AI()))
                ai->StartFromRune(player, target);
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_item_crystalforged_darkrune::HandleAfterHit);
    }
};

class spell_shartuul_smash_shield : public SpellScript
{
    PrepareSpellScript(spell_shartuul_smash_shield);

    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        if (!caster || caster->GetEntry() != NPC_FELGUARD_DEGRADER)
            return SPELL_CAST_OK;

        return caster->isMoving() ? SPELL_FAILED_MOVING : SPELL_CAST_OK;
    }

    void PreventFelguardDamage()
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || caster->GetEntry() != NPC_FELGUARD_DEGRADER || target != caster)
            return;

        SetHitDamage(0);
    }

    void PreventFelguardMovementEffect(SpellEffIndex effIndex)
    {
        Unit* caster = GetCaster();
        if (!caster || caster->GetEntry() != NPC_FELGUARD_DEGRADER)
            return;

        SpellEffectInfo const& effect = GetSpellInfo()->Effects[effIndex];
        switch (effect.Effect)
        {
            case SPELL_EFFECT_LEAP:
            case SPELL_EFFECT_JUMP:
            case SPELL_EFFECT_JUMP_DEST:
            case SPELL_EFFECT_CHARGE:
            case SPELL_EFFECT_LEAP_BACK:
            case SPELL_EFFECT_CHARGE_DEST:
                PreventHitDefaultEffect(effIndex);
                break;
            default:
                break;
        }
    }

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        if (!caster || caster->GetEntry() != NPC_FELGUARD_DEGRADER)
            return;

        if (Creature* controller = GetShartuulController(caster))
        {
            controller->AI()->SetGUID(caster->GetCharmerGUID(), DATA_SHARTUUL_PLAYER);
            controller->AI()->SetGUID(caster->GetGUID(), DATA_SHARTUUL_DEGRADER);
            controller->AI()->DoAction(ACTION_SHARTUUL_START_PHASE_ONE);
        }
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_shartuul_smash_shield::CheckCast);
        OnEffectHitTarget += SpellEffectFn(spell_shartuul_smash_shield::PreventFelguardMovementEffect, EFFECT_0, SPELL_EFFECT_ANY);
        OnEffectHitTarget += SpellEffectFn(spell_shartuul_smash_shield::PreventFelguardMovementEffect, EFFECT_1, SPELL_EFFECT_ANY);
        OnEffectHitTarget += SpellEffectFn(spell_shartuul_smash_shield::PreventFelguardMovementEffect, EFFECT_2, SPELL_EFFECT_ANY);
        OnHit += SpellHitFn(spell_shartuul_smash_shield::PreventFelguardDamage);
        AfterCast += SpellCastFn(spell_shartuul_smash_shield::HandleAfterCast);
    }
};

class spell_shartuul_doomguard_fel_flames : public SpellScript
{
    PrepareSpellScript(spell_shartuul_doomguard_fel_flames);

    ObjectGuid targetGuid;

    Unit* ResolveTarget(Unit* caster)
    {
        if (!caster)
            return nullptr;

        if (Unit* target = GetExplTargetUnit())
            return target;

        if (Player* player = ObjectAccessor::GetPlayer(*caster, caster->GetCharmerGUID()))
            if (Unit* target = player->GetSelectedUnit())
                return target;

        if (Unit* target = ObjectAccessor::GetUnit(*caster, caster->GetGuidValue(UNIT_FIELD_TARGET)))
            return target;

        return caster->GetVictim();
    }

    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        if (caster && ShartuulFelFlamesVisualCasters.find(caster->GetGUID()) != ShartuulFelFlamesVisualCasters.end())
            return SPELL_CAST_OK;

        if (!caster || caster->GetEntry() != NPC_DOOMGUARD_PUNISHER || !caster->isPossessed())
            return SPELL_CAST_OK;

        Unit* target = ResolveTarget(caster);
        if (!target || !target->IsAlive())
            return SPELL_FAILED_BAD_TARGETS;

        Creature* doomguard = caster->ToCreature();
        if (!doomguard)
            return SPELL_CAST_OK;

        doomguard->ClearUnitState(UNIT_STATE_CASTING);

        if (npc_shartuul_doomguard_punisher::npc_shartuul_doomguard_punisherAI* ai = CAST_AI(npc_shartuul_doomguard_punisher::npc_shartuul_doomguard_punisherAI, doomguard->AI()))
            ai->StartControlledFelFlames(target);

        doomguard->AddSpellCooldown(SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES, 0, 8000);

        if (Player* player = ObjectAccessor::GetPlayer(*doomguard, doomguard->GetCharmerGUID()))
        {
            WorldPacket cooldownPacket;
            doomguard->BuildCooldownPacket(cooldownPacket, SPELL_COOLDOWN_FLAG_INCLUDE_GCD, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES, 8000);
            player->GetSession()->SendPacket(&cooldownPacket);
        }

        return SPELL_FAILED_DONT_REPORT;
    }

    void PreventBurstDamage()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (Unit* target = GetHitUnit())
            targetGuid = target->GetGUID();

        SetHitDamage(0);
    }

    void StartScriptedTicks()
    {
        Unit* caster = GetCaster();
        if (caster && ShartuulFelFlamesVisualCasters.find(caster->GetGUID()) != ShartuulFelFlamesVisualCasters.end())
            return;

        if (!caster || caster->GetEntry() != NPC_DOOMGUARD_PUNISHER)
            return;

        if (!caster->isPossessed() || targetGuid.IsEmpty())
            return;

        for (uint8 i = 1; i <= SHARTUUL_DOOMGUARD_FEL_FLAMES_TICKS; ++i)
        {
            caster->m_Events.AddEventAtOffset([caster, targetGuid = targetGuid]
            {
                Unit* target = ObjectAccessor::GetUnit(*caster, targetGuid);
                if (!caster || !caster->IsAlive() || !target || !target->IsAlive())
                    return;

                Unit::DealDamage(caster, target, 1000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE);
            }, Milliseconds(1000 * i));
        }

        caster->ClearUnitState(UNIT_STATE_CASTING);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_shartuul_doomguard_fel_flames::CheckCast);
        OnHit += SpellHitFn(spell_shartuul_doomguard_fel_flames::PreventBurstDamage);
        AfterCast += SpellCastFn(spell_shartuul_doomguard_fel_flames::StartScriptedTicks);
    }
};

class spell_shartuul_war_stomp_visual : public SpellScript
{
    PrepareSpellScript(spell_shartuul_war_stomp_visual);

    void PreventDamage()
    {
        SetHitDamage(0);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_shartuul_war_stomp_visual::PreventDamage);
    }
};

class spell_shartuul_doomguard_punishing_blow : public SpellScript
{
    PrepareSpellScript(spell_shartuul_doomguard_punishing_blow);

    void ScaleDamage()
    {
        Unit* caster = GetCaster();
        if (!caster || caster->GetEntry() != NPC_DOOMGUARD_PUNISHER)
            return;

        SetHitDamage(urand(2000, 3000));
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_shartuul_doomguard_punishing_blow::ScaleDamage);
    }
};

class spell_shartuul_doomguard_consume_essence : public SpellScript
{
    PrepareSpellScript(spell_shartuul_doomguard_consume_essence);

    Unit* ResolveTarget()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return nullptr;

        if (Unit* target = GetExplTargetUnit())
            return target;

        if (Unit* target = ObjectAccessor::GetUnit(*caster, caster->GetGuidValue(UNIT_FIELD_TARGET)))
            return target;

        if (Player* player = ObjectAccessor::GetPlayer(*caster, caster->GetCharmerGUID()))
            if (Unit* target = player->GetSelectedUnit())
                return target;

        return caster->GetVictim();
    }

    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        if (!caster || caster->GetEntry() != NPC_DOOMGUARD_PUNISHER)
            return SPELL_CAST_OK;

        Unit* target = ResolveTarget();
        if (!target || target->GetEntry() != NPC_SHARTUUL_GANARG_UNDERLING)
            return SPELL_FAILED_BAD_TARGETS;

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_shartuul_doomguard_consume_essence::CheckCast);
    }
};

class spell_shartuul_doomguard_throw_axe : public SpellScript
{
    PrepareSpellScript(spell_shartuul_doomguard_throw_axe);

    Unit* ResolveDoomguardCaster()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return nullptr;

        if (caster->GetEntry() == NPC_DOOMGUARD_PUNISHER)
            return caster;

        if (Unit* charm = caster->GetCharm())
            if (charm->GetEntry() == NPC_DOOMGUARD_PUNISHER)
                return charm;

        if (Unit* originalCaster = GetOriginalCaster())
        {
            if (originalCaster->GetEntry() == NPC_DOOMGUARD_PUNISHER)
                return originalCaster;

            if (Unit* charm = originalCaster->GetCharm())
                if (charm->GetEntry() == NPC_DOOMGUARD_PUNISHER)
                    return charm;
        }

        return nullptr;
    }

    Unit* ResolveTarget(Unit* doomguard)
    {
        if (!doomguard)
            return nullptr;

        if (Player* player = ObjectAccessor::GetPlayer(*doomguard, doomguard->GetCharmerGUID()))
            if (Unit* target = player->GetSelectedUnit())
                if (target != doomguard)
                    return target;

        if (Unit* target = ObjectAccessor::GetUnit(*doomguard, doomguard->GetGuidValue(UNIT_FIELD_TARGET)))
            if (target != doomguard)
                return target;

        if (Unit* target = doomguard->GetVictim())
            if (target != doomguard)
                return target;

        if (Player* playerCaster = GetCaster() ? GetCaster()->ToPlayer() : nullptr)
            if (Unit* target = playerCaster->GetSelectedUnit())
                if (target != doomguard)
                    return target;

        if (Unit* target = GetExplTargetUnit())
        {
            if (target == doomguard)
                return nullptr;

            return target;
        }

        return nullptr;
    }

    void HandleCast()
    {
        Unit* doomguardUnit = ResolveDoomguardCaster();
        if (!doomguardUnit)
            return;

        Creature* doomguard = doomguardUnit->ToCreature();
        Unit* target = ResolveTarget(doomguardUnit);
        if (!doomguard || !target || !target->IsAlive())
            return;

        if (npc_shartuul_doomguard_punisher::npc_shartuul_doomguard_punisherAI* ai = CAST_AI(npc_shartuul_doomguard_punisher::npc_shartuul_doomguard_punisherAI, doomguard->AI()))
            ai->StartThrowAxe(target);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_shartuul_doomguard_throw_axe::HandleCast);
    }
};

class spell_shartuul_doomguard_super_jump : public SpellScript
{
    PrepareSpellScript(spell_shartuul_doomguard_super_jump);

    ObjectGuid targetGuid;
    bool jumpTriggered = false;

    Unit* ResolveDoomguardCaster()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return nullptr;

        if (caster->GetEntry() == NPC_DOOMGUARD_PUNISHER)
            return caster;

        if (Unit* charm = caster->GetCharm())
            if (charm->GetEntry() == NPC_DOOMGUARD_PUNISHER)
                return charm;

        if (Unit* originalCaster = GetOriginalCaster())
        {
            if (originalCaster->GetEntry() == NPC_DOOMGUARD_PUNISHER)
                return originalCaster;

            if (Unit* charm = originalCaster->GetCharm())
                if (charm->GetEntry() == NPC_DOOMGUARD_PUNISHER)
                    return charm;
        }

        return nullptr;
    }

    Unit* ResolveTarget(Unit* doomguard)
    {
        if (!doomguard)
            return nullptr;

        if (Player* player = ObjectAccessor::GetPlayer(*doomguard, doomguard->GetCharmerGUID()))
            if (Unit* target = player->GetSelectedUnit())
                if (target != doomguard)
                    return target;

        if (Unit* target = ObjectAccessor::GetUnit(*doomguard, doomguard->GetGuidValue(UNIT_FIELD_TARGET)))
            if (target != doomguard)
                return target;

        if (Unit* target = doomguard->GetVictim())
            if (target != doomguard)
                return target;

        if (Player* playerCaster = GetCaster() ? GetCaster()->ToPlayer() : nullptr)
            if (Unit* target = playerCaster->GetSelectedUnit())
                if (target != doomguard)
                    return target;

        if (Unit* target = GetExplTargetUnit())
        {
            if (target == doomguard)
                return nullptr;

            return target;
        }

        return nullptr;
    }

    SpellCastResult CheckCast()
    {
        Unit* doomguard = ResolveDoomguardCaster();
        if (!doomguard)
            return SPELL_CAST_OK;

        if (ShartuulNativeSuperJumpCasters.find(doomguard->GetGUID()) != ShartuulNativeSuperJumpCasters.end())
            return SPELL_CAST_OK;

        Unit* target = ResolveTarget(doomguard);
        if (!target || !target->IsAlive())
            return SPELL_FAILED_BAD_TARGETS;

        targetGuid = target->GetGUID();
        Player* player = ObjectAccessor::GetPlayer(*doomguard, doomguard->GetCharmerGUID());

        if (player)
            ShartuulPendingSuperJumpRequests[player->GetGUID()] = { doomguard->GetGUID(), target->GetGUID() };
        else if (Creature* creature = doomguard->ToCreature())
            if (npc_shartuul_doomguard_punisher::npc_shartuul_doomguard_punisherAI* ai = CAST_AI(npc_shartuul_doomguard_punisher::npc_shartuul_doomguard_punisherAI, creature->AI()))
                ai->StartSuperJump(target);

        jumpTriggered = true;
        if (Creature* creature = doomguard->ToCreature())
        {
            creature->AddSpellCooldown(SPELL_SHARTUUL_DOOMGUARD_SUPER_JUMP, 0, 15000);
            if (player)
            {
                WorldPacket cooldownPacket;
                creature->BuildCooldownPacket(cooldownPacket, SPELL_COOLDOWN_FLAG_INCLUDE_GCD, SPELL_SHARTUUL_DOOMGUARD_SUPER_JUMP, 15000);
                player->GetSession()->SendPacket(&cooldownPacket);
            }
        }

        return SPELL_FAILED_DONT_REPORT;
    }

    void TriggerSuperJump()
    {
        if (Unit* doomguardUnit = ResolveDoomguardCaster())
        {
            auto itr = ShartuulNativeSuperJumpCasters.find(doomguardUnit->GetGUID());
            if (itr != ShartuulNativeSuperJumpCasters.end())
            {
                ShartuulNativeSuperJumpCasters.erase(itr);
                return;
            }
        }

        if (jumpTriggered)
            return;

        Unit* doomguardUnit = ResolveDoomguardCaster();
        if (!doomguardUnit)
            return;

        Creature* doomguard = doomguardUnit->ToCreature();
        Unit* target = ObjectAccessor::GetUnit(*doomguardUnit, targetGuid);
        if (!target)
            target = ResolveTarget(doomguardUnit);

        if (!doomguard || !target)
            return;

        if (npc_shartuul_doomguard_punisher::npc_shartuul_doomguard_punisherAI* ai = CAST_AI(npc_shartuul_doomguard_punisher::npc_shartuul_doomguard_punisherAI, doomguard->AI()))
        {
            jumpTriggered = true;
            ai->StartSuperJump(target);
        }
    }

    void PreventDefaultJumpEffect(SpellEffIndex effIndex)
    {
        Unit* doomguard = ResolveDoomguardCaster();
        if (!doomguard)
            return;

        if (ShartuulNativeSuperJumpCasters.find(doomguard->GetGUID()) != ShartuulNativeSuperJumpCasters.end())
            return;

        PreventHitDefaultEffect(effIndex);
    }

    void PreventDefaultJumpLaunch(SpellEffIndex effIndex)
    {
        Unit* doomguard = ResolveDoomguardCaster();
        if (!doomguard)
            return;

        if (ShartuulNativeSuperJumpCasters.find(doomguard->GetGUID()) != ShartuulNativeSuperJumpCasters.end())
            return;

        PreventHitDefaultEffect(effIndex);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_shartuul_doomguard_super_jump::CheckCast);
        OnEffectLaunchTarget += SpellEffectFn(spell_shartuul_doomguard_super_jump::PreventDefaultJumpLaunch, EFFECT_0, SPELL_EFFECT_ANY);
        OnEffectLaunchTarget += SpellEffectFn(spell_shartuul_doomguard_super_jump::PreventDefaultJumpLaunch, EFFECT_1, SPELL_EFFECT_ANY);
        OnEffectLaunchTarget += SpellEffectFn(spell_shartuul_doomguard_super_jump::PreventDefaultJumpLaunch, EFFECT_2, SPELL_EFFECT_ANY);
        OnEffectHitTarget += SpellEffectFn(spell_shartuul_doomguard_super_jump::PreventDefaultJumpEffect, EFFECT_0, SPELL_EFFECT_ANY);
        OnEffectHitTarget += SpellEffectFn(spell_shartuul_doomguard_super_jump::PreventDefaultJumpEffect, EFFECT_1, SPELL_EFFECT_ANY);
        OnEffectHitTarget += SpellEffectFn(spell_shartuul_doomguard_super_jump::PreventDefaultJumpEffect, EFFECT_2, SPELL_EFFECT_ANY);
        AfterCast += SpellCastFn(spell_shartuul_doomguard_super_jump::TriggerSuperJump);
    }
};

class player_shartuul_possession_spell_handler : public PlayerScript
{
public:
    player_shartuul_possession_spell_handler() : PlayerScript("player_shartuul_possession_spell_handler", { PLAYERHOOK_ON_SPELL_CAST, PLAYERHOOK_ON_UPDATE }) { }

    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        if (!player)
            return;

        auto felFlamesItr = ShartuulPendingFelFlamesCasts.find(player->GetGUID());
        if (felFlamesItr != ShartuulPendingFelFlamesCasts.end())
        {
            ShartuulPendingFelFlames& felFlames = felFlamesItr->second;
            Creature* doomguard = ObjectAccessor::GetCreature(*player, felFlames.DoomguardGuid);
            Unit* target = doomguard ? ObjectAccessor::GetUnit(*doomguard, felFlames.TargetGuid) : nullptr;

            if (!doomguard || !doomguard->IsAlive() || !target || !target->IsAlive())
            {
                if (doomguard)
                {
                    doomguard->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, ObjectGuid::Empty);
                    doomguard->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
                }

                if (doomguard)
                    if (Creature* helper = ObjectAccessor::GetCreature(*doomguard, felFlames.HelperGuid))
                    {
                        ShartuulFelFlamesVisualCasters.erase(helper->GetGUID());
                        helper->DespawnOrUnsummon();
                    }

                ShartuulPendingFelFlamesCasts.erase(felFlamesItr);
            }
            else
            {
                if (felFlames.VisualTimer <= diff)
                {
                    doomguard->SetFacingToObject(target);
                    doomguard->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, target->GetGUID());
                    doomguard->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES);
                    doomguard->ClearUnitState(UNIT_STATE_CASTING);

                    if (Creature* helper = ObjectAccessor::GetCreature(*doomguard, felFlames.HelperGuid))
                    {
                        helper->NearTeleportTo(doomguard->GetPositionX(), doomguard->GetPositionY(), doomguard->GetPositionZ() + 1.0f, doomguard->GetOrientation());
                        helper->SetFacingToObject(target);
                        helper->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, target->GetGUID());
                        helper->SetUInt32Value(UNIT_CHANNEL_SPELL, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES);

                        if (felFlames.TicksDone == 0)
                        {
                            ShartuulFelFlamesVisualCasters.insert(helper->GetGUID());
                            helper->CastSpell(target, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES, false);
                        }
                    }

                    if (felFlames.TicksDone == 0)
                    {
                        ShartuulFelFlamesVisualCasters.insert(doomguard->GetGUID());
                        doomguard->CastSpell(target, SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES, false);
                        doomguard->ClearUnitState(UNIT_STATE_CASTING);
                    }

                    felFlames.VisualTimer = 250;
                }
                else
                    felFlames.VisualTimer -= diff;

                if (felFlames.TickTimer <= diff)
                {
                    if (felFlames.TicksDone < SHARTUUL_DOOMGUARD_FEL_FLAMES_TICKS && !IsShartuulProtectedAmbient(target))
                    {
                        Unit::DealDamage(doomguard, target, 1000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE);
                        ++felFlames.TicksDone;
                    }

                    felFlames.TickTimer = 1000;
                }
                else
                    felFlames.TickTimer -= diff;

                if (felFlames.DurationTimer <= diff)
                {
                    doomguard->SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, ObjectGuid::Empty);
                    doomguard->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
                    doomguard->ClearUnitState(UNIT_STATE_CASTING);
                    ShartuulFelFlamesVisualCasters.erase(doomguard->GetGUID());

                    if (Creature* helper = ObjectAccessor::GetCreature(*doomguard, felFlames.HelperGuid))
                    {
                        ShartuulFelFlamesVisualCasters.erase(helper->GetGUID());
                        helper->DespawnOrUnsummon();
                    }

                    ShartuulPendingFelFlamesCasts.erase(felFlamesItr);
                }
                else
                    felFlames.DurationTimer -= diff;
            }
        }

        auto requestItr = ShartuulPendingSuperJumpRequests.find(player->GetGUID());
        if (requestItr != ShartuulPendingSuperJumpRequests.end())
        {
            ShartuulPendingSuperJumpRequest request = requestItr->second;
            ShartuulPendingSuperJumpRequests.erase(requestItr);

            Creature* doomguard = ObjectAccessor::GetCreature(*player, request.DoomguardGuid);
            Unit* target = ObjectAccessor::GetUnit(*player, request.TargetGuid);
            if (doomguard && doomguard->IsAlive() && target && target->IsAlive())
            {
                Position landing = target->GetPosition();
                float const dxToDoomguard = doomguard->GetPositionX() - target->GetPositionX();
                float const dyToDoomguard = doomguard->GetPositionY() - target->GetPositionY();
                float const distToDoomguard = std::sqrt(dxToDoomguard * dxToDoomguard + dyToDoomguard * dyToDoomguard);
                float const landingOffset = target->GetCombatReach() + doomguard->GetCombatReach() + 0.75f;
                if (distToDoomguard > 0.1f)
                {
                    landing.m_positionX += (dxToDoomguard / distToDoomguard) * landingOffset;
                    landing.m_positionY += (dyToDoomguard / distToDoomguard) * landingOffset;
                }

                float const jumpDist = doomguard->GetExactDist2d(landing.GetPositionX(), landing.GetPositionY());
                doomguard->ClearUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING);
                doomguard->SetControlled(false, UNIT_STATE_ROOT);
                doomguard->StopMoving();
                doomguard->GetMotionMaster()->Clear();
                doomguard->CastSpell(doomguard, SPELL_SHARTUUL_WAR_STOMP_VISUAL, true);

                if (jumpDist > 0.1f)
                {
                    uint32 const entries[] =
                    {
                        NPC_SHARTUUL_WAVE_FELHOUND,
                        NPC_SHARTUUL_WAVE_IMP,
                        NPC_SHARTUUL_GANARG_UNDERLING,
                        NPC_SHARTUUL_MOARG_TORMENTER,
                        NPC_SHARTUUL_PORTABLE_FEL_CANNON,
                        NPC_SHARTUUL_SHIVAN_ASSASSIN
                    };

                    for (uint32 entry : entries)
                    {
                        std::list<Creature*> creatures;
                        GetCreatureListWithEntryInGrid(creatures, doomguard, entry, 6.0f);
                        for (Creature* creature : creatures)
                        {
                            if (!creature || !creature->IsAlive() || creature == doomguard || IsShartuulProtectedAmbient(creature))
                                continue;

                            ApplyShartuulEventStun(creature, 5000);
                        }
                    }

                    doomguard->GetMotionMaster()->MoveJump(landing.GetPositionX(), landing.GetPositionY(), landing.GetPositionZ(), 24.0f, 12.0f);
                    ShartuulPendingSuperJumpImpacts[player->GetGUID()] = { doomguard->GetGUID(), target->GetGUID(), std::clamp<uint32>(uint32(jumpDist / 24.0f * 1000.0f) + 300, 700, 1800) };
                }
            }
        }

        auto itr = ShartuulPendingSuperJumpImpacts.find(player->GetGUID());
        if (itr == ShartuulPendingSuperJumpImpacts.end())
            return;

        if (itr->second.Timer > diff)
        {
            itr->second.Timer -= diff;
            return;
        }

        ShartuulPendingSuperJumpImpact impact = itr->second;
        ShartuulPendingSuperJumpImpacts.erase(itr);

        Creature* doomguard = ObjectAccessor::GetCreature(*player, impact.DoomguardGuid);
        if (!doomguard || !doomguard->IsAlive())
            return;

        Unit* impactCenter = ObjectAccessor::GetUnit(*doomguard, impact.TargetGuid);
        if (!impactCenter || !impactCenter->IsAlive())
            impactCenter = doomguard;

        if (Unit* target = ObjectAccessor::GetUnit(*doomguard, impact.TargetGuid))
        {
            doomguard->SetFacingToObject(target);
            if (!doomguard->GetVictim())
                doomguard->AI()->AttackStart(target);
        }
    }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!player || !spell)
            return;

        SpellInfo const* spellInfo = spell->GetSpellInfo();
        if (!spellInfo || spellInfo->Id != SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES)
            return;

        Unit* charm = player->GetCharm();
        if (!charm || charm->GetEntry() != NPC_DOOMGUARD_PUNISHER)
            return;

        Creature* doomguard = charm->ToCreature();
        if (!doomguard)
            return;

        Unit* target = player->GetSelectedUnit();
        if (!target)
            target = ObjectAccessor::GetUnit(*doomguard, doomguard->GetGuidValue(UNIT_FIELD_TARGET));
        if (!target)
            target = doomguard->GetVictim();

        if (!target || !target->IsAlive())
            return;

        if (npc_shartuul_doomguard_punisher::npc_shartuul_doomguard_punisherAI* ai = CAST_AI(npc_shartuul_doomguard_punisher::npc_shartuul_doomguard_punisherAI, doomguard->AI()))
        {
            ai->StartControlledFelFlames(target);
        }
    }
};

class all_spell_shartuul_possession_spell_handler : public AllSpellScript
{
public:
    all_spell_shartuul_possession_spell_handler() : AllSpellScript("all_spell_shartuul_possession_spell_handler", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST, ALLSPELLHOOK_ON_CAST }) { }

    Creature* ResolveDoomguard(Unit* caster) const
    {
        if (!caster)
            return nullptr;

        if (caster->GetEntry() == NPC_DOOMGUARD_PUNISHER)
            return caster->ToCreature();

        if (Unit* charm = caster->GetCharm())
            if (charm->GetEntry() == NPC_DOOMGUARD_PUNISHER)
                return charm->ToCreature();

        return nullptr;
    }

    Unit* ResolveTarget(Spell* spell, Creature* doomguard) const
    {
        if (!doomguard)
            return nullptr;

        if (spell)
            if (Unit* target = spell->m_targets.GetUnitTarget())
                return target;

        if (Player* player = ObjectAccessor::GetPlayer(*doomguard, doomguard->GetCharmerGUID()))
            if (Unit* target = player->GetSelectedUnit())
                return target;

        if (Unit* target = ObjectAccessor::GetUnit(*doomguard, doomguard->GetGuidValue(UNIT_FIELD_TARGET)))
            return target;

        return doomguard->GetVictim();
    }

    void HandleDoomguardSpell(Spell* spell, Unit* caster, SpellInfo const* spellInfo, bool fromCheckCast, SpellCastResult* result = nullptr)
    {
        if (!spellInfo || spellInfo->Id != SPELL_SHARTUUL_DOOMGUARD_FEL_FLAMES)
            return;

        Creature* doomguard = ResolveDoomguard(caster);
        if (!doomguard || !doomguard->isPossessed())
            return;

        Unit* target = ResolveTarget(spell, doomguard);
        if (!target || !target->IsAlive())
            return;

        if (npc_shartuul_doomguard_punisher::npc_shartuul_doomguard_punisherAI* ai = CAST_AI(npc_shartuul_doomguard_punisher::npc_shartuul_doomguard_punisherAI, doomguard->AI()))
        {
            if (!fromCheckCast)
                ai->StartControlledFelFlames(target);
        }
    }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& res) override
    {
        if (!spell)
            return;

        HandleDoomguardSpell(spell, spell->GetCaster(), spell->GetSpellInfo(), true, &res);
    }

    void OnSpellCast(Spell* spell, Unit* caster, SpellInfo const* spellInfo, bool /*skipCheck*/) override
    {
        HandleDoomguardSpell(spell, caster, spellInfo, false);
    }
};

class all_spell_shartuul_shivan_spell_handler : public AllSpellScript
{
public:
    all_spell_shartuul_shivan_spell_handler() : AllSpellScript("all_spell_shartuul_shivan_spell_handler", { ALLSPELLHOOK_ON_CAST }) { }

    Creature* ResolveShivan(Unit* caster) const
    {
        if (!caster)
            return nullptr;

        if (caster->GetEntry() == NPC_SHARTUUL_SHIVAN_ASSASSIN)
            return caster->ToCreature();

        if (Unit* charm = caster->GetCharm())
            if (charm->GetEntry() == NPC_SHARTUUL_SHIVAN_ASSASSIN)
                return charm->ToCreature();

        return nullptr;
    }

    void OnSpellCast(Spell* /*spell*/, Unit* caster, SpellInfo const* spellInfo, bool /*skipCheck*/) override
    {
        if (!spellInfo)
            return;

        Creature* shivan = ResolveShivan(caster);
        if (!shivan || !shivan->isPossessed())
            return;

        switch (spellInfo->Id)
        {
            case SPELL_SHARTUUL_SHIVAN_ASPECT_SHADOW:
                SetShartuulShivanSpells(shivan, ShartuulShivanAspect::Shadow);
                break;
            case SPELL_SHARTUUL_SHIVAN_ASPECT_FLAME:
                SetShartuulShivanSpells(shivan, ShartuulShivanAspect::Flame);
                break;
            case SPELL_SHARTUUL_SHIVAN_ASPECT_ICE:
                SetShartuulShivanSpells(shivan, ShartuulShivanAspect::Ice);
                break;
            case SPELL_SHARTUUL_SHIVAN_ICE_BLOCK:
                ShartuulShivanIceBlockEndTime[shivan->GetGUID()] = GameTime::GetGameTimeMS().count() + 4000;
                shivan->CastSpell(shivan, SPELL_SHARTUUL_STUN_VISUAL, true);
                break;
            default:
                break;
        }
    }
};

void AddSC_shartuul_event()
{
    new npc_shartuul_felguard_degrader();
    new npc_shartuul_doomguard_punisher();
    new npc_shartuul_wave_melee("npc_shartuul_wave_felhound");
    new npc_shartuul_wave_melee("npc_shartuul_ganarg_underling");
    new npc_shartuul_wave_imp();
    new npc_shartuul_portable_fel_cannon();
    new npc_shartuul_shivan_assassin();
    new npc_shartuul_moarg_tormenter();
    new npc_shartuul_eye_of_shartuul();
    new npc_shartuul_dreadmaw();
    new npc_shartuul_eredar();
    new npc_shartuul_event_controller();
    new player_shartuul_possession_spell_handler();
    new all_spell_shartuul_possession_spell_handler();
    new all_spell_shartuul_shivan_spell_handler();
    RegisterSpellScript(spell_item_crystalforged_darkrune);
    RegisterSpellScript(spell_shartuul_smash_shield);
    RegisterSpellScript(spell_shartuul_doomguard_fel_flames);
    RegisterSpellScript(spell_shartuul_war_stomp_visual);
    RegisterSpellScript(spell_shartuul_doomguard_punishing_blow);
    RegisterSpellScript(spell_shartuul_doomguard_consume_essence);
    RegisterSpellScript(spell_shartuul_doomguard_throw_axe);
    RegisterSpellScript(spell_shartuul_doomguard_super_jump);
}
