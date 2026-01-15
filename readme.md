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

---

## Keybindings

| Key     | Function                          |
|---------|-----------------------------------|
| K       | Toggle to see this table          |
| F       | Toggle rendering of fog of war    |
| G       | Regenerate map                    |
| W A S D | Move map                          |
| N       | Preview settlements               |
| B       | Preview roads                     |
| P       | Print tile ID at mouse to console |
| Q       | See active quests                 |
| + -     | Zoom                              |
| H       | toggle movement                   |
| F7      | set Idle animation                |
| F8      | set Swim animation                |
| F9      | set Attack animation              |
| F10     | set Jump animation                |
| F11     | set Run animation                 |

