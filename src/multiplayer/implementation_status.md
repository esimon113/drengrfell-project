# Multiplayer implementation status

Last updated: 13 September 2026.

This is the working context for turning Drengrfell into a client–server multiplayer game. It is not a playable multiplayer client yet.

## How to verify (macOS)

The OpenGL game does not run on macOS. You do **not** need that binary.

- `session_logic_test` and `network_tests` both pass on macOS (`build-mp`). They do **not** launch the game. The session test still links GLFW/gl3w plus a HUD stub (`test/session/render_notification_stub.cpp`).
- A GL-free `drengrfell_core` target is still the right way to stop depending on that stack.

## Locked architecture

- One authoritative server. Every player is a client, including the host (`Midgard` over localhost TCP).
- Baseline is Bifrost / Asgard / Midgard / SessionManager from `feature/multiplayer`, **not** unused POSIX stubs.
- Keep the real `src/main.cpp` (`Application`). Do not take that branch’s lobby-demo `main` or stripped CMake.
- `GameController` takes `GameState&` only — no `Registry*`, no UI from the controller.
- Map distribution by **seed/config**. Never use `Graph::deserialize` (undefined behavior: stack pointers; node vectors never filled).
- Snapshots via `serializeFor(playerId)` (true hidden information).
- Reconnect identity: `playerName`. Host process death ends the session. Player count: 2–6.
- v1 network commands: build settlement, build road, **MoveHero (at most one successful move per turn)**, end turn, settlement upgrade, productivity buildings.
- Out of MP v1: tutorial, AI, weather, and hazards/quests/trading as **networked** features. Neutralize hooks so a headless server does not crash. Single-player can keep some of this as a client of the same controller.
- Weather on the server stays SUNNY / `weatherModifier` 0.

## What is implemented (uncommitted on this branch)

- Protocol stack imported; game entry point kept.
- Headless controller: heroes live on `Player` / `Hero`; `movedThisTurn`; `endTurn` does not call weather/HUD.
- Seed written back into lobby/world config; insular generator uses `config.seed` after resolution; `Player` JSON includes hero.
- `GameState::serialize` writes world config, not graph JSON. `serializeFor` strips other players’ resources and explored tiles, hides heroes on unexplored tiles, and filters settlements/roads/productivity buildings by viewer fog (`Player::exploredTileIds`).
- SessionManager: land-tile hero spawn, `moveHero`, per-socket `serializeFor`, `GameStarted` / `GameState` / reconnect snapshots.
- Asgard sends per-socket filtered state (not one global dump).
- Bifrost + Midgard + SessionManager: `UpgradeSettlement`, `BuildProductivityBuilding`.
- Spec (`bifrost_specification.md`) uses per-player sync, not “Full State Sync”.
- Quest notifications are null-safe if there is no render system.
- Socket-free test: `test/session/session_logic_test.cpp`.

## Not done

- GL-free `drengrfell_core` + `drengrfell_server` CMake (session test still pulls GLFW via quests/HUD headers).
- Application driven by Midgard: apply **partial** snapshots to ECS (add **and remove** entities).
- `localPlayerId` vs `currentPlayerId` (lots of `getPlayer(0)` / `animations.entities.front()` remain).
- Two fog stores still exist: `Tile::visibleForPlayers` vs `Player::exploredTileIds` (snapshot path uses the player list).
- Encode/send off the sim thread.
- Client must not call `GameState::deserialize`’s `map` fallback (`Graph::deserialize`). Current `serialize()` omits `map` and prefers `world` + `regenerate`.

## Verification results

See the bottom of this file after the latest local check.

---

## Verification log

Date: 13 September 2026 (macOS AppleClang, `build-mp`). Did **not** run the OpenGL `drengrfell` binary.

### Ran

- `network_tests` — **pass** (TCP echo, lifecycle, max connections, rate limit).
- `session_logic_test` — **pass** after two compile fixes and a test-only HUD stub:
  - `GameController` missing `#include "utils/worldNodeMapper.h"`
  - `UpgradeSettlement` / productivity payload used an ambiguous `tileTypeToString`
  - `test/session/render_notification_stub.cpp` supplies empty `showNotification` so quests can link without the real GL HUD
- Two clients join, both ready, host `startGame`, world seed is non-zero (`1955403660` this run), `endTurn` advances `currentPlayerId` to 1, `UpgradeSettlement` JSON roundtrips, viewer 0 does not receive player 1 `resources`.

### Static review (code, not the game)

- `GameController` has no `Registry*`. `endTurn` does not call weather/HUD.
- `moveHeroToTile` rejects a second move via `hasMovedThisTurn()`.
- `GameState::serialize()` writes `world` config, not graph JSON. Asgard uses `getSerializedGameStateForSocket` → `serializeFor`.
- Fog for snapshots uses `Player::exploredTileIds` (`isTileVisibleTo`).
- `Graph::deserialize` is **not** used on the serialize path. `GameState::deserialize` still **falls back** to it if a payload has `map` and no `world`. Clients must not send that.
- `serializeFor` does **not** strip other players’ `settlementIds` / `roadIds` / `productivityBuildingIds` / `heroPoints`. Object lists are fog-filtered; id lists on the player object can still leak existence.
- `session_logic_test` does **not** require a successful `buildSettlement`. This run logged `vertex 0 not found` and still passed (it only rejects a missing error object).
- SessionManager still owns a `Registry` and `GameController` still constructs `QuestsSystem` (render header). Headless core is not fully cut yet.

### Not verified

- OpenGL client, Midgard applying snapshots to ECS, localhost host client path, reconnect, upgrade/productivity **game** success (only protocol encode for upgrade).
