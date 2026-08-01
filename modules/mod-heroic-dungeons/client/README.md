# Cloud9 Heroic UI client patch

This patch exposes the client's standard **Dungeon Difficulty** submenu to
characters below level 65. It does not replace `Wow.exe` and does not add a
custom protocol: selecting **5 Player (Heroic)** calls the stock
`SetDungeonDifficulty(2)` function.

## Installation (WoW 3.3.5a build 12340)

1. Close the game.
2. Copy the `Cloud9HeroicUI` directory into
   `World of Warcraft/Interface/AddOns/`.
3. Start the game and enable **Cloud9 Heroic UI** in the AddOns screen.
4. Right-click the player portrait, open **Dungeon Difficulty**, and select
   **5 Player (Heroic)** before entering the dungeon.

Only a solo player or the party/raid leader can change difficulty. The normal
client restrictions while inside an instance or in an LFG-restricted group
remain intact.

The server remains authoritative: showing this menu does not make an
unconfigured dungeon heroic. The server module decides which maps have heroic
configuration and applies the configured level range and modifiers.
