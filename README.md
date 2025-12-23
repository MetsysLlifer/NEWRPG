# NEWRPG

**Version 2.1**

Welcome to NEWRPG, a top-down 2D physics-based RPG featuring dynamic entity interactions and emergent gameplay.

## ⚠️ Major Update Notice

**This version represents a complete reimagining of the game.** Due to significant limitations in the old game's physics system and spell mechanics, we've rebuilt the project from the ground up with a new world and streamlined gameplay architecture. The previous magic system has been removed to focus on core physics interactions and a cleaner, more maintainable codebase.

## Inspiration

NEWRPG draws its core inspiration from two innovative games:

*   **Noita:** The deep, emergent physics and chemistry simulation that allows for creative problem-solving and gloriously chaotic fun.
*   **CodeSpells:** The concept of programmable interactions and seeing your creations come to life.

## About the Game

In NEWRPG v2.1, you explore a dynamic, physics-driven world where every entity responds realistically to forces and collisions. The focus is on fundamental physics simulation, providing a solid foundation for future gameplay systems.

The world is an interactive sandbox. The custom physics engine allows for emergent behaviors through entity interactions, environmental dynamics, and realistic collision resolution.

## Core Features

*   **Robust Physics Engine (`physics.c`):** Entity movement, collision detection, and wall constraints with realistic friction and velocity handling.
*   **Dynamic Grass System (`graphics.c`):** Environmental grass that responds to entity movement with realistic bending and recovery.
*   **Custom Particle Engine (`particles.c`):** Visual effects system for explosions and other dynamic events.
*   **Inventory System (`inventory.c`):** Pick up, store, and drop items with keyboard-driven management.
*   **Simple 2D Graphics & UI (`graphics.c`, `ui.c`):** Clean rendering with health/mana display and inventory visualization.
*   **Entity Pool System:** Support for different entity states including raw entities, projectiles, static walls, and liquid pools.

## Technical Stack

*   **Language:** C (C99)
*   **Graphics Library:** [Raylib](https://www.raylib.com/)
*   **Compiler:** Clang
*   **Build System:** Make

## Getting Started

### Prerequisites

Before you begin, ensure you have `clang`, `make`, and the `raylib` library installed. On macOS, you can install `raylib` with Homebrew:
```bash
brew install raylib
```

### Building and Running

The project uses a `makefile` for a streamlined build process.

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/MetsysLlifer/NEWRPG.git
    cd NEWRPG
    ```

2.  **Compile the game:**
    ```bash
    make
    ```

3.  **Run the executable:**
    ```bash
    make run
    ```
    Or directly:
    ```bash
    ./game
    ```

## File Structure

Here is a brief overview of the key source files:

| File          | Description                                               |
|---------------|-----------------------------------------------------------|
| `main.c`      | The main entry point and game loop.                       |
| `game.h`      | Main header with core data structures and declarations.   |
| `graphics.c`  | Handles all rendering and graphics-related logic.         |
| `physics.c`   | Manages the physics simulation for entities.   |
| `particles.c` | Implements the particle engine for visual effects.        |
| `inventory.c` | Manages the player's inventory.                           |
| `ui.c`        | Responsible for drawing UI elements to the screen.        |
| `makefile`    | Defines the build process and rules for compilation.      |

## Contributing

Contributions are welcome! This is a learning project focused on physics simulation and game architecture. Feel free to open issues or submit pull requests for improvements to the physics engine, rendering optimizations, or new gameplay features.

## Version History

**v2.1** - Complete rebuild with simplified physics-focused architecture. Removed legacy magic system due to design limitations. Improved performance and maintainability.

