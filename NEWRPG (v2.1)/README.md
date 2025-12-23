# NEWRPG

**Version 2.1**

Welcome to NEWRPG, a top-down 2D sandbox RPG where your creativity is your most powerful weapon. Dive into a dynamic, physics-based world, and craft powerful, custom spells to shape your environment and vanquish your foes.

## Inspiration

NEWRPG draws its core inspiration from two innovative games:

*   **Noita:** The deep, emergent physics and chemistry simulation that allows for creative problem-solving and gloriously chaotic fun.
*   **CodeSpells:** The idea of writing your own magic and seeing it come to life is a central theme. We aim to give players the tools to design and cast their own unique spells.

## About the Game

In NEWRPG, you take on the role of a powerful mage venturing through a dangerous and interactive world. Forget finding pre-made spells in dusty tomes. Here, you will craft your own magical abilities from scratch by combining different elements, principles, and modifiers.

The world is your magical sandbox. The physics engine allows for complex and often unpredictable interactions between your spells, the environment, and the creatures that inhabit it. Will you conjure a rain of fire, sculpt the terrain to your advantage, or create a chain reaction that accidentally destroys a whole cavern? The choice is yours.

## Core Features

*   **Dynamic Spell Crafting (`magic.c`):** A robust system for designing and creating your own unique spells. Combine elemental properties, behaviors, and modifiers to build your arsenal.
*   **Physics-Based Magic & World (`physics.c`):** Spells and objects interact with the world in emergent ways, governed by a custom physics engine.
*   **Custom Particle Engine (`particles.c`):** A vibrant particle engine brings your spells to life with beautiful and chaotic visual effects.
*   **Inventory System (`inventory.c`):** Manage your collection of magical ingredients, crafted artifacts, and other essential items.
*   **Simple 2D Graphics & UI (`graphics.c`, `ui.c`):** A clean graphics engine renders the world, while a simple UI keeps you informed.

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
| `physics.c`   | Manages the physics simulation for entities and spells.   |
| `magic.c`     | Contains the logic for the spell crafting system.         |
| `particles.c` | Implements the particle engine for visual effects.        |
| `inventory.c` | Manages the player's inventory.                           |
| `ui.c`        | Responsible for drawing UI elements to the screen.        |
| `makefile`    | Defines the build process and rules for compilation.      |

## Contributing

Contributions are welcome! If you have ideas for new spells, features, or improvements, feel free to open an issue or submit a pull request.

