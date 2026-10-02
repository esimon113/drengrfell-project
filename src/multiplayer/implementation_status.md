# Multiplayer implementation status

Last checked: 2 October 2026, against this branch.

A match is playable. Every window is a Midgard client. The authority is `drengrfell_server` (Asgard, bind `0.0.0.0:7777`), or an Asgard inside the window when `--solo` is used with host `127.0.0.1`. If that port is already taken, the window joins the server that has it.

## Rules

- First joiner is host. Start needs every joined player ready: 2 unless the host set solo, then 1. `MIN_PLAYERS` stays 1. Max 6. Names must be unique. No join after start.
- Server owns turns, one hero move per turn, buildings, upgrades, productivity buildings, weather, hazards, per-player tutorial, and per-player quests (including rewards). A hazard blocks movement and counts down only on that player's own end of turn. Pay works on any turn.
- Win is 20 points or 3 castles. The snapshot carries `winnerId`, so fogged castles still decide it.
- Snapshots use `serializeFor` / `applyAuthoritativeSnapshot`. The map is regenerated from the world seed. Other players' resources and explored tiles are omitted. Their hero is omitted unless the viewer has explored that tile.
- The walk animation stays on the client. The server tile is not applied while that hero is walking.
- While connected, the L-key AI does not run, and trade clicks do not change resources. Both still exist for a window that never connected.
- Disconnect during play sets the session to paused and rejects game commands. The protocol can reconnect by player name (`Midgard::reconnect`); no window calls it. A disconnected player is removed after `reconnectTimeoutSeconds` (default 60).

## Layout

- `GameController(GameState&)` has no `Registry*` and no UI. `SessionManager` has no `Registry`; the server game state is constructed with a null one so it does not allocate ECS entities.
- `QuestsSystem` still includes the notification header. `drengrfell_server`, `session_logic_test`, and `local_server_test` link a stub (`test/session/render_notification_stub.cpp`). There is no GL-free `drengrfell_core` target.
- `GameState::deserialize` still falls back to `Graph::deserialize` when a payload has `map` and no `world`. The client path does not use that; it regenerates from `world`.

## Tests

From the repo root: `./build/session_logic_test`, `./build/network_tests`, `./build/local_server_test`. They do not open a window. The OpenGL match has to be checked by playing it.

## Still open

- The window never reconnects, and it does not resume a paused match.
- Trading is not a server command.
- Turns stay sequential.
- After a finished match, "Back to Menu" returns to the menu; Start does not begin another match in the same process.
