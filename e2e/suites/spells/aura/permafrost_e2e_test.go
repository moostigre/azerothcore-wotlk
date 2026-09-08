//go:build e2e

package aura_test

import (
	"encoding/binary"
	"fmt"
	"math"
	"sync"
	"testing"
	"time"

	"github.com/azerothcore/AzerothGhost/client"
	"github.com/azerothcore/AzerothGhost/e2e/e2eharness"
	"github.com/azerothcore/azerothcore-wotlk/e2e/internal/meta"
	_ "github.com/go-sql-driver/mysql"
)

// Issue: https://github.com/azerothcore/azerothcore-wotlk/issues/16496
// The old RemoveMovementImpairingAuras changed the shared area's snare amount
// to zero. The aura icon alone therefore cannot prove that Permafrost works.
// Exercise real DBC spells, DynObjAura targeting, client-cast Freedom and natural
// expiry. GM setup makes a Frost Sphere cast the area spell once at a safe pad;
// the Anub'arak encounter, falling animation and spike interaction are out of scope.
func TestAC_16496_PermafrostReturnsAfterFreedom(t *testing.T) {
	meta.Begin(t, meta.TestMeta{
		Tags:    []string{"med", "spells", "issue", "multi_bot"},
		Runtime: "med", Issue: 16496, Category: "spells/aura",
	})
	const freedom = uint32(1044)
	bots := e2eharness.NewScenario(t, e2eharness.ScenarioOpts{
		Prefix: "Pfr",
		Bots: []e2eharness.BotSpec{
			{Role: "paladin", Race: e2eharness.RaceHuman, Class: e2eharness.ClassPaladin, Level: 80},
			{Role: "control", Race: e2eharness.RaceHuman, Class: e2eharness.ClassWarrior, Level: 80},
		},
	})
	paladin := e2eharness.ByRole(t, bots, "paladin")
	control := e2eharness.ByRole(t, bots, "control")
	// Share a parent-scoped DB handle: a lazily opened handle would otherwise
	// be cached on the bot but closed after the first difficulty subtest.
	worldDB, err := e2eharness.OpenWorldDB()
	if err != nil {
		e2eharness.HarnessFailf(t, "world DB required for Frost Sphere cleanup: %v", err)
	}
	paladin.WorldDB = worldDB
	t.Cleanup(func() { _ = worldDB.Close() })
	pad := e2eharness.PackagePad(t)
	e2eharness.FormPartyAtPad(t, pad, paladin, control)
	paladin.Learn(t, freedom)
	paladin.CheatPower(t)

	for _, tc := range []struct {
		name  string
		spell uint32
		ratio float32
	}{
		{"10_normal", 66193, 0.70},
		{"25_normal", 67855, 0.70},
		{"10_heroic", 67856, 0.20},
		{"25_heroic", 67857, 0.20},
	} {
		if !t.Run(tc.name, func(t *testing.T) {
			e2eharness.TeleportAllPad(t, bots, pad)
			// Spawn registers cleanup of this fixture and its owned dynamic object.
			// Outside ToC the sphere does not run encounter AI or recast Permafrost.
			sphere, spawnID := paladin.SpawnPersistent(t, 34606, 10*time.Second)
			if spawnID == 0 {
				e2eharness.Preconditionf(t, "Frost Sphere has no cleanup spawn id")
			}
			control.WaitUnitGUID(t, sphere, 10*time.Second)
			for _, bot := range bots {
				bot.CombatReady(t)
			}
			if err := paladin.World.SetTarget(paladin.GUID); err != nil {
				e2eharness.HarnessFailf(t, "select paladin: %v", err)
			}
			paladin.GM(t, ".cooldown 1044")
			paladin.FlushWorld(t)

			// Watch packets before casting so short-lived wrong speed changes are
			// retained, including a control player's slow briefly being removed.
			palSpeed := observePermafrostSpeed(t, paladin)
			controlSpeed := observePermafrostSpeed(t, control)
			if err := paladin.World.SetTarget(sphere); err != nil {
				e2eharness.HarnessFailf(t, "select Frost Sphere: %v", err)
			}
			// Do not append "triggered": the GM command's FULL_DEBUG_MASK
			// includes IGNORE_EFFECTS, unlike the encounter's triggered=true cast.
			paladin.GM(t, fmt.Sprintf(".cast self %d", tc.spell))
			wantSlow := float32(7) * tc.ratio
			if !waitPermafrostCondition(5*time.Second, func() bool {
				return paladin.HasAura(tc.spell) && control.HasAura(tc.spell) &&
					palSpeed.is(wantSlow) && controlSpeed.is(wantSlow)
			}) {
				e2eharness.Preconditionf(t, "spell %d must slow both players to %.2f: paladin=%v control=%v auras=%v/%v",
					tc.spell, wantSlow, palSpeed.values(), controlSpeed.values(), paladin.World.SelfAuras(), control.World.SelfAuras())
			}
			controlStart := len(controlSpeed.values())
			t.Logf("Permafrost %d from sphere=0x%X: both speeds=%.2f", tc.spell, sphere, wantSlow)

			paladin.CastMust(t, freedom, paladin.GUID, 5*time.Second)
			if !waitPermafrostCondition(2*time.Second, func() bool {
				return paladin.HasAura(freedom) && !paladin.HasAura(tc.spell) && palSpeed.is(7)
			}) {
				e2eharness.ConfirmedBugf(t, 16496, "Freedom must remove Permafrost %d and restore speed: speeds=%v auras=%v",
					tc.spell, palSpeed.values(), paladin.World.SelfAuras())
			}
			t.Logf("Freedom active: paladin=7.00, control must remain %.2f", wantSlow)
			// Freedom lasts 6s without talents. Wait for natural expiry; continuously
			// check the control player and retain every speed packet, without recasts.
			expired := waitPermafrostCondition(10*time.Second, func() bool {
				if !control.HasAura(tc.spell) || !controlSpeed.is(wantSlow) {
					e2eharness.ConfirmedBugf(t, 16496, "Freedom on one player changed control's Permafrost %d: speeds=%v auras=%v",
						tc.spell, controlSpeed.values(), control.World.SelfAuras())
				}
				return !paladin.HasAura(freedom)
			})
			if !expired {
				e2eharness.Assertf(t, "Hand of Freedom did not expire naturally")
			}
			expiredAt := time.Now()
			// Core refreshes DynObjAura targets every 500ms. Allow 2s for map and
			// network scheduling; the player stays inside the same single cast.
			if !waitPermafrostCondition(2*time.Second, func() bool {
				return paladin.HasAura(tc.spell) && palSpeed.is(wantSlow)
			}) {
				e2eharness.ConfirmedBugf(t, 16496, "Permafrost %d did not restore %.2f after Freedom: speeds=%v auras=%v",
					tc.spell, wantSlow, palSpeed.values(), paladin.World.SelfAuras())
			}
			for _, speed := range controlSpeed.values()[controlStart:] {
				if !permafrostSpeedEqual(speed, wantSlow) {
					e2eharness.ConfirmedBugf(t, 16496, "control received speed %.2f instead of %.2f on the same Permafrost %d",
						speed, wantSlow, tc.spell)
				}
			}
			if !control.HasAura(tc.spell) || !controlSpeed.is(wantSlow) {
				e2eharness.ConfirmedBugf(t, 16496, "control lost Permafrost %d after Freedom expired", tc.spell)
			}
			t.Logf("PASS AC#16496 spell=%d: paladin %.2f -> 7.00 -> %.2f; control %.2f; restored %s after observed expiry",
				tc.spell, wantSlow, wantSlow, wantSlow, time.Since(expiredAt).Round(time.Millisecond))
			paladin.AssertWorldAlive(t)
		}) {
			return // Do not repeat fixture failures for the remaining difficulties.
		}
	}
}

// The v1.0.8 client's MoveSpeed getter is not synchronized with its read loop.
// Record SMSG_FORCE_RUN_SPEED_CHANGE using the supported packet hook instead.
type permafrostSpeedObserver struct {
	mu     sync.Mutex
	speeds []float32
}

func observePermafrostSpeed(t *testing.T, bot *e2eharness.ScenarioBot) *permafrostSpeedObserver {
	t.Helper()
	o := &permafrostSpeedObserver{}
	cancel := bot.World.AddPacketHook(func(opcode uint16, data []byte) {
		if opcode != client.SmsgForceRunSpeedChange {
			return
		}
		// packed GUID, uint32 counter, uint8 unknown, float32 run speed.
		if len(data) == 0 {
			return
		}
		pos := 1
		var guid uint64
		for i := 0; i < 8; i++ {
			if data[0]&(1<<i) != 0 {
				if pos >= len(data) {
					return
				}
				guid |= uint64(data[pos]) << (8 * i)
				pos++
			}
		}
		if guid != bot.GUID || len(data) != pos+9 {
			return
		}
		speed := math.Float32frombits(binary.LittleEndian.Uint32(data[pos+5:]))
		o.mu.Lock()
		o.speeds = append(o.speeds, speed)
		o.mu.Unlock()
	})
	t.Cleanup(cancel)
	return o
}

func (o *permafrostSpeedObserver) values() []float32 {
	o.mu.Lock()
	defer o.mu.Unlock()
	return append([]float32(nil), o.speeds...)
}

func (o *permafrostSpeedObserver) is(want float32) bool {
	o.mu.Lock()
	defer o.mu.Unlock()
	return len(o.speeds) > 0 && permafrostSpeedEqual(o.speeds[len(o.speeds)-1], want)
}

func permafrostSpeedEqual(got, want float32) bool {
	return math.Abs(float64(got-want)) < 0.01
}

func waitPermafrostCondition(timeout time.Duration, condition func() bool) bool {
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
