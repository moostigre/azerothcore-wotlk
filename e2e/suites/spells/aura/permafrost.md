# Permafrost / Hand of Freedom regression

Tracks [AC #16496](https://github.com/azerothcore/azerothcore-wotlk/issues/16496).

## Run

Configure the `E2E_*` variables described in the main [README](../../../README.md), including
`E2E_WORLD_DSN` for spawn cleanup. From `e2e/`:

```sh
go test -tags=e2e ./suites/spells/aura -run '^TestAC_16496_' -count=2 -p 1 -parallel 1 -v -timeout 5m
```

Use a test realm and run serially. The harness creates two level-80 characters, forms a party, and places
them at the package's isolation pad. Each case creates a Frost Sphere fixture, which is removed afterward.

## Assertions

The sphere casts the actual persistent-area spell once, creating a shared `DynObjAura`. Both players
must receive its aura and expected slow before the reproduction begins. A paladin then casts Hand of
Freedom (`1044`) through the client protocol and remains on the same ice patch until natural expiry.

| Variant | Permafrost spell | Paladin run speed, before / during / after Freedom | Control player |
|---------|------------------|--------------------------------------------------|----------------|
| 10 normal | 66193 | 4.90 / 7.00 / 4.90 yd/s | Stays at 4.90 |
| 25 normal | 67855 | 4.90 / 7.00 / 4.90 yd/s | Stays at 4.90 |
| 10 heroic | 67856 | 1.40 / 7.00 / 1.40 yd/s | Stays at 1.40 |
| 25 heroic | 67857 | 1.40 / 7.00 / 1.40 yd/s | Stays at 1.40 |

The oracle checks aura state **and server-issued run-speed packets**, including every control-player
speed update during Freedom. Checking only an icon would miss the historical shared-aura amount bug.
The returning slow has a 2-second deadline, allowing for the 500-ms area-target refresh and scheduling.
Setup failures fail as `precondition`; a broken Freedom interaction fails as `AC#16496 CONFIRMED BUG`.

The sphere is outside Trial of the Crusader, so encounter AI cannot recast Permafrost and hide a failure.
GM `.cast self` is fixture setup, not `.aura`: it exercises actual area targeting. Do not append
`triggered` to that command: its debug flag mask includes `TRIGGERED_IGNORE_EFFECTS`.

## Live verification — 2026-09-08

Two consecutive runs passed all four variants: **8/8 cases**, approximately 89 seconds total.
The returning slow was observed within approximately 100–151 ms of observed Freedom expiry, and each
control player remained slowed. **The reported spell interaction was not reproduced.**

Tested binary: existing PTR `ca8e6d78dee1+` (2026-08-23), not a freshly built master.
Test branch base: upstream `a5e0e6b8f2bf878cb45cb1dc2251eb1448b9bbc3`.
The relevant area-target refresh and immunity-removal paths were checked against that master;
no gameplay code was changed, rebuilt, or deployed for this test.

This is a focused spell-interaction regression, **not a full Anub'arak encounter test**: sphere falling,
visual rendering, difficulty-ID selection by the boss script, burrowers, and pursuing spikes are not asserted.
