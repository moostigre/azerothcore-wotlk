# mod-heroic-dungeons

Server-side heroic versions of expansion endgame dungeons for AzerothCore
3.3.5a. The first implementation enables Heroic Stratholme (map 329).

The module uses the client's standard Normal/Heroic selector. It adds a
server-side heroic `MapDifficulty` record, so no client patch is required for
entering the heroic instance. Normal Stratholme remains unchanged.

## Configuration model

The distributed configuration supports:

- dungeon-wide health, melee-damage, and spell-damage multipliers;
- per-creature health and melee overrides;
- per-creature/per-spell damage modifiers;
- replacement cooldowns for spells already used by a creature;
- additional spell IDs with independent initial and repeat cooldowns;
- victim, random-player, and self targeting;
- dedicated C++ encounter extensions for mechanics that do not fit a generic
  spell scheduler.

`replaceOriginal=1` makes the module authoritative for that creature/spell
pair: the normal AI cast is rejected and the heroic scheduler casts it at the
configured interval. This is useful for changing existing SmartAI cooldowns
without rewriting the normal-mode SmartAI rows.

## Stratholme example

The default configuration:

- gives all heroic Stratholme creatures 3x health, 1.6x melee damage, and
  1.4x spell damage;
- gives Baron Rivendare and Archivist Galford individual stat overrides;
- replaces their configured existing-spell cooldowns;
- adds Shadow Bolt Volley to Baron Rivendare;
- extends Baron Rivendare with skeleton waves at 70% health and an enrage
  phase at 35% health.

The generic engine lives in `src/heroic_dungeons.cpp`. Boss-specific mechanics
live in separate files such as `src/boss_baron_rivendare_heroic.cpp`, allowing
future encounters to add phases, summons, movement, or instance coordination
without turning the generic configuration format into a scripting language.
