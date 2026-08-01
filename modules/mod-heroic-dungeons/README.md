# mod-heroic-dungeons

Server-side heroic versions of expansion endgame dungeons for AzerothCore
3.3.5a. The first implementation enables Heroic Stratholme (map 329).

The module uses the client's standard Normal/Heroic selector. It adds a
server-side heroic `MapDifficulty` record, so no client patch is required for
entering the heroic instance. Normal Stratholme remains unchanged.

## YAML configuration

`mod_heroic_dungeons.conf` contains the module enable flag and the path to
`heroic_dungeons.yaml`. The YAML file is the source of truth for:

- dungeon-wide health, melee-damage, and spell-damage multipliers;
- per-creature health and melee overrides;
- per-creature/per-spell damage modifiers;
- replacement cooldowns for spells already used by a creature;
- additional spell IDs with independent initial and repeat cooldowns;
- victim, random-player, and self targeting;
- dedicated C++ encounter extensions for mechanics that do not fit a generic
  spell scheduler.
- Heroic-only loot tables in `add` or `replace` mode.

At startup the module parses every enabled dungeon, creates its Heroic
`MapDifficulty`, enables the Heroic spawn bit for that map, and synchronizes
YAML-owned rows in `creature_loot_template`. YAML-owned loot rows use comments
beginning with `Heroic YAML:` so the synchronization never deletes unrelated
loot.

`add` mode combines the normal table with the YAML Heroic table. `replace`
mode selects only the YAML Heroic table. Runtime multipliers and spell rules
are refreshed by a configuration reload; spawn and loot template changes
should be applied with a worldserver restart.

`replaceOriginal=1` makes the module authoritative for that creature/spell
pair: the normal AI cast is rejected and the heroic scheduler casts it at the
configured interval. This is useful for changing existing SmartAI cooldowns
without rewriting the normal-mode SmartAI rows.

## Stratholme example

The active Stratholme YAML configuration:

- gives all heroic Stratholme creatures 3x health, 1.6x melee damage, and
  1.4x spell damage;
- gives Baron Rivendare and Archivist Galford individual stat overrides;
- replaces their configured existing-spell cooldowns;
- adds Shadow Bolt Volley to Baron Rivendare;
- extends Baron Rivendare with skeleton waves at 70% health and an enrage
  phase at 35% health.
- makes the Stratholme service entrance select Heroic difficulty before the
  teleport, while leaving the main entrance unchanged;
- gives Heroic Baroness Anastari additional Banshee mechanics and a guaranteed
  Savage Gladiator Chain (item 11726) drop through Heroic loot mode.

The generic engine lives in `src/heroic_dungeons.cpp`. Boss-specific mechanics
live in separate files such as `src/boss_baron_rivendare_heroic.cpp`, allowing
future encounters to add phases, summons, movement, or instance coordination
without turning the generic configuration format into a scripting language.

The distributed YAML ends with a fully commented Ragefire Chasm example that
documents every generic dungeon, creature, spell, targeting, cooldown, and
loot option.
