//go:build e2e

package strand_test

import (
	"bytes"
	"encoding/binary"
	"testing"
)

// These are offline tests of the narrow protocol helpers, not evidence that the
// battleground regression passes. The live oracle is TestAC_23129_*.
func TestSotaWire_OpeningTargetsGameObject(t *testing.T) {
	// Packed GUID F110012345000007 has two zero bytes.
	want := []byte{0, 0x93, 0x54, 0, 0, 0, 0, 8, 0, 0, 0xF9, 7, 0x45, 0x23, 1, 0x10, 0xF1}
	if got := openingPayload(0xF110012345000007); !bytes.Equal(got, want) {
		t.Fatalf("opening payload: got %X, want %X", got, want)
	}
	for _, enter := range []bool{false, true} {
		got := portPayload(enter)
		if len(got) != 9 || binary.LittleEndian.Uint32(got[2:]) != sotaType ||
			binary.LittleEndian.Uint16(got[6:]) != 0x1F90 || got[8] != byte(boolValue(enter)) {
			t.Fatalf("invalid battlefield port payload: %X", got)
		}
	}
}

func TestSotaWire_WorldStatesAndMalformedPackets(t *testing.T) {
	s := &sotaState{values: map[uint32]uint32{999: 1}, invites: make(chan uint32, 4)}
	initial := make([]byte, 30)
	binary.LittleEndian.PutUint32(initial, sotaMap)
	binary.LittleEndian.PutUint16(initial[12:], 2)
	binary.LittleEndian.PutUint32(initial[14:], wsAllianceAttacks)
	binary.LittleEndian.PutUint32(initial[18:], 1)
	binary.LittleEndian.PutUint32(initial[22:], wsTimer)
	s.packet(smsgInitWorldStates, initial)
	if !s.is(wsAllianceAttacks, 1) || !s.is(wsTimer, 0) {
		t.Fatal("initial world states were not decoded")
	}
	if _, ok := s.get(999); ok {
		t.Fatal("initial packet retained stale world states")
	}
	update := make([]byte, 8)
	binary.LittleEndian.PutUint32(update, wsTimer)
	binary.LittleEndian.PutUint32(update[4:], 1)
	s.packet(smsgUpdateWorldState, update)
	if !s.is(wsTimer, 1) {
		t.Fatal("timer update was not decoded")
	}
	for _, op := range []uint16{smsgInitWorldStates, smsgUpdateWorldState, smsgBattlefieldStatus} {
		for n := 0; n < 8; n++ {
			s.packet(op, make([]byte, n))
		}
	}
	s.packet(smsgInitWorldStates, initial[:29]) // truncated last pair must not clear state
	if !s.is(wsTimer, 1) {
		t.Fatal("malformed packet changed observer state")
	}
}

func TestSotaWire_InvitationFiltersBattlegroundAndStatus(t *testing.T) {
	s := &sotaState{values: make(map[uint32]uint32), invites: make(chan uint32, 4)}
	packet := make([]byte, 39)
	binary.LittleEndian.PutUint32(packet[6:], sotaType)
	binary.LittleEndian.PutUint32(packet[14:], 123)
	binary.LittleEndian.PutUint32(packet[19:], 2)
	binary.LittleEndian.PutUint32(packet[23:], sotaMap)
	s.packet(smsgBattlefieldStatus, packet)
	select {
	case id := <-s.invites:
		if id != 123 {
			t.Fatalf("got instance %d, want 123", id)
		}
	default:
		t.Fatal("invitation not decoded")
	}
	for _, offset := range []int{6, 19, 23} {
		bad := append([]byte(nil), packet...)
		binary.LittleEndian.PutUint32(bad[offset:], 99)
		s.packet(smsgBattlefieldStatus, bad)
	}
	select {
	case id := <-s.invites:
		t.Fatalf("accepted wrong battleground/status: %d", id)
	default:
	}
}
