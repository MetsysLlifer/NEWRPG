#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "raymath.h"
#include <stdlib.h> 
#include <stdio.h> 

// --- WORLD SETTINGS ---
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CELL_SIZE 8 
#define GRID_W (SCREEN_WIDTH / CELL_SIZE)
#define GRID_H (SCREEN_HEIGHT / CELL_SIZE)

// --- BLOCK TYPES ---
typedef enum {
    BLOCK_AIR = 0,
    BLOCK_STONE, // Wall (Indestructible-ish)
    BLOCK_DIRT,  // Wall
    BLOCK_SAND,  // Wall (But looks grainy)
    BLOCK_WATER, // Floor (Spreads)
    BLOCK_LAVA,  // Floor (Spreads + Damage)
    BLOCK_WOOD,  // Wall (Flammable)
    BLOCK_FIRE,  // Emitter
    BLOCK_SMOKE, // Visual
    BLOCK_COUNT
} BlockType;

// --- PLAYER ---
typedef struct {
    Vector2 position; 
    Vector2 velocity;
    float size;       
    Color color;
} Player;

// --- INVENTORY ---
typedef struct {
    BlockType slots[9];
    int selected;
} Inventory;

// --- GRID SYSTEM ---
typedef struct {
    BlockType type;
    int life;       // For decay (Fire/Smoke)
    bool active;    // Optimization
    Color color;    // Persistent noise color
} Cell;

// --- PROTOTYPES ---
void InitWorld();
void UpdateWorld(); // Cellular Automata (Top-Down Logic)
void DrawWorld();

void InitPlayer(Player* p);
void UpdatePlayer(Player* p, float dt);
void DrawPlayer(Player* p);

// Interaction
void EditWorld(int x, int y, BlockType type, int radius);
bool IsSolid(BlockType t); // True if it blocks player movement

// UI
void DrawHUD(Player* p, Inventory* inv);

#endif