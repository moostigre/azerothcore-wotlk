/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "TC9CorpseTransfer.h"

#include "Creature.h"
#include "GameTime.h"
#include "Group.h"
#include "GroupMgr.h"
#include "Log.h"
#include "LootMgr.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "Random.h"
#include "TC9Sidecar.h"
#include "TemporarySummon.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <algorithm>
#include <limits>
#include <memory>

namespace
{
constexpr uint8 SnapshotVersion = 1;
constexpr uint8 SnapshotUpsert = 1;
constexpr uint8 SnapshotRemove = 2;
constexpr std::size_t MaxSnapshotBytes = 64 * 1024;
constexpr uint8 MaxTransferredPlayers = 40;
constexpr std::size_t MaxTrackedCorpsesPerCarrier = 1024;
constexpr uint64 MaxTransferredLifetime = 24 * HOUR;
constexpr uint64 TransferredRollLootGrace = MINUTE;

struct DecodedLoot
{
    Loot loot;
    GuidSet viewers;
    ObjectGuid recipient;
    ObjectGuid::LowType recipientGroup{0};
    uint32 entry{0};
    uint32 phaseMask{PHASEMASK_NORMAL};
    Position position;
    struct RollState
    {
        std::vector<std::pair<ObjectGuid, uint8>> votes;
        uint64 deadline{0};
        uint8 voteMask{0};
    };
    std::vector<RollState> itemRolls;
    std::vector<RollState> questItemRolls;
};

void DeleteQuestItemMap(QuestItemMap& map)
{
    for (auto const& pair : map)
        delete pair.second;
    map.clear();
}

struct QuestItemMapsCleanup
{
    QuestItemMap& quest;
    QuestItemMap& ffa;
    QuestItemMap& conditional;

    ~QuestItemMapsCleanup()
    {
        DeleteQuestItemMap(quest);
        DeleteQuestItemMap(ffa);
        DeleteQuestItemMap(conditional);
    }
};

void WriteGuidSet(WorldPacket& packet, GuidSet const& guids)
{
    uint8 count = uint8(std::min<std::size_t>(guids.size(), MaxTransferredPlayers));
    packet << count;
    auto itr = guids.begin();
    for (uint8 i = 0; i < count; ++i, ++itr)
        packet << uint64(itr->GetRawValue());
}

bool ReadGuidSet(WorldPacket& packet, GuidSet& guids)
{
    uint8 count = 0;
    packet >> count;
    if (count > MaxTransferredPlayers)
        return false;

    for (uint8 i = 0; i < count; ++i)
    {
        uint64 rawGuid = 0;
        packet >> rawGuid;
        ObjectGuid guid(rawGuid);
        if (!guid.IsPlayer())
            return false;
        guids.insert(guid);
    }
    return true;
}

bool WriteLootItem(WorldPacket& packet, LootItem const& item, GuidSet const& eligibility, uint64 blockedUntil,
    Group* group, Loot* loot, uint8 itemSlot)
{
    bool blocked = item.is_blocked && !item.is_looted;
    if (!blocked)
        blockedUntil = 0;
    uint8 flags = 0;
    flags |= item.is_looted ? 0x01 : 0;
    flags |= blocked ? 0x02 : 0;
    flags |= item.freeforall ? 0x04 : 0;
    flags |= item.is_underthreshold ? 0x08 : 0;
    flags |= item.is_counted ? 0x10 : 0;
    flags |= item.needs_quest ? 0x20 : 0;
    flags |= item.follow_loot_rules ? 0x40 : 0;
    flags |= item.HasConditions() ? 0x80 : 0;

    packet << item.itemid << item.randomSuffix << item.randomPropertyId << item.count << flags << item.groupid;
    packet << uint64(item.rollWinnerGUID.GetRawValue()) << blockedUntil;
    WriteGuidSet(packet, eligibility);

    std::vector<std::pair<ObjectGuid, uint8>> votes;
    uint8 voteMask = 0;
    if (blocked && blockedUntil && (!group || !group->GetTC9RollState(loot, itemSlot, votes, voteMask)))
        return false;
    packet << voteMask << uint8(votes.size());
    for (auto const& vote : votes)
        packet << uint64(vote.first.GetRawValue()) << vote.second;
    return true;
}

bool IsSubset(GuidSet const& subset, GuidSet const& superset)
{
    return std::all_of(subset.begin(), subset.end(), [&superset](ObjectGuid const& guid)
    {
        return superset.find(guid) != superset.end();
    });
}

bool ReadLootItem(WorldPacket& packet, LootItem& item, uint32 index, GuidSet const& viewers, uint64 expiresAt,
    DecodedLoot::RollState& rollState)
{
    uint8 flags = 0;
    uint8 count = 0;
    uint8 groupId = 0;
    uint64 rollWinner = 0;
    uint64 blockedUntil = 0;
    packet >> item.itemid >> item.randomSuffix >> item.randomPropertyId >> count >> flags >> groupId >>
        rollWinner >> blockedUntil;
    item.itemIndex = index;
    item.count = count;
    item.groupid = groupId;
    item.is_looted = (flags & 0x01) != 0;
    bool wasBlocked = (flags & 0x02) != 0;
    if ((wasBlocked && blockedUntil > expiresAt) || (!wasBlocked && blockedUntil))
        return false;
    item.is_blocked = wasBlocked;
    item.freeforall = (flags & 0x04) != 0;
    item.is_underthreshold = (flags & 0x08) != 0;
    item.is_counted = (flags & 0x10) != 0;
    item.needs_quest = (flags & 0x20) != 0;
    item.follow_loot_rules = (flags & 0x40) != 0;
    item.had_transferred_conditions = (flags & 0x80) != 0;
    item.uses_transferred_eligibility = true;
    item.rollWinnerGUID = ObjectGuid(rollWinner);
    if (!item.itemid || (!item.is_looted && !item.count) || !sObjectMgr->GetItemTemplate(item.itemid) ||
        !ReadGuidSet(packet, item.allowedGUIDs) || !IsSubset(item.allowedGUIDs, viewers))
        return false;
    if (item.rollWinnerGUID && (!item.rollWinnerGUID.IsPlayer() ||
        item.allowedGUIDs.find(item.rollWinnerGUID) == item.allowedGUIDs.end()))
        return false;

    uint8 voterCount = 0;
    packet >> rollState.voteMask >> voterCount;
    if (voterCount > MaxTransferredPlayers || (rollState.voteMask & ~ROLL_ALL_TYPE_MASK))
        return false;
    rollState.deadline = blockedUntil;
    GuidSet voters;
    for (uint8 i = 0; i < voterCount; ++i)
    {
        uint64 rawGuid = 0;
        uint8 vote = 0;
        packet >> rawGuid >> vote;
        ObjectGuid guid(rawGuid);
        bool voteAllowed = vote == PASS || vote == NOT_EMITED_YET || vote == NOT_VALID ||
            (vote == NEED && (rollState.voteMask & ROLL_FLAG_TYPE_NEED)) ||
            (vote == GREED && (rollState.voteMask & ROLL_FLAG_TYPE_GREED)) ||
            (vote == DISENCHANT && (rollState.voteMask & ROLL_FLAG_TYPE_DISENCHANT));
        if (!guid.IsPlayer() || viewers.find(guid) == viewers.end() || !voteAllowed || !voters.insert(guid).second)
            return false;
        rollState.votes.emplace_back(guid, vote);
    }

    if (!wasBlocked || !blockedUntil)
        return !voterCount && !rollState.voteMask;
    return voterCount && rollState.voteMask;
}

void WriteQuestItemMap(WorldPacket& packet, QuestItemMap const& map)
{
    uint8 playerCount = uint8(std::min<std::size_t>(map.size(), MaxTransferredPlayers));
    packet << playerCount;
    auto mapItr = map.begin();
    for (uint8 i = 0; i < playerCount; ++i, ++mapItr)
    {
        packet << uint64(mapItr->first.GetRawValue());
        QuestItemList const& items = *mapItr->second;
        uint8 itemCount = uint8(std::min<std::size_t>(items.size(), std::numeric_limits<uint8>::max()));
        packet << itemCount;
        for (uint8 itemIndex = 0; itemIndex < itemCount; ++itemIndex)
            packet << items[itemIndex].index << uint8(items[itemIndex].is_looted);
    }
}

bool ReadQuestItemMap(WorldPacket& packet, QuestItemMap& map, uint32 maximumIndex, GuidSet const& viewers)
{
    uint8 playerCount = 0;
    packet >> playerCount;
    if (playerCount > MaxTransferredPlayers)
        return false;

    for (uint8 i = 0; i < playerCount; ++i)
    {
        uint64 rawGuid = 0;
        uint8 itemCount = 0;
        packet >> rawGuid >> itemCount;
        ObjectGuid guid(rawGuid);
        if (!guid.IsPlayer() || viewers.find(guid) == viewers.end() || map.find(guid) != map.end())
            return false;

        std::unique_ptr<QuestItemList> items = std::make_unique<QuestItemList>();
        items->reserve(itemCount);
        for (uint8 itemIndex = 0; itemIndex < itemCount; ++itemIndex)
        {
            uint8 index = 0;
            uint8 looted = 0;
            packet >> index >> looted;
            if (index >= maximumIndex || looted > 1)
                return false;
            items->emplace_back(index, looted != 0);
        }
        map.emplace(guid, items.release());
    }
    return true;
}

GuidSet EligiblePlayersForItem(Creature* creature, LootItem const& item, GuidSet const& viewers)
{
    GuidSet eligible = item.GetAllowedLooters();
    for (ObjectGuid const& guid : viewers)
        if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
            if (player->IsInMap(creature) && item.AllowedForPlayer(player, creature->GetGUID()))
                eligible.insert(guid);
    return eligible;
}
}

TC9CorpseTransfer* TC9CorpseTransfer::instance()
{
    static TC9CorpseTransfer instance;
    return &instance;
}

uint64 TC9CorpseTransfer::GenerateSnapshotId()
{
    uint64 id = 0;
    do
        id = (uint64(urand(1, std::numeric_limits<uint32>::max())) << 32) |
            urand(1, std::numeric_limits<uint32>::max());
    while (_trackedCorpses.find(id) != _trackedCorpses.end());
    return id;
}

uint64 TC9CorpseTransfer::LocationKey(uint32 mapId, uint32 instanceId)
{
    return (uint64(mapId) << 32) | instanceId;
}

void TC9CorpseTransfer::TrackAndPublish(Creature* creature)
{
    if (!sToCloud9Sidecar->ClusterModeEnabled() || !creature || !creature->IsInWorld() || creature->IsAlive() ||
        creature->loot.isLooted())
        return;

    std::lock_guard<std::recursive_mutex> lock(_mutex);

    ObjectGuid carrier = creature->loot.lootOwnerGUID;
    if (!carrier)
        carrier = creature->GetLootRecipientGUID();
    Player* carrierPlayer = ObjectAccessor::FindConnectedPlayer(carrier);
    if (!carrierPlayer || !carrierPlayer->IsInMap(creature))
        return;

    std::size_t trackedForCarrier = std::count_if(
        _trackedCorpses.begin(), _trackedCorpses.end(), [carrier](auto const& pair)
    {
        return pair.second.carrier == carrier;
    });
    if (trackedForCarrier >= MaxTrackedCorpsesPerCarrier)
    {
        creature->SetTC9CorpseTransferData(0, 0, carrier, GuidSet{}, false);
        creature->SetTC9CorpseTransferOverflow(true);
        ++_overflowCorpses[carrier][LocationKey(creature->GetMapId(), creature->GetInstanceId())];
        return;
    }

    TrackedCorpse tracked;
    tracked.creatureGuid = creature->GetGUID();
    tracked.carrier = carrier;
    tracked.mapId = creature->GetMapId();
    tracked.instanceId = creature->GetInstanceId();
    tracked.expiresAt = uint64(creature->GetCorpseRemoveTime());
    tracked.viewers.insert(carrier);

    if (Group* group = creature->GetLootRecipientGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr; itr = itr->next())
            if (Player* member = itr->GetSource())
                if (member->IsInMap(creature) && member->IsAtLootRewardDistance(creature))
                    tracked.viewers.insert(member->GetGUID());

    for (ObjectGuid const& guid : creature->GetAllowedLooters())
        tracked.viewers.insert(guid);

    for (LootItem const& item : creature->loot.items)
        tracked.itemEligibility.push_back(EligiblePlayersForItem(creature, item, tracked.viewers));
    for (LootItem const& item : creature->loot.quest_items)
        tracked.questItemEligibility.push_back(EligiblePlayersForItem(creature, item, tracked.viewers));

    uint64 snapshotId = GenerateSnapshotId();
    creature->SetTC9CorpseTransferData(snapshotId, 0, carrier, tracked.viewers, false);
    _trackedCorpses.emplace(snapshotId, std::move(tracked));
    Publish(creature);
}

void TC9CorpseTransfer::Publish(Creature* creature)
{
    if (!sToCloud9Sidecar->ClusterModeEnabled() || !creature)
        return;

    std::lock_guard<std::recursive_mutex> lock(_mutex);

    uint64 snapshotId = creature->GetTC9CorpseSnapshotId();
    auto trackedItr = _trackedCorpses.find(snapshotId);
    if (!snapshotId || trackedItr == _trackedCorpses.end())
        return;

    TrackedCorpse& tracked = trackedItr->second;
    uint32 revision = creature->GetTC9CorpseSnapshotRevision() + 1;
    creature->SetTC9CorpseSnapshotRevision(revision);

    uint64 now = uint64(GameTime::GetGameTime().count());
    uint64 liveBlockedUntil = creature->m_groupLootTimer ?
        now + (creature->m_groupLootTimer + IN_MILLISECONDS - 1) / IN_MILLISECONDS : now;
    uint64 latestBlockedUntil = creature->m_groupLootTimer ? liveBlockedUntil : 0;
    if (latestBlockedUntil)
        tracked.expiresAt = std::max(tracked.expiresAt, latestBlockedUntil + TransferredRollLootGrace);

    if (creature->loot.isLooted() || now >= tracked.expiresAt)
    {
        SendRemove(tracked, snapshotId, revision);
        _trackedCorpses.erase(trackedItr);
        creature->ClearTC9CorpseSnapshotId();
        return;
    }

    Player* carrier = ObjectAccessor::FindConnectedPlayer(tracked.carrier);
    if (!carrier || !carrier->GetSession())
        return;

    Group* group = creature->GetLootRecipientGroup();
    if (group && group->GetLootMethod() == MASTER_LOOT && creature->loot.loot_type == LOOT_CORPSE)
    {
        bool hasAllowedLooters = !creature->GetAllowedLooters().empty();
        for (GroupReference* itr = group->GetFirstMember(); itr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || (!member->IsAtLootRewardDistance(creature) &&
                (!hasAllowedLooters || !creature->HasAllowedLooter(member->GetGUID()))))
                continue;
            tracked.viewers.insert(member->GetGUID());
            for (uint32 i = 0; i < creature->loot.items.size(); ++i)
                if (creature->loot.items[i].AllowedForPlayer(member, creature->GetGUID()))
                    tracked.itemEligibility[i].insert(member->GetGUID());
            for (uint32 i = 0; i < creature->loot.quest_items.size(); ++i)
                if (creature->loot.quest_items[i].AllowedForPlayer(member, creature->GetGUID()))
                    tracked.questItemEligibility[i].insert(member->GetGUID());
        }
    }
    if (creature->m_groupLootTimer)
    {
        auto captureRollViewers =
            [group, creature, &tracked](LootItem const& item, uint8 itemSlot, GuidSet& eligibility)
        {
            if (!item.is_blocked || item.is_looted)
                return true;
            std::vector<std::pair<ObjectGuid, uint8>> votes;
            uint8 voteMask = 0;
            if (!group || !group->GetTC9RollState(&creature->loot, itemSlot, votes, voteMask))
                return false;
            for (auto const& vote : votes)
            {
                tracked.viewers.insert(vote.first);
                if (Player* voter = ObjectAccessor::FindConnectedPlayer(vote.first))
                    if (item.AllowedForPlayer(voter, creature->GetGUID()))
                        eligibility.insert(vote.first);
            }
            return true;
        };
        for (uint32 i = 0; i < creature->loot.items.size(); ++i)
            if (!captureRollViewers(creature->loot.items[i], uint8(i), tracked.itemEligibility[i]))
            {
                tracked.transferable = false;
                return;
            }
        for (uint32 i = 0; i < creature->loot.quest_items.size(); ++i)
            if (!captureRollViewers(creature->loot.quest_items[i], uint8(creature->loot.items.size() + i),
                tracked.questItemEligibility[i]))
            {
                tracked.transferable = false;
                return;
            }
    }

    if (tracked.viewers.size() > MaxTransferredPlayers)
    {
        tracked.transferable = false;
        return;
    }

    WorldPacket packet(TC9_SMSG_CORPSE_SNAPSHOT, 1024);
    packet << SnapshotVersion << SnapshotUpsert << snapshotId << revision << tracked.mapId << tracked.instanceId;
    packet << uint64(tracked.carrier.GetRawValue()) << tracked.expiresAt;
    packet << creature->GetEntry() << creature->GetPositionX() << creature->GetPositionY() <<
        creature->GetPositionZ() << creature->GetOrientation();
    packet << creature->GetPhaseMask() << uint64(creature->GetLootRecipientGUID().GetRawValue()) <<
        creature->GetLootRecipientGroupGUID();
    packet << uint8(creature->loot.loot_type) << creature->loot.gold << creature->loot.unlootedCount;
    packet << uint64(creature->loot.roundRobinPlayer.GetRawValue()) << uint64(tracked.carrier.GetRawValue());
    WriteGuidSet(packet, tracked.viewers);

    packet << uint8(creature->loot.items.size());
    for (uint32 i = 0; i < creature->loot.items.size(); ++i)
    {
        GuidSet eligibility = i < tracked.itemEligibility.size() ? tracked.itemEligibility[i] : GuidSet{};
        LootItem const& item = creature->loot.items[i];
        uint64 itemBlockedUntil = item.is_blocked && creature->m_groupLootTimer ? liveBlockedUntil : 0;
        if (!WriteLootItem(packet, item, eligibility, itemBlockedUntil, group, &creature->loot, uint8(i)))
        {
            tracked.transferable = false;
            return;
        }
    }
    packet << uint8(creature->loot.quest_items.size());
    for (uint32 i = 0; i < creature->loot.quest_items.size(); ++i)
    {
        GuidSet eligibility = i < tracked.questItemEligibility.size() ? tracked.questItemEligibility[i] : GuidSet{};
        LootItem const& item = creature->loot.quest_items[i];
        uint64 itemBlockedUntil = item.is_blocked && creature->m_groupLootTimer ? liveBlockedUntil : 0;
        if (!WriteLootItem(packet, item, eligibility, itemBlockedUntil, group, &creature->loot,
            uint8(creature->loot.items.size() + i)))
        {
            tracked.transferable = false;
            return;
        }
    }

    WriteQuestItemMap(packet, creature->loot.GetPlayerQuestItems());
    WriteQuestItemMap(packet, creature->loot.GetPlayerFFAItems());
    WriteQuestItemMap(packet, creature->loot.GetPlayerNonQuestNonFFAConditionalItems());

    if (packet.size() > MaxSnapshotBytes)
    {
        tracked.transferable = false;
        LOG_WARN("network", "TC9 corpse snapshot {} is too large ({} bytes), skipping", snapshotId, packet.size());
        return;
    }
    tracked.transferable = true;
    carrier->GetSession()->SendPacket(&packet);
}

void TC9CorpseTransfer::PublishLootState(Loot const* loot)
{
    if (!sToCloud9Sidecar->ClusterModeEnabled() || !loot || !loot->sourceWorldObjectGUID.IsCreatureOrVehicle())
        return;

    std::lock_guard<std::recursive_mutex> lock(_mutex);
    for (auto const& pair : _trackedCorpses)
    {
        TrackedCorpse const& tracked = pair.second;
        if (tracked.creatureGuid != loot->sourceWorldObjectGUID)
            continue;

        if (Player* carrier = ObjectAccessor::FindConnectedPlayer(tracked.carrier))
            if (Creature* creature = carrier->GetMap()->GetCreature(tracked.creatureGuid))
                if (&creature->loot == loot)
                {
                    Publish(creature);
                    return;
                }
    }
}

void TC9CorpseTransfer::SendRemove(TrackedCorpse const& tracked, uint64 snapshotId, uint32 revision)
{
    Player* carrier = ObjectAccessor::FindConnectedPlayer(tracked.carrier);
    if (!carrier || !carrier->GetSession())
        return;

    WorldPacket packet(TC9_SMSG_CORPSE_SNAPSHOT, 38);
    packet << SnapshotVersion << SnapshotRemove << snapshotId << revision << tracked.mapId << tracked.instanceId;
    packet << uint64(tracked.carrier.GetRawValue()) << tracked.expiresAt;
    carrier->GetSession()->SendPacket(&packet);
}

void TC9CorpseTransfer::Forget(Creature* creature, bool notifyGateway)
{
    if (!sToCloud9Sidecar->ClusterModeEnabled() || !creature)
        return;
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    if (creature->IsTC9CorpseTransferOverflow())
    {
        ObjectGuid carrier = creature->GetTC9CorpseCarrier();
        auto carrierItr = _overflowCorpses.find(carrier);
        if (carrierItr != _overflowCorpses.end())
        {
            uint64 location = LocationKey(creature->GetMapId(), creature->GetInstanceId());
            auto locationItr = carrierItr->second.find(location);
            if (locationItr != carrierItr->second.end() && --locationItr->second == 0)
                carrierItr->second.erase(locationItr);
            if (carrierItr->second.empty())
                _overflowCorpses.erase(carrierItr);
        }
        creature->SetTC9CorpseTransferOverflow(false);
        return;
    }

    uint64 snapshotId = creature->GetTC9CorpseSnapshotId();
    auto itr = _trackedCorpses.find(snapshotId);
    if (!snapshotId || itr == _trackedCorpses.end())
        return;

    if (notifyGateway)
        SendRemove(itr->second, snapshotId, creature->GetTC9CorpseSnapshotRevision() + 1);
    _trackedCorpses.erase(itr);
    creature->ClearTC9CorpseSnapshotId();
}

bool TC9CorpseTransfer::Detach(Player* player, std::vector<uint64> const& snapshotIds, bool allCarriedCorpses)
{
    if (!player)
        return false;

    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto overflowItr = _overflowCorpses.find(player->GetGUID());
    if (overflowItr != _overflowCorpses.end() &&
        overflowItr->second.find(LocationKey(player->GetMapId(), player->GetInstanceId())) != overflowItr->second.end())
        return false;

    std::vector<uint64> ids = snapshotIds;
    if (allCarriedCorpses)
        for (auto const& pair : _trackedCorpses)
            if (pair.second.carrier == player->GetGUID() && pair.second.mapId == player->GetMapId() &&
                pair.second.instanceId == player->GetInstanceId())
                if (std::find(ids.begin(), ids.end(), pair.first) == ids.end())
                    ids.push_back(pair.first);

    for (uint64 snapshotId : ids)
    {
        auto itr = _trackedCorpses.find(snapshotId);
        if (itr == _trackedCorpses.end() || itr->second.carrier != player->GetGUID() ||
            itr->second.mapId != player->GetMapId() || itr->second.instanceId != player->GetInstanceId())
            continue;
        if (Creature* creature = player->GetMap()->GetCreature(itr->second.creatureGuid))
            if (Group* group = creature->GetLootRecipientGroup())
                if (group->HasTC9RollForLoot(&creature->loot))
                    return false;
    }

    // Send one final revision before READY_FOR_REDIRECT. Packets on this socket
    // are ordered, so the gateway caches it before closing the connection.
    for (uint64 snapshotId : ids)
    {
        auto itr = _trackedCorpses.find(snapshotId);
        if (itr == _trackedCorpses.end())
            continue;
        if (Creature* creature = player->GetMap()->GetCreature(itr->second.creatureGuid))
            Publish(creature);
    }

    for (uint64 snapshotId : ids)
    {
        auto itr = _trackedCorpses.find(snapshotId);
        if (itr != _trackedCorpses.end() && itr->second.carrier == player->GetGUID() &&
            itr->second.mapId == player->GetMapId() && itr->second.instanceId == player->GetInstanceId() &&
            !itr->second.transferable)
            return false;
    }

    for (uint64 snapshotId : ids)
    {
        auto itr = _trackedCorpses.find(snapshotId);
        if (itr == _trackedCorpses.end() || itr->second.carrier != player->GetGUID() ||
            itr->second.mapId != player->GetMapId() || itr->second.instanceId != player->GetInstanceId())
            continue;

        Creature* creature = player->GetMap()->GetCreature(itr->second.creatureGuid);
        if (!creature || creature->GetTC9CorpseSnapshotId() != snapshotId)
            continue;

        _trackedCorpses.erase(itr);
        creature->ClearTC9CorpseSnapshotId();
        creature->RemoveCorpse(false);
    }
    return true;
}

bool TC9CorpseTransfer::Restore(Player* player, WorldPacket& packet)
{
    if (!sToCloud9Sidecar->ClusterModeEnabled() || !player || packet.size() > MaxSnapshotBytes)
        return false;

    std::lock_guard<std::recursive_mutex> lock(_mutex);

    QuestItemMap questMaps;
    QuestItemMap ffaMaps;
    QuestItemMap conditionalMaps;
    QuestItemMapsCleanup mapsCleanup{questMaps, ffaMaps, conditionalMaps};
    try
    {
        uint8 version = 0;
        uint8 operation = 0;
        uint64 snapshotId = 0;
        uint32 revision = 0;
        uint32 mapId = 0;
        uint32 instanceId = 0;
        uint64 carrierRaw = 0;
        uint64 expiresAt = 0;
        packet >> version >> operation >> snapshotId >> revision >> mapId >> instanceId >> carrierRaw >> expiresAt;
        ObjectGuid carrier(carrierRaw);
        uint64 now = uint64(GameTime::GetGameTime().count());
        if (version != SnapshotVersion || operation != SnapshotUpsert || !snapshotId ||
            carrier != player->GetGUID() || mapId != player->GetMapId() || instanceId != player->GetInstanceId() ||
            expiresAt <= now || expiresAt - now > MaxTransferredLifetime)
            return false;

        auto existingItr = _trackedCorpses.find(snapshotId);
        if (existingItr != _trackedCorpses.end())
        {
            if (existingItr->second.carrier != carrier || existingItr->second.mapId != mapId ||
                existingItr->second.instanceId != instanceId)
                return false;
            if (Creature* existing = player->GetMap()->GetCreature(existingItr->second.creatureGuid))
                if (existing->GetTC9CorpseSnapshotRevision() >= revision)
                    return true;
        }

        DecodedLoot decoded;
        float x = 0.0f, y = 0.0f, z = 0.0f, orientation = 0.0f;
        uint64 recipientRaw = 0;
        uint8 lootType = 0;
        uint64 roundRobinRaw = 0;
        uint64 lootOwnerRaw = 0;
        packet >> decoded.entry >> x >> y >> z >> orientation >> decoded.phaseMask >> recipientRaw >>
            decoded.recipientGroup;
        packet >> lootType >> decoded.loot.gold >> decoded.loot.unlootedCount >> roundRobinRaw >> lootOwnerRaw;
        decoded.position.Relocate(x, y, z, orientation);
        decoded.recipient = ObjectGuid(recipientRaw);
        decoded.loot.loot_type = LootType(lootType);
        decoded.loot.roundRobinPlayer = ObjectGuid(roundRobinRaw);
        decoded.loot.lootOwnerGUID = ObjectGuid(lootOwnerRaw);
        if (!decoded.entry || !sObjectMgr->GetCreatureTemplate(decoded.entry) || !decoded.position.IsPositionValid() ||
            (decoded.loot.loot_type != LOOT_NONE && decoded.loot.loot_type != LOOT_CORPSE) ||
            !decoded.recipient.IsPlayer() || decoded.loot.lootOwnerGUID != carrier ||
            !ReadGuidSet(packet, decoded.viewers) || decoded.viewers.find(carrier) == decoded.viewers.end())
            return false;

        uint8 itemCount = 0;
        packet >> itemCount;
        if (itemCount > MAX_NR_LOOT_ITEMS)
            return false;
        decoded.loot.items.resize(itemCount);
        decoded.itemRolls.resize(itemCount);
        for (uint32 i = 0; i < itemCount; ++i)
            if (!ReadLootItem(packet, decoded.loot.items[i], i, decoded.viewers, expiresAt, decoded.itemRolls[i]))
                return false;

        uint8 questItemCount = 0;
        packet >> questItemCount;
        if (questItemCount > MAX_NR_QUEST_ITEMS)
            return false;
        decoded.loot.quest_items.resize(questItemCount);
        decoded.questItemRolls.resize(questItemCount);
        for (uint32 i = 0; i < questItemCount; ++i)
            if (!ReadLootItem(packet, decoded.loot.quest_items[i], i, decoded.viewers, expiresAt,
                decoded.questItemRolls[i]))
                return false;

        if (!ReadQuestItemMap(packet, questMaps, decoded.loot.quest_items.size(), decoded.viewers) ||
            !ReadQuestItemMap(packet, ffaMaps, decoded.loot.items.size(), decoded.viewers) ||
            !ReadQuestItemMap(packet, conditionalMaps, decoded.loot.items.size(), decoded.viewers) ||
            packet.rpos() != packet.size())
            return false;
        decoded.loot.RestorePlayerLootMaps(std::move(questMaps), std::move(ffaMaps), std::move(conditionalMaps));

        auto hasDeadline = [](DecodedLoot::RollState const& roll) { return roll.deadline != 0; };
        bool hasActiveRoll = std::any_of(decoded.itemRolls.begin(), decoded.itemRolls.end(), hasDeadline) ||
            std::any_of(decoded.questItemRolls.begin(), decoded.questItemRolls.end(), hasDeadline);
        Group* rollGroup = hasActiveRoll ? sGroupMgr->GetGroupByGUID(decoded.recipientGroup) : nullptr;
        if (hasActiveRoll && !rollGroup)
            return false;

        Creature* corpse = nullptr;
        if (existingItr != _trackedCorpses.end())
            corpse = player->GetMap()->GetCreature(existingItr->second.creatureGuid);

        if (!corpse)
        {
            TempSummon* summon = new TempSummon(nullptr, ObjectGuid::Empty);
            summon->SetTC9CorpseTransferData(snapshotId, revision, carrier, decoded.viewers, true);
            ObjectGuid::LowType corpseGuid = player->GetMap()->GenerateLowGuid<HighGuid::Unit>();
            if (!summon->Create(corpseGuid, player->GetMap(), decoded.phaseMask, decoded.entry, 0,
                    x, y, z, orientation))
            {
                delete summon;
                return false;
            }
            summon->InitStats(uint32((expiresAt - now) * IN_MILLISECONDS));
            summon->SetTempSummonType(TEMPSUMMON_CORPSE_TIMED_DESPAWN);
            // Build the visual loot corpse directly. JustDied would notify
            // ZoneScript/InstanceScript and replay the creature's real death.
            summon->setDeathState(DeathState::Corpse);
            summon->SetHealth(0);
            summon->SetTarget();
            summon->ReplaceAllNpcFlags(UNIT_NPC_FLAG_NONE);
            summon->SetCorpseRemoveTime(uint32(expiresAt - now));
            summon->SetTC9TransferredLootRecipient(decoded.recipient, decoded.recipientGroup);
            if (!player->GetMap()->AddToMap(static_cast<Creature*>(summon)))
            {
                delete summon;
                return false;
            }
            corpse = summon;
        }
        else
        {
            corpse->loot.clear();
            corpse->SetTC9CorpseTransferData(snapshotId, revision, carrier, decoded.viewers, true);
            corpse->SetTC9TransferredLootRecipient(decoded.recipient, decoded.recipientGroup);
        }

        corpse->loot.TakeTransferredState(decoded.loot);
        corpse->loot.sourceWorldObjectGUID = corpse->GetGUID();
        corpse->SetTC9CorpseSnapshotRevision(revision);

        auto restoreRoll = [rollGroup, corpse, now](DecodedLoot::RollState const& roll, uint8 itemSlot)
        {
            if (!roll.deadline)
                return;
            uint64 remainingSeconds = roll.deadline > now ? roll.deadline - now : 0;
            uint32 remainingTime = uint32(std::max<uint64>(1, remainingSeconds * IN_MILLISECONDS));
            rollGroup->RestoreTC9Roll(&corpse->loot, corpse, itemSlot, roll.votes, roll.voteMask, remainingTime);
        };
        for (uint32 i = 0; i < decoded.itemRolls.size(); ++i)
            restoreRoll(decoded.itemRolls[i], uint8(i));
        for (uint32 i = 0; i < decoded.questItemRolls.size(); ++i)
            restoreRoll(decoded.questItemRolls[i], uint8(corpse->loot.items.size() + i));
        corpse->UpdateObjectVisibility(false);

        TrackedCorpse tracked;
        tracked.creatureGuid = corpse->GetGUID();
        tracked.carrier = carrier;
        tracked.viewers = corpse->GetTC9CorpseViewers();
        tracked.mapId = mapId;
        tracked.instanceId = instanceId;
        tracked.expiresAt = expiresAt;
        for (LootItem const& item : corpse->loot.items)
            tracked.itemEligibility.emplace_back(item.GetAllowedLooters().begin(), item.GetAllowedLooters().end());
        for (LootItem const& item : corpse->loot.quest_items)
            tracked.questItemEligibility.emplace_back(item.GetAllowedLooters().begin(), item.GetAllowedLooters().end());
        _trackedCorpses[snapshotId] = std::move(tracked);
        return true;
    }
    catch (ByteBufferException const&)
    {
        return false;
    }
}
