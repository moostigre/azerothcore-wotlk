//go:build e2e

package trial_of_the_crusader_test

import (
	"encoding/binary"
	"fmt"
	"sync"
	"testing"
	"time"

	"github.com/azerothcore/AzerothGhost/e2e/e2eharness"
	"github.com/azerothcore/azerothcore-wotlk/e2e/internal/meta"
	_ "github.com/go-sql-driver/mysql"
)

// 3.3.5a opcodes from src/server/game/Server/Protocol/Opcodes.h. The pinned
// harness exposes SendPacketRaw but has no typed raid-difficulty/gossip API.
const (
	touchRaidConvert    = uint16(0x28E)
	touchRaidDifficulty = uint16(0x4EB)
	touchGossipHello    = uint16(0x17B)
	touchPeriodicLog    = uint16(0x24E)
	touchMap            = uint32(649)
)

// Issue: https://github.com/azerothcore/azerothcore-wotlk/issues/19834
// The Touch handler is map-specific: testing at an outdoor pad would silently
// skip every tick. Use a fresh heroic ToC instance without starting the encounter.
// A passive training dummy casts the real Touch once; three clients observe its
// raid-wide ticks. Real essence gossip drives color changes; a client-cast shield
// proves that calculated absorption is reflected in both damage and combat logs.
func TestAC_19834_TwinTouchRespectsEssenceAndAbsorbs(t *testing.T) {
	meta.Begin(t, meta.TestMeta{
		Tags:    []string{"med", "instances", "spells", "issue", "multi_bot"},
		Runtime: "med", Issue: 19834, Category: "instances/northrend/trial_of_the_crusader",
	})
	for _, difficulty := range []struct {
		name                                         string
		id                                           uint32
		light, dark, lightTouch, darkTouch, powering uint32
	}{
		{"10_heroic", 2, 67223, 67177, 67297, 67282, 67603},
		{"25_heroic", 3, 67224, 67178, 67298, 67283, 67604},
	} {
		t.Run(difficulty.name, func(t *testing.T) {
			// Separate parties isolate instance difficulty, binds, and difficulty
			// change cooldowns; reuse these clients for both Touch colors.
			bots := e2eharness.NewScenario(t, e2eharness.ScenarioOpts{
				Prefix: "Twt",
				Bots: []e2eharness.BotSpec{
					{Role: "holder", Race: e2eharness.RaceHuman, Class: e2eharness.ClassWarrior, Level: 80},
					{Role: "matching", Race: e2eharness.RaceHuman, Class: e2eharness.ClassWarrior, Level: 80},
					{Role: "shielded", Race: e2eharness.RaceHuman, Class: e2eharness.ClassPriest, Level: 80},
				},
			})
			holder := e2eharness.ByRole(t, bots, "holder")
			matching := e2eharness.ByRole(t, bots, "matching")
			shielded := e2eharness.ByRole(t, bots, "shielded")
			e2eharness.FormPartyAtPad(t, e2eharness.PackagePad(t), holder, matching, shielded)
			// Gate instance entry on world visibility, not just party membership:
			// a gateway may still be co-locating members after accepting invites.
			for _, bot := range bots[1:] {
				holder.WaitUnitGUID(t, bot.GUID, 15*time.Second)
				bot.WaitUnitGUID(t, holder.GUID, 15*time.Second)
			}
			setTouchRaidDifficulty(t, holder, difficulty.id)
			for _, bot := range bots {
				bot.Teleport(t, 563.26, 139.60, 394.08, touchMap)
				if _, _, _, mapID := bot.Pos(); mapID != touchMap {
					e2eharness.Preconditionf(t, "ToC teleport failed: map=%d", mapID)
				}
			}
			for _, bot := range bots[1:] {
				holder.WaitUnitGUID(t, bot.GUID, 10*time.Second)
				bot.WaitUnitGUID(t, holder.GUID, 10*time.Second)
			}

			caster, casterSpawn := holder.SpawnPersistent(t, 31146, 10*time.Second)
			lightPortal, lightSpawn := holder.SpawnPersistent(t, 34568, 10*time.Second)
			darkPortal, darkSpawn := holder.SpawnPersistent(t, 34567, 10*time.Second)
			if casterSpawn == 0 || lightSpawn == 0 || darkSpawn == 0 {
				e2eharness.Preconditionf(t, "world DB spawn IDs are required for fixture cleanup")
			}
			for _, bot := range bots {
				bot.WaitUnitGUID(t, lightPortal, 10*time.Second)
				bot.WaitUnitGUID(t, darkPortal, 10*time.Second)
				// God mode would hide precisely the damage bug under test.
				bot.GM(t, ".cheat god off")
				e2eharness.CombatReady(t, bot.World, e2eharness.CombatReadyOpts{})
			}
			shielded.Learn(t, 17) // Power Word: Shield, rank 1 (partial absorption).
			shielded.CheatPower(t)

			for _, color := range []struct {
				name                      string
				spell, matching, opposite uint32
				portal, otherPortal       uint64
			}{
				{"light", difficulty.lightTouch, difficulty.light, difficulty.dark, lightPortal, darkPortal},
				{"dark", difficulty.darkTouch, difficulty.dark, difficulty.light, darkPortal, lightPortal},
			} {
				t.Run(color.name, func(t *testing.T) {
					for _, bot := range bots {
						if err := bot.World.SetTarget(bot.GUID); err != nil {
							e2eharness.HarnessFailf(t, "select self: %v", err)
						}
						bot.GM(t, ".modify hp 100000")
						bot.GM(t, fmt.Sprintf(".unaura %d", difficulty.powering))
						bot.FlushWorld(t)
					}
					chooseTouchEssence(t, holder, color.otherPortal, color.opposite, color.matching)
					chooseTouchEssence(t, matching, color.portal, color.matching, color.opposite)
					chooseTouchEssence(t, shielded, color.otherPortal, color.opposite, color.matching)
					shielded.GM(t, ".unaura 6788") // Remove prior case's Weakened Soul during setup.
					shielded.GM(t, ".cooldown 17")
					shielded.CastMust(t, 17, shielded.GUID, 5*time.Second)
					if !waitTouchCondition(3*time.Second, func() bool { return shielded.HasAura(17) }) {
						e2eharness.Preconditionf(t, "shield did not apply before Touch")
					}
					if !waitTouchCondition(3*time.Second, func() bool {
						for _, bot := range bots {
							if hp, _ := bot.UnitHP(bot.GUID); hp < 90000 {
								return false
							}
						}
						return true
					}) {
						e2eharness.Preconditionf(t, "test players lack enough health for Touch")
					}

					log := observeTouchTicks(t, holder, color.spell, caster)
					// Cleanup the debuff even when an assertion aborts this case.
					t.Cleanup(func() {
						_ = holder.World.SetTarget(holder.GUID)
						holder.GM(t, fmt.Sprintf(".unaura %d", color.spell))
						holder.FlushWorld(t)
					})
					if err := holder.World.SetTarget(caster); err != nil {
						e2eharness.HarnessFailf(t, "select caster: %v", err)
					}
					// No "triggered": GM FULL_DEBUG_MASK includes IGNORE_EFFECTS.
					holder.GM(t, fmt.Sprintf(".cast back %d", color.spell))
					if !waitTouchCondition(5*time.Second, func() bool { return holder.HasAura(color.spell) }) {
						e2eharness.Preconditionf(t, "Touch %d did not apply to its opposite-color holder", color.spell)
					}
					if !waitTouchCondition(7*time.Second, func() bool {
						return len(log.forTarget(holder.GUID)) >= 2 && len(log.forTarget(shielded.GUID)) >= 2
					}) {
						e2eharness.Assertf(t, "Touch %d did not tick on both opposite-color players", color.spell)
					}
					if ticks := log.forTarget(matching.GUID); len(ticks) != 0 || matching.HasAura(difficulty.powering) {
						e2eharness.ConfirmedBugf(t, 19834, "same-color player was processed by Touch %d: ticks=%v powering=%v",
							color.spell, ticks, matching.HasAura(difficulty.powering))
					}
					if hp, _ := matching.UnitHP(matching.GUID); hp != 100000 {
						e2eharness.ConfirmedBugf(t, 19834, "same-color player lost health: %d", hp)
					}
					absorbed := false
					for _, tick := range log.forTarget(shielded.GUID) {
						total := tick.damage + tick.absorb + tick.resist
						min, max := uint32(2925), uint32(3075)
						if difficulty.id == 3 {
							min, max = 5850, 6150
						}
						if total < min || total > max {
							e2eharness.ConfirmedBugf(t, 19834, "Touch mitigation/log mismatch: %+v raw range=%d..%d",
								tick, min, max)
						}
						absorbed = absorbed || tick.absorb > 0
					}
					if !absorbed {
						e2eharness.ConfirmedBugf(t, 19834, "Touch %d ignored Power Word: Shield absorption", color.spell)
					}
					if hp, _ := holder.UnitHP(holder.GUID); hp >= 100000 {
						e2eharness.Assertf(t, "opposite-color holder did not lose health (god mode?)")
					}

					// Re-evaluate each tick, not only when the debuff was first cast.
					chooseTouchEssence(t, matching, color.otherPortal, color.opposite, color.matching)
					if !waitTouchCondition(4*time.Second, func() bool { return len(log.forTarget(matching.GUID)) > 0 }) {
						e2eharness.ConfirmedBugf(t, 19834, "switching to opposite Essence did not expose player to Touch")
					}
					chooseTouchEssence(t, matching, color.portal, color.matching, color.opposite)
					protectedAfter := time.Now()
					holderTicks := len(log.forTarget(holder.GUID))
					if !waitTouchCondition(6*time.Second, func() bool {
						return len(log.forTarget(holder.GUID)) >= holderTicks+2
					}) {
						e2eharness.Assertf(t, "Touch stopped before color-switch protection could be checked")
					}
					for _, tick := range log.forTarget(matching.GUID) {
						if tick.at.After(protectedAfter) {
							e2eharness.ConfirmedBugf(t, 19834, "Touch still processed player after switching back: %+v", tick)
						}
					}
					chooseTouchEssence(t, holder, color.portal, color.matching, color.opposite)
					if !waitTouchCondition(3*time.Second, func() bool { return !holder.HasAura(color.spell) }) {
						e2eharness.ConfirmedBugf(t, 19834, "holder's color switch did not remove Touch %d", color.spell)
					}
					t.Logf("PASS AC#19834 Touch=%d: matching excluded, opposite damaged, shield absorbed, color switches work",
						color.spell)
				})
			}
		})
	}
}

func chooseTouchEssence(t *testing.T, bot *e2eharness.ScenarioBot, portal uint64, want, unwanted uint32) {
	t.Helper()
	data := make([]byte, 8)
	binary.LittleEndian.PutUint64(data, portal)
	if err := bot.World.SendPacketRaw(touchGossipHello, data); err != nil {
		e2eharness.HarnessFailf(t, "essence gossip: %v", err)
	}
	if !waitTouchCondition(5*time.Second, func() bool { return bot.HasAura(want) && !bot.HasAura(unwanted) }) {
		e2eharness.Preconditionf(t, "essence gossip did not yield aura %d (remove %d): %v",
			want, unwanted, bot.World.SelfAuras())
	}
}

func setTouchRaidDifficulty(t *testing.T, leader *e2eharness.ScenarioBot, difficulty uint32) {
	t.Helper()
	if err := leader.World.SendPacketRaw(touchRaidConvert, nil); err != nil {
		e2eharness.HarnessFailf(t, "convert to raid: %v", err)
	}
	if !waitTouchCondition(5*time.Second, func() bool { return leader.World.GroupState().GroupType&2 != 0 }) {
		e2eharness.Preconditionf(t, "party did not convert to raid")
	}
	ack := make(chan uint32, 1)
	cancel := leader.World.AddPacketHook(func(opcode uint16, data []byte) {
		if opcode == touchRaidDifficulty && len(data) >= 4 {
			select {
			case ack <- binary.LittleEndian.Uint32(data):
			default:
			}
		}
	})
	defer cancel()
	data := make([]byte, 4)
	binary.LittleEndian.PutUint32(data, difficulty)
	if err := leader.World.SendPacketRaw(touchRaidDifficulty, data); err != nil {
		e2eharness.HarnessFailf(t, "set heroic raid difficulty: %v", err)
	}
	select {
	case got := <-ack:
		if got != difficulty {
			e2eharness.Preconditionf(t, "raid difficulty=%d, want=%d", got, difficulty)
		}
	case <-time.After(5 * time.Second):
		e2eharness.Preconditionf(t, "no heroic raid difficulty acknowledgement")
	}
}

type touchTick struct {
	target                 uint64
	damage, absorb, resist uint32
	at                     time.Time
}

type touchTickLog struct {
	mu    sync.Mutex
	ticks []touchTick
}

func observeTouchTicks(t *testing.T, bot *e2eharness.ScenarioBot, spell uint32, caster uint64) *touchTickLog {
	t.Helper()
	log := &touchTickLog{}
	t.Cleanup(bot.World.AddPacketHook(func(opcode uint16, data []byte) {
		if opcode != touchPeriodicLog {
			return
		}
		target, rest, ok := touchPackedGUID(data)
		if !ok {
			return
		}
		guid, rest, ok := touchPackedGUID(rest)
		// spell, count, aura type, damage, overkill, school, absorb, resist, crit.
		if !ok || guid != caster || len(rest) != 33 || binary.LittleEndian.Uint32(rest) != spell ||
			binary.LittleEndian.Uint32(rest[4:]) != 1 || binary.LittleEndian.Uint32(rest[8:]) != 3 {
			return
		}
		log.mu.Lock()
		log.ticks = append(log.ticks, touchTick{target: target, damage: binary.LittleEndian.Uint32(rest[12:]),
			absorb: binary.LittleEndian.Uint32(rest[24:]), resist: binary.LittleEndian.Uint32(rest[28:]), at: time.Now()})
		log.mu.Unlock()
	}))
	return log
}

func (log *touchTickLog) forTarget(guid uint64) []touchTick {
	log.mu.Lock()
	defer log.mu.Unlock()
	var ticks []touchTick
	for _, tick := range log.ticks {
		if tick.target == guid {
			ticks = append(ticks, tick)
		}
	}
	return ticks
}

func touchPackedGUID(data []byte) (uint64, []byte, bool) {
	if len(data) == 0 {
		return 0, nil, false
	}
	mask, pos := data[0], 1
	var guid uint64
	for i := 0; i < 8; i++ {
		if mask&(1<<i) != 0 {
			if pos >= len(data) {
				return 0, nil, false
			}
			guid |= uint64(data[pos]) << (8 * i)
			pos++
		}
	}
	return guid, data[pos:], true
}

func waitTouchCondition(timeout time.Duration, condition func() bool) bool {
	ticker := time.NewTicker(25 * time.Millisecond)
	defer ticker.Stop()
	timer := time.NewTimer(timeout)
	defer timer.Stop()
	for {
		if condition() {
			return true
		}
		select {
		case <-ticker.C:
		case <-timer.C:
			return condition()
		}
	}
}
