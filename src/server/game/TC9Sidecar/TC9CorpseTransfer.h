/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#ifndef _TC9_CORPSE_TRANSFER_H
#define _TC9_CORPSE_TRANSFER_H

#include "Common.h"
#include "ObjectGuid.h"

#include <mutex>
#include <unordered_map>
#include <vector>

class Creature;
class Player;
class WorldPacket;
struct Loot;
struct LootItem;

class TC9CorpseTransfer
{
public:
    static TC9CorpseTransfer* instance();

    void TrackAndPublish(Creature* creature);
    void Publish(Creature* creature);
    void PublishLootState(Loot const* loot);
    void Forget(Creature* creature, bool notifyGateway);
    bool Detach(Player* player, std::vector<uint64> const& snapshotIds, bool allCarriedCorpses);
    bool Restore(Player* player, WorldPacket& packet);

private:
    struct TrackedCorpse
    {
        ObjectGuid creatureGuid;
        ObjectGuid carrier;
        GuidSet viewers;
        std::vector<GuidSet> itemEligibility;
        std::vector<GuidSet> questItemEligibility;
        uint32 mapId{0};
        uint32 instanceId{0};
        uint64 expiresAt{0};
        bool transferable{true};
    };

    uint64 GenerateSnapshotId();
    static uint64 LocationKey(uint32 mapId, uint32 instanceId);
    void SendRemove(TrackedCorpse const& tracked, uint64 snapshotId, uint32 revision);

    std::recursive_mutex _mutex;
    std::unordered_map<uint64, TrackedCorpse> _trackedCorpses;
    std::unordered_map<ObjectGuid, std::unordered_map<uint64, std::size_t>> _overflowCorpses;
};

#define sTC9CorpseTransfer TC9CorpseTransfer::instance()

#endif
