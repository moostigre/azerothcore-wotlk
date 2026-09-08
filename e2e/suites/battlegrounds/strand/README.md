# Strand of the Ancients graveyard interaction — AC #23129

`TestAC_23129_CapturedGraveyardsStayNonInteractable` exercises the live
`BattlegroundSA` lifecycle using an Alliance and a Horde protocol client.

## Dedicated test-realm prerequisite

Use an **exclusive disposable realm**, configured with `Debug.Battleground = 1`.
This permits the two bots to start a match without recruiting a full battleground.
Do not run alongside real players, matchmaking bots, or other battleground tests;
disable cross-faction battleground modules for this stock-core scenario.

The test does **not** toggle `.debug bg`, reload configuration, edit world DB
templates, create persistent spawns, or require ToCloud9. Joining/leaving uses
normal battleground packets, and cleanup leaves the queue/match before disconnecting.
Fixture accounts/characters follow the harness's normal lifecycle.

This dedicated profile is explicitly opt-in; an ordinary full-suite run reports
this scenario as **SKIP**, not a validated pass. With the opt-in set, unmet setup
conditions fail as `precondition:` rather than silently skipping.

From `e2e/`, after exporting the standard `E2E_*` connection variables:

```sh
E2E_SOTA_TEST_REALM=1 go test -tags=e2e ./suites/battlegrounds/strand \
  -run '^TestAC_23129_' -count=1 -v -parallel=1 -p=1 -timeout=15m
```

Allow approximately 6–10 minutes, including the normal first- and second-round
warmups. Round one deliberately lasts at least three minutes so round two has
enough time for the same assertions. Repeat with `-count=2 -timeout=30m` when
validating a built fix.

## Assertions

- The bots accept invitations to the same actual SotA instance (map 607).
- All three graveyards begin defender-owned and their banners are non-selectable.
- Destroying the green gate enables the side banners, but not the central banner.
- The attacker completes spell 21651 through `CMSG_CAST_SPELL` with a gameobject
  target; both clients receive the correct graveyard ownership world states.
- Every replacement banner immediately has `GO_FLAG_NOT_SELECTABLE` for **both**
  clients, including the old defender. This is the native-client interaction flag
  that prevents the cogwheel/capture attempt.
- Destroying the blue gate does not re-enable the captured side banners.
- Destroying red enables the central banner; after capture, destroying purple,
  yellow and ancient gates leaves **all three** captured banners non-selectable.
- The Titan Relic remains selectable after all gates fall, despite central
  graveyard ownership; using it actually advances round one into round two.
- Round two resets ownership and banner interaction state. The complete capture
  and later-gate sequence repeats with the opposite faction attacking.

Gate damage is setup: the attacker learns and casts Huge Seaforium Blast (66672)
at the real gates, exercising `ModifyHealth` and the real destruction callback.
The test waits for destroyed-gate world states; it never fabricates flags,
ownership, or round progress. It does not test demolisher steering or bomb items.

The oracle is protocol/object state, **not rendered pixels**: visually confirming
the cogwheel and capture animation still requires a native WoW client. Deliberately
forged casts against non-selectable targets are not the ordinary client path and
are outside this UI regression; the existing authoritative capture guard remains.

## Validation status

Authoring checks passed: C++ codestyle, `git diff --check`, Go package
compilation, `go vet`, and all three offline wire-helper tests. The live scenario
has **not been run**; only its default opt-in skip was checked.

The live scenario requires a worldserver built with this branch. Compilation of
the Go package and offline wire-helper tests do not prove the in-game assertions.
No AzerothCore build or deployment is performed by the test suite.

Offline helper checks (no realm/DB connections):

```sh
go test -tags=e2e ./suites/battlegrounds/strand -run '^TestSotaWire_' -count=1
```
