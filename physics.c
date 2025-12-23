#include "game.h"

// The World Grid (Double Buffered)
static Cell grid[GRID_H][GRID_W];
static Cell nextGrid[GRID_H][GRID_W]; 

// Generate noise colors for texture
Color GetBlockColor(BlockType t) {
    int v = GetRandomValue(-15, 15);
    switch(t) {
        case BLOCK_STONE: return (Color){100+v, 100+v, 100+v, 255};
        case BLOCK_DIRT:  return (Color){120+v, 90+v, 40+v, 255};
        case BLOCK_SAND:  return (Color){230+v, 210+v, 100+v, 255};
        case BLOCK_WATER: return (Color){0, 150+v, 250, 200}; // Transparent Blue
        case BLOCK_LAVA:  return (Color){255, 100+v, 0, 255};
        case BLOCK_WOOD:  return (Color){139+v, 69+v, 19+v, 255};
        case BLOCK_FIRE:  return (Color){255, GetRandomValue(150,250), 0, 255}; // Flicker
        case BLOCK_SMOKE: return (Color){50+v, 50+v, 50+v, 150};
        default: return BLANK;
    }
}

// --- PHYSICS HELPER: DENSITY ---
int GetDensity(BlockType t) {
    switch(t) {
        case BLOCK_STONE: 
        case BLOCK_DIRT:
        case BLOCK_SAND:
        case BLOCK_WOOD: return 1000; // Immovable Solids
        
        case BLOCK_LAVA: return 100;  // Heavy Liquid
        case BLOCK_WATER: return 50;  // Light Liquid
        
        case BLOCK_SMOKE: return 5;   // Gas
        case BLOCK_FIRE: return 1;    // Plasma/Gas
        case BLOCK_AIR: return 0;     // Vacuum
        default: return 0;
    }
}

void InitWorld() {
    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            grid[y][x].type = BLOCK_AIR;
            grid[y][x].active = false;
            
            // Generate Floor
            if (y > GRID_H - 5) grid[y][x] = (Cell){BLOCK_STONE, 0, true, GetBlockColor(BLOCK_STONE)};
            else if (y > GRID_H - 15) grid[y][x] = (Cell){BLOCK_DIRT, 0, true, GetBlockColor(BLOCK_DIRT)};
        }
    }
}

bool IsSolid(BlockType t) {
    return (t == BLOCK_STONE || t == BLOCK_WOOD || t == BLOCK_DIRT || t == BLOCK_SAND);
}

bool IsValid(int x, int y) {
    return (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H);
}

// Brush Tool
void EditWorld(int x, int y, BlockType type, int radius) {
    for(int j = -radius; j <= radius; j++) {
        for(int i = -radius; i <= radius; i++) {
            int nx = x + i; int ny = y + j;
            if (IsValid(nx, ny)) {
                // Don't overwrite Solids (Stone) with fluids, unless mining (Air)
                if (type == BLOCK_AIR || !IsSolid(grid[ny][nx].type)) {
                    grid[ny][nx].type = type;
                    grid[ny][nx].color = GetBlockColor(type);
                    
                    // Initialize spread life
                    if (type == BLOCK_WATER) grid[ny][nx].life = 10; 
                    else if (type == BLOCK_LAVA) grid[ny][nx].life = 20;
                    else if (type == BLOCK_FIRE) grid[ny][nx].life = 100;
                    else grid[ny][nx].life = 0;
                    
                    grid[ny][nx].active = true; 
                }
            }
        }
    }
}

// --- CELLULAR AUTOMATA ENGINE ---
void UpdateWorld() {
    // 1. Copy State
    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            nextGrid[y][x] = grid[y][x];
        }
    }

    // 2. Physics Pass
    for(int y = 0; y < GRID_H; y++) {
        for(int x = 0; x < GRID_W; x++) {
            Cell c = grid[y][x];
            if (c.type == BLOCK_AIR) continue;

            // Coordinates (x, y)
            int dx = 0, dy = 0; 

            // --- FLUIDS (Water, Lava) ---
            if (c.type == BLOCK_WATER || c.type == BLOCK_LAVA) {
                if (c.life <= 0) continue; // Settled

                // Viscosity check
                int skipChance = (c.type == BLOCK_LAVA) ? 10 : 2;
                if (GetRandomValue(0, skipChance) != 0) continue;

                // Random Direction
                int dir = GetRandomValue(0, 3);
                switch(dir){
                    case 0:
                        dy = -1;
                        break;
                    case 1:
                        dx = 1;
                        break;
                    case 2:
                        dy = 1;
                        break;
                    case 3:
                        dx = -1;
                        break;
                    default:
                        dx = 1;
                }

                if (IsValid(x+dx, y+dy)) {
                    BlockType target = grid[y+dy][x+dx].type;
                    int myDen = GetDensity(c.type);
                    int targetDen = GetDensity(target);

                    // Move if empty OR Displace if heavier
                    if (target == BLOCK_AIR || (!IsSolid(target) && myDen > targetDen)) {
                        if (nextGrid[y+dy][x+dx].type == target) {
                            nextGrid[y+dy][x+dx] = c;
                            nextGrid[y+dy][x+dx].life--;
                            nextGrid[y][x] = grid[y+dy][x+dx]; // Swap
                        }
                    }
                }
            }

            // --- FIRE ---
            else if (c.type == BLOCK_FIRE) {
                // Spread
                for(int i=-1; i<=1; i++) {
                    for(int j=-1; j<=1; j++) {
                        if(IsValid(x+i, y+j) && grid[y+j][x+i].type == BLOCK_WOOD) {
                            if(GetRandomValue(0, 20) == 0) {
                                nextGrid[y+j][x+i].type = BLOCK_FIRE;
                                nextGrid[y+j][x+i].life = 150;
                                nextGrid[y+j][x+i].color = GetBlockColor(BLOCK_FIRE);
                            }
                        }
                    }
                }
                // Decay
                nextGrid[y][x].life--;
                nextGrid[y][x].color = GetBlockColor(BLOCK_FIRE); 
                if(nextGrid[y][x].life <= 0) {
                    nextGrid[y][x].type = BLOCK_SMOKE;
                    nextGrid[y][x].life = 60;
                    nextGrid[y][x].color = GetBlockColor(BLOCK_SMOKE);
                }
            }

            // --- SMOKE ---
            else if (c.type == BLOCK_SMOKE) {
                if (GetRandomValue(0, 3) == 0) {
                    int dir = GetRandomValue(0, 3);
                    switch(dir){
                        case 0:
                            dy = -1;
                            break;
                        case 1:
                            dx = 1;
                            break;
                        case 2:
                            dy = 1;
                            break;
                        case 3:
                            dx = -1;
                            break;
                        default:
                            dx = 1;
                    }
                    
                    if (IsValid(x+dx, y+dy) && grid[y+dy][x+dx].type == BLOCK_AIR) {
                         if (nextGrid[y+dy][x+dx].type == BLOCK_AIR) {
                            nextGrid[y+dy][x+dx] = c;
                            nextGrid[y][x].type = BLOCK_AIR;
                         }
                    }
                }
                nextGrid[y][x].life--;
                if(nextGrid[y][x].life <= 0) nextGrid[y][x].type = BLOCK_AIR;
            }
        }
    }

    // 3. Swap
    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            grid[y][x] = nextGrid[y][x];
        }
    }
}

// --- RENDER GRID ---
void DrawWorld() {
    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            if (grid[y][x].type != BLOCK_AIR) {
                DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, grid[y][x].color);
            }
        }
    }
}

// --- PLAYER PHYSICS (MOVE AND SLIDE) ---

void InitPlayer(Player* p) {
    p->position = (Vector2){400, 300};
    p->velocity = (Vector2){0,0};
    p->size = 12.0f;
    p->color = MAROON;
}

// Helper: Checks if a rectangle overlaps any SOLID blocks in the grid
bool CheckCollision(Vector2 pos, float radius) {
    int minX = (int)((pos.x - radius) / CELL_SIZE);
    int maxX = (int)((pos.x + radius) / CELL_SIZE);
    int minY = (int)((pos.y - radius) / CELL_SIZE);
    int maxY = (int)((pos.y + radius) / CELL_SIZE);

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            if (IsValid(x, y) && IsSolid(grid[y][x].type)) {
                return true;
            }
        }
    }
    return false;
}

void UpdatePlayer(Player* p, float dt) {
    Vector2 input = {0,0};
    if (IsKeyDown(KEY_W)) input.y -= 1;
    if (IsKeyDown(KEY_S)) input.y += 1;
    if (IsKeyDown(KEY_A)) input.x -= 1;
    if (IsKeyDown(KEY_D)) input.x += 1;
    
    if (Vector2Length(input) > 0) input = Vector2Normalize(input);
    
    p->velocity = Vector2Scale(input, 200.0f); // 200 pixels/sec speed
    
    // --- MOVE AND SLIDE LOGIC ---
    
    // 1. Try Moving X
    Vector2 nextPosX = { p->position.x + p->velocity.x * dt, p->position.y };
    if (!CheckCollision(nextPosX, p->size)) {
        p->position.x = nextPosX.x;
    }
    
    // 2. Try Moving Y (Independent of X)
    Vector2 nextPosY = { p->position.x, p->position.y + p->velocity.y * dt };
    if (!CheckCollision(nextPosY, p->size)) {
        p->position.y = nextPosY.y;
    }
    
    // 3. Screen Bounds Check
    if (p->position.x < p->size) p->position.x = p->size;
    if (p->position.x > SCREEN_WIDTH - p->size) p->position.x = SCREEN_WIDTH - p->size;
    if (p->position.y < p->size) p->position.y = p->size;
    if (p->position.y > SCREEN_HEIGHT - p->size) p->position.y = SCREEN_HEIGHT - p->size;
}

void DrawPlayer(Player* p) {
    DrawRectangle(p->position.x - p->size, p->position.y - p->size, p->size*2, p->size*2, p->color);
    DrawRectangleLines(p->position.x - p->size, p->position.y - p->size, p->size*2, p->size*2, BLACK);
}