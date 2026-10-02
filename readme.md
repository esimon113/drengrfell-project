# Drengrfell

**Drengrfell** (from Old Norse *drengr* for hero and *fell* for mountain) is a turn-based strategy game that blends exploration, resource management, and survival, drawing inspiration from *Settlers of Catan*. Developed as an university project, the game challenges players to explore a generated hexagonal wilderness, establish strategic settlements and road networks, and navigate dynamic environmental hazards. The project shows high-performance modern C++ engineering, featuring a custom engine built on an Entity Component System (ECS) and a sophisticated tri-partite graph backend that manages the complex interplay between terrain, infrastructure, and player progression.


## Technical Highlights

* **Graph-Based Game Logic**: The core engine manages a tri-partite graph where:
    * **Tiles** (Nodes) represent resource-generating biomes.
    * **Vertices** (Nodes) represent potential settlement locations.
    * **Edges** represent road connections.
    This architecture enables efficient pathfinding and adjacency-based resource distribution.
* **World Generation**: Provides two mechanisms for world generation. Either randomly assign tiles to the map and surround it with water, or utilize multi-octave **Perlin noise** for biome distribution and terrain elevation. The generation is configurable via JSON and includes custom-authored textures for diverse environments (forests, mountains, plains, ocean).
* **Entity Component System (ECS)**: Built on the **tinyecs** framework, the game decouples data (Components) from logic (Systems). This ensures high performance for batch rendering and complex state updates.
* **Custom Rendering Pipeline**: Developed using **OpenGL**, featuring:
    * Custom GLSL shaders for **dynamic lighting, shadows, and Fog of War**.
    * **Texture Arrays** for efficient tile and sprite rendering.
    * Animated character, tile, and settlement sprites using frame-based animation systems.
    * Framebuffer-based effects for UI and post-processing.
* **Advanced Gameplay Systems**:
    * **Exploration**: Hero-based exploration with a persistent Fog of War state.
    * **Hazard System**: Environmental triggers that affect player economy and movement.
    * **Quest System**: A data-driven system for managing player objectives and progression.


## Tech Stack

* **Language**: C++20
* **Graphics**: OpenGL, GLFW, GLM, gl3w
* **ECS Framework**: tinyecs
* **Audio**: miniaudio
* **Data & Assets**: nlohmann_json, tinyobjloader, stb_image, freetype (text rendering)
* **Build System**: CMake with automated dependency management via FetchContent.


## Project Structure

The codebase is organized into modular directories following a clear separation of concerns:

- **`src/`**: Main application entry point and global ECS registry management.
- **`src/core/`**: Domain-specific logic. Contains the Graph implementation, Game State management, and World Generation algorithms.
- **`src/systems/`**: ECS Systems. Discrete logic for rendering (Tiles, Hero, HUD), Physics, Audio, and Gameplay (Quests, Movement).
- **`src/utils/`**: Engine-level utilities and OpenGL abstractions (Shaders, Textures, Framebuffers, Mesh loaders).
- **`assets/`**:
    - `shaders/`: Custom GLSL source code.
    - `textures/`: Hand-crafted environment and character sprites.
    - `mesh/`: OBJ models for buildings and environmental objects.
    - `jsons/`: Configuration for world generation and game balance.

## Getting Started

### Prerequisites

* **CMake** (v3.24 or higher)
* **C++20 Compiler** (GCC 11+, Clang 13+, or MSVC 19.30+)
* **OpenGL Drivers**

Note that MacOS is currently not supported.

### Build & Run

The project includes automation scripts for quick setup:

* **Linux**:
  ```bash
  chmod +x build-run.sh  # add execution permission
  ./build-run.sh
  ```
* **Windows**:
  Run `build-run.bat` from the root directory.

Alternatively, you can use standard CMake commands:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

That build produces two programs: `drengrfell_server` and `drengrfell`. Every match is a server plus one window per player.

To play alone, start only the window:

```bash
./build/drengrfell --solo
```

With `--solo` and the default address, the window runs its own server on `127.0.0.1`, port 7777, and only this machine can reach it. On Linux, if a `drengrfell_server` already listens on that port, the window joins that server instead.

### Playing on one network

Start the server on the machine that should host the match. It listens on every network interface, port **7777**:

```bash
./build/drengrfell_server
```

On Windows, run `build\drengrfell_server.exe`. An optional port argument changes the port: `./build/drengrfell_server 7777`.

A window connects to `127.0.0.1` unless you pass `--host`. On the server machine, and on every other device on the same network, start a window with that machine's LAN address and a distinct player name:

```bash
./build/drengrfell --host 192.168.1.20 --name Alice
./build/drengrfell --host 192.168.1.20 --name Bob
```

Replace `192.168.1.20` with the server machine's address. Each device needs its own build of the game and a copy of `assets/`. The server shares the match, not the program.

The first player to join is the host. Without `--solo`, the lobby stays open until at least two players are ready, and the host then starts the match. `--solo` starts with one player. Names must be unique. Up to six players can join.

| Flag | Meaning |
|------|---------|
| `--host <address>` | Server address. Default `127.0.0.1`. |
| `--port <port>` | Server port. Default `7777`. |
| `--name <name>` | Player name. Default `Player`. |
| `--solo` | Allow the host to start alone. With the default address, the window also runs its own server. |

If a firewall is enabled on the server machine, allow inbound TCP **7777**. A player who disconnects during a match pauses the game. This build does not reconnect them.

---

## Keybindings

Click a tile to choose where your hero walks. Enter, or the End Turn button, walks there (one move per turn) and ends your turn. You can act only on your own turn.

| Key | Function |
|-----|----------|
| W A S D | Move the map |
| N | Preview settlements |
| B | Preview roads |
| Q | Active quests |
| C | Building costs |
| V | How victory points are scored |
| T | Trade 4 of one resource for 1 of another with the bank, on your turn |
| K | This list |
| Esc | Close the open window |
| + - | Zoom. On a German layout this is the key that types `+` or `-`, and the numpad keys |
| Space | Center the camera on your hero |

