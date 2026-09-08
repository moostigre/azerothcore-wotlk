# Twin Val'kyr Touch regression

Tracks [AC #19834](https://github.com/azerothcore/azerothcore-wotlk/issues/19834).

## Run

Use an isolated test realm with this branch's worldserver. Configure `E2E_AUTH_ADDR`,
`E2E_AUTH_DSN`, `E2E_CHAR_DSN`, and `E2E_WORLD_DSN` as described in the main
[README](../../../../README.md). From `e2e/`:

```sh
go test -tags=e2e ./suites/instances/northrend/trial_of_the_crusader \
  -run '^TestAC_19834_' -count=2 -p 1 -parallel 1 -v -timeout 5m
```

The test creates three level-80 characters per difficulty and a fresh raid group.
It requests 10-player or 25-player heroic via the client protocol before entering map 649.
That map is mandatory: the Touch script deliberately does nothing at outdoor isolation pads.
Run serially; group members must be on the same world instance/layer.

## Coverage

| Difficulty | Touch of Light / matching Essence | Touch of Darkness / matching Essence |
|------------|----------------------------------|-------------------------------------|
| 10 heroic | 67297 / 67223 | 67282 / 67177 |
| 25 heroic | 67298 / 67224 | 67283 / 67178 |

A passive training dummy casts Touch once onto the opposite-color holder. Real Essence NPC
gossip selects colors, and a priest casts Power Word: Shield through the client protocol.
Health is increased for survival; **god mode is explicitly disabled** so it cannot mask the bug.

Assertions:

- An opposite-color holder actually loses health, and both opposite-color players receive periodic ticks.
- A matching-color observer receives no Touch ticks, loses no health, and gains no Powering Up aura.
- Shield absorption is reported, and damage + absorption + resistance matches the raw heroic tick range.
- Switching an observer to the opposite color exposes them to the still-active Touch;
  switching back protects them while ticks continue on its holder.
- Switching the holder to the matching color removes Touch through the existing gossip script.

The fixture cleans up its spawned dummy and Essence NPCs and removes Touch on assertion failures.
Boss scheduling, full encounter progression, orb/vortex mechanics, and the normal-mode spell variants
are outside this E2E's scope. The fix nevertheless supplies matching-Essence exclusions for all eight
Touch spell IDs, consistently with the existing Surge mappings.

## Verification — 2026-09-08

- E2E package compilation and Go vet passed; C++ codestyle and `git diff --check` passed.
- The existing, **unfixed** PTR binary (`ca8e6d78dee1+`) reproduced AC #19834 for all four heroic
  Touch variants across two runs: matching-color players took approximately 3,000 / 6,000 damage
  per tick and incorrectly gained Powering Up.
- The runs also encountered separate PTR layer/instance co-location setup failures. These failed
  explicitly as preconditions, not as successful tests or additional confirmations of this issue.
- **No fixed-binary live run or C++ build has been performed.** Post-fix absorption and color-switch
  assertions still require verification after building/deploying this branch. The baseline runs
  fail at the earlier matching-color assertion and do not establish those later checks as passing.
