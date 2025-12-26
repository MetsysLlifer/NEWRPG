#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "raymath.h"
#include <stdlib.h> 
#include <stdio.h> 

// --- WORLD SETTINGS ---
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CELL_SIZE 4
#define GRID_W (SCREEN_WIDTH / CELL_SIZE)
#define GRID_H (SCREEN_HEIGHT / CELL_SIZE)
#define MAX_TRAIL_LENGTH 10 // maximum number of positions to store in the trail

// --- BLOCK TYPES ---
typedef enum {
    BLOCK_AIR = 0,
    BLOCK_STONE, // Solid Wall
    BLOCK_DIRT,  // Solid Wall
    BLOCK_SAND,  // Solid Wall (Grainy)
    BLOCK_WATER, // Liquid (Low Density)
    BLOCK_LAVA,  // Liquid (High Density)
    BLOCK_WOOD,  // Solid Fuel
    BLOCK_FIRE,  // Gas (Hot)
    BLOCK_SMOKE, // Gas (Rising)
    BLOCK_COUNT
} BlockType;

// --- ENTITIES ---
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
    BlockType type; // FOREGROUND: Wall, Water, Fire, or AIR (Empty)
    Color color;    // Foreground color

    BlockType floor;    // BACKGROUND: Always Dirt (or other floor types)
    Color floorColor;   // Background Color (It would flicker if we would not save it)

    int life;   // Acts as "Stamina" for liquids (spread distance) or "Health" for fire
    bool active;    // Optimization flag (not yet used)
} Cell;

// --- PROTOTYPES ---
void InitWorld();
void UpdateWorld(); // Cellular Automata Logic
void DrawWorld();

void InitPlayer(Player* p);
void UpdateTrail(Vector2* trailPositions, Player p);
void UpdatePlayer(Player* p, float dt);
void DrawPlayer(Player* p);
Color GetBlockColor(BlockType t);

// Interaction
void EditWorld(int x, int y, BlockType type, int radius);
bool IsSolid(BlockType t);
int GetDensity(BlockType t); // New Density Check

// UI
void DrawHUD(Player* p, Inventory* inv);

// FX
void Trail(Player* p, Vector2 *trailPositions);

#endif