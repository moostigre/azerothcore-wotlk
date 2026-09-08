//go:build e2e

package trial_of_the_crusader_test

import (
	"testing"
	"time"

	_ "github.com/go-sql-driver/mysql"

	"github.com/azerothcore/AzerothGhost/e2e/e2eharness"
	"github.com/azerothcore/azerothcore-wotlk/e2e/internal/meta"
)

const (
	trialOfTheCrusaderMap  = uint32(649)
	npcAnubarak            = uint32(34564)
	npcPursuingSpike       = uint32(34660)
	spellPursuedByAnubarak = uint32(67574)
)

func waitForPursuedPlayer(t *testing.T, bots []*e2eharness.ScenarioBot, spike uint64) *e2eharness.ScenarioBot {
	t.Helper()
	ticker := time.NewTicker(50 * time.Millisecond)
	defer ticker.Stop()
	timer := time.NewTimer(5 * time.Second)
	defer timer.Stop()

	for {
		target := bots[0].UnitTarget(spike)
		for _, bot := range bots {
			if target == bot.GUID && bot.HasAura(spellPursuedByAnubarak) {
				return bot
			}
		}
		select {
		case <-ticker.C:
		case <-timer.C:
			e2eharness.ConfirmedBugf(t, 23540, "spike 0x%X did not establish a marked player target", spike)
			return nil
		}
	}
}

// Issue: https://github.com/azerothcore/azerothcore-wotlk/issues/23540
// Reaching a living player must not reroll the pursuit target. Two stationary
// players expose random retargeting; god mode permits surviving Impale without
// applying an immunity or removing the pursuit mark.
func TestAC_23540_PursuingSpikeKeepsLivingTargetOnArrival(t *testing.T) {
	meta.Begin(t, meta.TestMeta{
		Tags:     []string{"long", "instances", "issue", "multi_bot", "serial"},
		Runtime:  "long",
		Issue:    23540,
		Category: "instances/northrend/trial_of_the_crusader",
	})

	bots := e2eharness.NewScenario(t, e2eharness.ScenarioOpts{
		Prefix: "AnubSp",
		Count:  2,
		Race:   e2eharness.RaceHuman,
		Class:  e2eharness.ClassWarrior,
		Level:  80,
	})
	leader := bots[0]
	pad := e2eharness.PackagePad(t)
	e2eharness.FormPartyAtPad(t, pad, leader, bots[1])
	defer e2eharness.DisbandParty(t, bots...)

	// Use a fresh group instance and a boss fixture to skip the preceding raid
	// encounters. The boss itself must summon the spike during its real submerge;
	// spawning a spike directly would bypass that spell and combat setup.
	e2eharness.TeleportAll(t, bots, 738, 130, 142.2, trialOfTheCrusaderMap)
	boss := leader.Spawn(t, npcAnubarak, 15*time.Second)
	bots[1].Teleport(t, 758, 130, 142.2, trialOfTheCrusaderMap)
	for _, bot := range bots {
		bot.CombatReady(t)
	}
	leader.Engage(t, boss, 15*time.Second)
	if err := leader.World.AttackStop(); err != nil {
		e2eharness.HarnessFailf(t, "stop autoattack after pulling Anub'arak: %v", err)
	}

	// Submerge starts at 80s and summons the spike 2.5s later. Do not kill any
	// Frost Spheres: airborne spheres remain above the spike's collision radius.
	spike := leader.WaitUnit(t, npcPursuingSpike, 100*time.Second)
	pursued := waitForPursuedPlayer(t, bots, spike)
	t.Logf("spike=0x%X pursued=0x%X boss=0x%X", spike, pursued.GUID, boss)

	ticker := time.NewTicker(50 * time.Millisecond)
	defer ticker.Stop()
	timer := time.NewTimer(25 * time.Second)
	defer timer.Stop()
	var reachedAt time.Time

	for {
		if hp, _ := pursued.UnitHP(pursued.GUID); hp == 0 {
			e2eharness.Preconditionf(t, "pursued player died before the target-retention oracle completed")
		}
		if leader.World.GetObject(spike) == nil {
			e2eharness.Assertf(t, "spike 0x%X disappeared before the target-retention oracle completed", spike)
		}
		if target := leader.UnitTarget(spike); target != pursued.GUID {
			e2eharness.ConfirmedBugf(t, 23540, "spike 0x%X switched from living player 0x%X to 0x%X without Permafrost", spike, pursued.GUID, target)
		}
		if !pursued.HasAura(spellPursuedByAnubarak) {
			e2eharness.ConfirmedBugf(t, 23540, "living pursued player 0x%X lost the mark without Permafrost", pursued.GUID)
		}

		obj := leader.World.GetObject(spike)
		if obj != nil && obj.HasKnownPosition() {
			sx, sy, sz := obj.InterpolatedPosition()
			px, py, pz, _ := pursued.Pos()
			if e2eharness.Distance3D(sx, sy, sz, px, py, pz) <= 4 {
				if reachedAt.IsZero() {
					reachedAt = time.Now()
					t.Logf("spike reached its living target; observing another 5s")
				}
				if time.Since(reachedAt) >= 5*time.Second {
					leader.AssertWorldAlive(t)
					t.Logf("PASS AC#23540 spike retained player 0x%X after reaching them", pursued.GUID)
					return
				}
			} else {
				reachedAt = time.Time{}
			}
		}

		select {
		case <-ticker.C:
		case <-timer.C:
			e2eharness.Assertf(t, "spike 0x%X did not reach and remain near its stationary target within 25s", spike)
		}
	}
}
