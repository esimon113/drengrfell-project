# Drengrfell

Turn-based strategy game: explore a generated hexagonal map, place settlements and roads, and deal with hazards. Inspired by *Settlers of Catan*. The Techstack consists of C++20, OpenGL 4.5, GLFW, CMake 3.24. Linux and Windows are supported, most testing happened on Linux.

Every game window is a client of a server. The server owns turns, building, movement, hazards, quests, and bank trades. Win with 20 points or 3 castles. Match rules are in [src/multiplayer/implementation_status.md](src/multiplayer/implementation_status.md).

## Build

```bash
# Build Script
chmod +x build-run.sh # set executable permission
./build-run.sh

# OR
cmake -S . -B build
cmake --build build
```

This produces `build/drengrfell` and `build/drengrfell_server` (on Windows, `build\drengrfell.exe` and `build\drengrfell_server.exe`).

For testing, run from repo root: `./build/session_logic_test`, `./build/network_tests`, `./build/local_server_test`. They do not open a window, so actual gameplay (GUI) has to be tested by the user.

## Play

Alone:

```bash
./build/drengrfell --solo
```

With `--solo` and the default address, the window runs a server on `127.0.0.1`, port 7777. If that port is already taken, the window joins the server that has it.

On a network, start the server on the host machine. It listens on every interface, port 7777. Then start one window per player, each with that machine's address and its own name:

```bash
./build/drengrfell_server
./build/drengrfell --host 192.168.1.20 --name Alice
```

An optional port argument changes the server port: `./build/drengrfell_server 7777`. The first player to join is the host. Two ready players are required, or one with `--solo`. Names must be unique. Up to six players can join. Each machine needs its own build and a copy of `assets/`. If a firewall is on, allow inbound TCP 7777 (or the set port). A disconnect pauses the match.

| Flag | Meaning |
|------|---------|
| `--host <address>` | Server address. Default `127.0.0.1`. |
| `--port <port>` | Server port. Default `7777`. |
| `--name <name>` | Player name. Default `Player`. |
| `--solo` | Start with one player. With the default address, the window also runs the server. |

## Keys

Click a tile, then Enter or End Turn. The hero walks there (one move per turn) and the turn ends. You can act only on your own turn.

| Key | Function |
|-----|----------|
| W A S D | Move the map |
| N | Preview settlements |
| B | Preview roads |
| Q | Active quests |
| C | Building costs |
| V | How victory points are scored |
| T | Trade with the bank, on your turn |
| J | Reset the tutorial |
| K | This list |
| Esc | Close the open window |
| + - | Zoom. On a German layout, the key that types `+` or `-`, and the numpad keys |
| Space | Center the camera on your hero |
