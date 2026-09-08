//go:build e2e

package strand_test

import (
	"encoding/binary"
	"os"
	"sync"
	"testing"
	"time"

	"github.com/azerothcore/AzerothGhost/e2e/e2eharness"
	"github.com/azerothcore/azerothcore-wotlk/e2e/internal/meta"
	_ "github.com/go-sql-driver/mysql"
)

const (
	sotaMap               = 607
	sotaType              = 9
	goFlagsField          = 0x09 // GAMEOBJECT_FLAGS, 3.3.5a
	goNotSelectable       = 0x10
	opening               = 21651
	gateDamage            = 66672 // Huge Seaforium Blast: setup through real GO damage/event handling
	wsTimer               = 3564
	wsMinutes             = 3559
	wsAllianceAttacks     = 4352
	wsHordeAttacks        = 4353
	cmsgBattlefieldPort   = 0x2D5
	cmsgLeaveBattlefield  = 0x2E1
	cmsgBattlemasterJoin  = 0x2EE
	smsgBattlefieldStatus = 0x2D4
	smsgInitWorldStates   = 0x2C2
	smsgUpdateWorldState  = 0x2C3
)

type graveyard struct {
	name                string
	alliance, horde     uint32
	allianceWS, hordeWS uint32
	x, y, z             float32
}

var graveyards = []graveyard{
	{"right", 191306, 191305, 3636, 3632, 1338.8593, -153.3273, 30.8951},
	{"left", 191308, 191307, 3635, 3633, 1309.1920, 9.4162, 30.8934},
	{"central", 191310, 191309, 3637, 3634, 1215.1080, -65.7158, 70.0843},
}

type gate struct {
	name      string
	entry, ws uint32
	x, y, z   float32
}

var gates = []gate{
	{"green", 190722, 3623, 1411.57, 108.163, 28.692},
	{"blue", 190724, 3620, 1431.3413, -219.437, 30.893},
	{"red", 190726, 3617, 1227.667, -212.555, 55.372},
	{"purple", 190723, 3614, 1214.681, 81.21, 53.413},
	{"yellow", 190727, 3638, 1055.452, -108.1, 82.134},
	{"ancient", 192549, 3849, 878.555, -108.2, 117.845},
}

// Issue: https://github.com/azerothcore/azerothcore-wotlk/issues/23129
// Requires an exclusive disposable realm with Debug.Battleground = 1. Never toggles
// global testing mode or edits battleground templates. See README.md before opting in.
func TestAC_23129_CapturedGraveyardsStayNonInteractable(t *testing.T) {
	meta.Begin(t, meta.TestMeta{Issue: 23129, Category: "battlegrounds/strand", Runtime: "long",
		Tags: []string{"battleground", "multi_bot", "exclusive_realm", "long"}})
	if os.Getenv("E2E_SOTA_TEST_REALM") != "1" {
		t.Skip("requires explicit E2E_SOTA_TEST_REALM=1 and exclusive realm with Debug.Battleground=1; see README.md")
	}

	bots := e2eharness.NewScenario(t, e2eharness.ScenarioOpts{Prefix: "Sotagy", Bots: []e2eharness.BotSpec{
		{Role: "alliance", Race: e2eharness.RaceHuman, Class: e2eharness.ClassWarrior, Level: 80},
		{Role: "horde", Race: e2eharness.RaceOrc, Class: e2eharness.ClassWarrior, Level: 80},
	}})
	states := []*sotaState{newSotaState(t, bots[0]), newSotaState(t, bots[1])}
	for _, bot := range bots {
		bot.TeleportPad(t, e2eharness.PackagePad(t))
		bot.Learn(t, opening)
		bot.Learn(t, gateDamage)
		bot.GM(t, ".gm off")
		bot.GM(t, ".cheat god off")
		bot.FlushWorld(t)
		// Registered after login cleanup so these packets are sent while sessions are open.
		t.Cleanup(func() {
			if err := bot.World.SendPacketRaw(cmsgBattlefieldPort, portPayload(false)); err != nil {
				t.Logf("queue cleanup %s: %v", bot.Name, err)
			}
			if err := bot.World.SendPacketRaw(cmsgLeaveBattlefield, nil); err != nil {
				t.Logf("battleground cleanup %s: %v", bot.Name, err)
			}
		})
	}

	// In 1v0 mode, queue sequentially into the same client instance rather than
	// letting both factions independently create an empty battleground.
	instance := joinSota(t, bots[0], states[0], 0)
	joinSota(t, bots[1], states[1], instance)
	if !eventually(150*time.Second, func() bool { return states[0].is(wsTimer, 1) && states[1].is(wsTimer, 1) }) {
		e2eharness.Preconditionf(t, "SotA did not start; requires Debug.Battleground=1 and no interfering players")
	}
	initialAlliance := states[0].is(wsAllianceAttacks, 1)
	for round := 1; round <= 2; round++ {
		alliance := initialAlliance
		if round == 2 {
			alliance = !alliance
		}
		attacker, defender := bots[0], bots[1]
		if !alliance {
			attacker, defender = defender, attacker
		}
		t.Logf("round %d: attacker=%s, defender=%s", round, attacker.Name, defender.Name)
		// Starting again with all three flags locked also checks ResetObjs/role swap.
		for _, gy := range graveyards {
			assertOwnership(t, states, gy, !alliance)
			checkBanner(t, bots, gy, !alliance, true)
		}

		destroyGate(t, attacker, states, gates[0])
		for _, gy := range graveyards[:2] {
			checkBanner(t, bots, gy, !alliance, false)
			capture(t, attacker, bots, states, gy, alliance)
		}
		checkBanner(t, bots, graveyards[2], !alliance, true)
		// This is the important second route: another destroyed gate must NOT
		// remove NOT_SELECTABLE from the two newly created captured banners.
		destroyGate(t, attacker, states, gates[1])
		for _, gy := range graveyards[:2] {
			checkBanner(t, bots, gy, alliance, true)
		}
		destroyGate(t, attacker, states, gates[2])
		checkBanner(t, bots, graveyards[2], !alliance, false)
		capture(t, attacker, bots, states, graveyards[2], alliance)
		for _, g := range gates[3:] {
			destroyGate(t, attacker, states, g)
			for _, gy := range graveyards {
				checkBanner(t, bots, gy, alliance, true)
				assertOwnership(t, states, gy, alliance)
			}
		}

		// The relic shares gate-check fallthrough with the central flag. Capturing
		// that flag must not disable the relic or prevent the second round.
		attacker.Teleport(t, 837.0653, -107.5367, 127.0248, sotaMap)
		relic := waitGO(t, attacker, 192834)
		assertSelectable(t, attacker, relic, false)
		if round == 1 {
			// Round two inherits round one's completion time. Leave at least three
			// minutes for its assertions instead of creating an artificially tiny limit.
			if !eventually(3*time.Minute, func() bool {
				v, ok := states[0].get(wsMinutes)
				return ok && v > 0 && v <= 7
			}) {
				e2eharness.Preconditionf(t, "round timer did not progress")
			}
			attacker.GameObjectUse(t, relic)
			if !eventually(15*time.Second, func() bool {
				return states[0].is(wsAllianceAttacks, boolValue(!alliance)) &&
					states[1].is(wsHordeAttacks, boolValue(alliance)) && states[0].is(wsTimer, 0)
			}) {
				e2eharness.ConfirmedBugf(t, 23129, "relic did not advance round after central graveyard capture")
			}
			if !eventually(90*time.Second, func() bool { return states[0].is(wsTimer, 1) && states[1].is(wsTimer, 1) }) {
				e2eharness.Assertf(t, "second round did not start")
			}
		}
	}
	t.Log("PASS: both attacking factions, three captures, later gate unlocks, round reset and relic eligibility")
}

func capture(t *testing.T, attacker *e2eharness.ScenarioBot, bots []*e2eharness.ScenarioBot, states []*sotaState, gy graveyard, alliance bool) {
	t.Helper()
	attacker.Teleport(t, gy.x, gy.y, gy.z, sotaMap)
	entry := gy.alliance
	if alliance {
		entry = gy.horde
	}
	guid := waitGO(t, attacker, entry)
	assertSelectable(t, attacker, guid, false)
	// Opening is cast with TARGET_FLAG_GAMEOBJECT, just like a normal client.
	// ScenarioBot.Cast targets units and would not exercise this path.
	attacker.ArmSpellWaiter()
	send(t, attacker, 0x12E, openingPayload(guid))
	result, err := attacker.WaitSpellID(opening, 15*time.Second)
	if err != nil || !result.Success {
		e2eharness.Assertf(t, "%s capture failed: result=%+v err=%v", gy.name, result, err)
	}
	assertOwnership(t, states, gy, alliance)
	checkBanner(t, bots, gy, alliance, true)
	t.Logf("PASS: %s captured and replacement banner disabled for both players", gy.name)
}

func checkBanner(t *testing.T, bots []*e2eharness.ScenarioBot, gy graveyard, alliance, locked bool) {
	t.Helper()
	entry := gy.horde
	if alliance {
		entry = gy.alliance
	}
	for _, bot := range bots {
		bot.Teleport(t, gy.x, gy.y, gy.z, sotaMap)
		guid := waitGO(t, bot, entry)
		assertSelectable(t, bot, guid, locked)
	}
}

func assertSelectable(t *testing.T, bot *e2eharness.ScenarioBot, guid uint64, locked bool) {
	t.Helper()
	if !eventually(3*time.Second, func() bool {
		obj := bot.World.GetObject(guid)
		return obj != nil && (obj.Value(goFlagsField)&goNotSelectable != 0) == locked
	}) {
		e2eharness.ConfirmedBugf(t, 23129, "%s sees GO %X with wrong interaction state: want locked=%t, object=%+v", bot.Name, guid, locked, bot.World.GetObject(guid))
	}
}

func assertOwnership(t *testing.T, states []*sotaState, gy graveyard, alliance bool) {
	t.Helper()
	if !eventually(5*time.Second, func() bool {
		for _, state := range states {
			if !state.is(gy.allianceWS, boolValue(alliance)) || !state.is(gy.hordeWS, boolValue(!alliance)) {
				return false
			}
		}
		return true
	}) {
		e2eharness.Assertf(t, "%s ownership world states do not match alliance=%t on both clients", gy.name, alliance)
	}
}

func destroyGate(t *testing.T, bot *e2eharness.ScenarioBot, states []*sotaState, g gate) {
	t.Helper()
	bot.Teleport(t, g.x+3, g.y, g.z, sotaMap)
	waitGO(t, bot, g.entry)
	// Setup only: learn/cast an existing building-damage spell, never write gate
	// fields or world states. The battleground must receive its real destruction event.
	for hit := 0; hit < 32 && !states[0].is(g.ws, 3); hit++ {
		result := bot.CastAtPosition(t, gateDamage, g.x, g.y, g.z, 5*time.Second)
		if !result.Success {
			e2eharness.Preconditionf(t, "%s gate damage cast failed: %+v", g.name, result)
		}
		bot.FlushWorld(t)
	}
	if !eventually(5*time.Second, func() bool { return states[0].is(g.ws, 3) && states[1].is(g.ws, 3) }) {
		e2eharness.Preconditionf(t, "%s gate not destroyed after bounded damage setup", g.name)
	}
	t.Logf("destroyed %s gate", g.name)
}

func waitGO(t *testing.T, bot *e2eharness.ScenarioBot, entry uint32) uint64 {
	t.Helper()
	var guid uint64
	if !eventually(8*time.Second, func() bool {
		guid = bot.World.FindGameObjectByEntry(entry, 30)
		return guid != 0
	}) {
		e2eharness.Preconditionf(t, "%s cannot see GO entry %d on map %d", bot.Name, entry, sotaMap)
	}
	return guid
}

func joinSota(t *testing.T, bot *e2eharness.ScenarioBot, state *sotaState, instance uint32) uint32 {
	t.Helper()
	payload := make([]byte, 17) // battlemaster GUID (unused), type, instance, joinAsGroup
	binary.LittleEndian.PutUint32(payload[8:], sotaType)
	binary.LittleEndian.PutUint32(payload[12:], instance)
	send(t, bot, cmsgBattlemasterJoin, payload)
	var invited uint32
	select {
	case invited = <-state.invites:
	case <-time.After(45 * time.Second):
		e2eharness.Preconditionf(t, "no SotA invitation for %s; enable Debug.Battleground=1 on an exclusive test realm", bot.Name)
	}
	if instance != 0 && invited != instance {
		e2eharness.Preconditionf(t, "bots invited to different SotA instances: %d vs %d", instance, invited)
	}
	send(t, bot, cmsgBattlefieldPort, portPayload(true))
	if !eventually(20*time.Second, func() bool {
		_, _, _, _, mapID := bot.World.Position()
		return mapID == sotaMap
	}) {
		e2eharness.Preconditionf(t, "%s failed to enter SotA", bot.Name)
	}
	bot.WaitInWorld(t, 10*time.Second)
	return invited
}

func portPayload(enter bool) []byte {
	payload := make([]byte, 9)
	binary.LittleEndian.PutUint32(payload[2:], sotaType)
	binary.LittleEndian.PutUint16(payload[6:], 0x1F90)
	if enter {
		payload[8] = 1
	}
	return payload
}

func openingPayload(guid uint64) []byte {
	payload := make([]byte, 11) // castCount, spellID, castFlags, targetFlags, packed GUID mask
	binary.LittleEndian.PutUint32(payload[1:], opening)
	binary.LittleEndian.PutUint32(payload[6:], 0x800) // TARGET_FLAG_GAMEOBJECT
	for i := 0; i < 8; i++ {
		if b := byte(guid >> (8 * i)); b != 0 {
			payload[10] |= 1 << i
			payload = append(payload, b)
		}
	}
	return payload
}

func send(t *testing.T, bot *e2eharness.ScenarioBot, opcode uint16, payload []byte) {
	t.Helper()
	if err := bot.World.SendPacketRaw(opcode, payload); err != nil {
		e2eharness.HarnessFailf(t, "send opcode %X: %v", opcode, err)
	}
}

// Observer is armed before joining; snapshot reads and packet callbacks are synchronized.
type sotaState struct {
	mu      sync.Mutex
	values  map[uint32]uint32
	invites chan uint32
}

func newSotaState(t *testing.T, bot *e2eharness.ScenarioBot) *sotaState {
	s := &sotaState{values: make(map[uint32]uint32), invites: make(chan uint32, 4)}
	t.Cleanup(bot.World.AddPacketHook(s.packet))
	return s
}

func (s *sotaState) packet(op uint16, data []byte) {
	s.mu.Lock()
	defer s.mu.Unlock()
	switch op {
	case smsgBattlefieldStatus:
		if len(data) >= 27 && binary.LittleEndian.Uint32(data[6:]) == sotaType &&
			binary.LittleEndian.Uint32(data[19:]) == 2 && binary.LittleEndian.Uint32(data[23:]) == sotaMap {
			select {
			case s.invites <- binary.LittleEndian.Uint32(data[14:]):
			default:
			}
		}
	case smsgInitWorldStates:
		if len(data) < 14 || binary.LittleEndian.Uint32(data) != sotaMap {
			return
		}
		n := int(binary.LittleEndian.Uint16(data[12:]))
		if len(data) < 14+8*n {
			return
		}
		s.values = make(map[uint32]uint32)
		for i := 0; i < n; i++ {
			row := data[14+8*i:]
			s.values[binary.LittleEndian.Uint32(row)] = binary.LittleEndian.Uint32(row[4:])
		}
	case smsgUpdateWorldState:
		if len(data) >= 8 {
			s.values[binary.LittleEndian.Uint32(data)] = binary.LittleEndian.Uint32(data[4:])
		}
	}
}

func (s *sotaState) get(id uint32) (uint32, bool) {
	s.mu.Lock()
	defer s.mu.Unlock()
	v, ok := s.values[id]
	return v, ok
}

func (s *sotaState) is(id, want uint32) bool {
	v, ok := s.get(id)
	return ok && v == want
}

func boolValue(v bool) uint32 {
	if v {
		return 1
	}
	return 0
}

func eventually(timeout time.Duration, predicate func() bool) bool {
	timer := time.NewTimer(timeout)
	defer timer.Stop()
	tick := time.NewTicker(25 * time.Millisecond)
	defer tick.Stop()
	for {
		if predicate() {
			return true
		}
		select {
		case <-timer.C:
			return false
		case <-tick.C:
		}
	}
}
