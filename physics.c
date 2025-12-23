#include "game.h"

static Cell grid[GRID_H][GRID_W];
static Cell nextGrid[GRID_H][GRID_W]; 

Color GetBlockColor(BlockType t) {
    int v = GetRandomValue(-15, 15);
    switch(t) {
        case BLOCK_STONE: return (Color){100+v, 100+v, 100+v, 255};
        case BLOCK_DIRT:  return (Color){120+v, 90+v, 40+v, 255};
        case BLOCK_SAND:  return (Color){230+v, 210+v, 100+v, 255};
        case BLOCK_WATER: return (Color){0, 150+v, 250+v, 200}; 
        case BLOCK_LAVA:  return (Color){255, 100+v, 0, 255};
        case BLOCK_WOOD:  return (Color){139+v, 69+v, 19+v, 255};
        case BLOCK_FIRE:  return (Color){255, GetRandomValue(150,250), 0, 255};
        case BLOCK_SMOKE: return (Color){50+v, 50+v, 50+v, 150};
        default: return BLANK;
    }
}

void InitWorld() {
    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            grid[y][x].type = BLOCK_AIR;
            grid[y][x].active = false;
        }
    }
}

// Defines what stops the player
bool IsSolid(BlockType t) {
    return (t == BLOCK_STONE || t == BLOCK_WOOD || t == BLOCK_DIRT || t == BLOCK_SAND);
}

bool IsValid(int x, int y) {
    return (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H);
}

void EditWorld(int x, int y, BlockType type, int radius) {
    for(int j = -radius; j <= radius; j++) {
        for(int i = -radius; i <= radius; i++) {
            int nx = x + i; int ny = y + j;
            if (IsValid(nx, ny)) {
                if (type == BLOCK_AIR || !IsSolid(grid[ny][nx].type)) {
                    grid[ny][nx].type = type;
                    grid[ny][nx].color = GetBlockColor(type);
                    grid[ny][nx].life = (type == BLOCK_FIRE) ? 100 : 0; 
                    grid[ny][nx].active = true; 
                }
            }
        }
    }
}

// --- TOP-DOWN PHYSICS ---
void UpdateWorld() {
    // 1. Snapshot
    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            nextGrid[y][x] = grid[y][x];
        }
    }

    // 2. Logic Pass
    for(int y = 0; y < GRID_H; y++) {
        for(int x = 0; x < GRID_W; x++) {
            Cell c = grid[y][x];
            if (c.type == BLOCK_AIR) continue;

            int dx = 0, dy = 0; 

            // --- FLUIDS (Diffusion) ---
            // Water/Lava spread randomly to neighbors to simulate a puddle
            if (c.type == BLOCK_WATER || c.type == BLOCK_LAVA) {
                // Pick a random direction: 0:U, 1:R, 2:D, 3:L
                int dir = GetRandomValue(0, 3);
                if (dir == 0) dy = -1;
                else if (dir == 1) dx = 1;
                else if (dir == 2) dy = 1;
                else if (dir == 3) dx = -1;

                // Move if empty
                if (IsValid(x+dx, y+dy) && grid[y+dy][x+dx].type == BLOCK_AIR) {
                    // Check nextGrid too to prevent overwriting
                    if (nextGrid[y+dy][x+dx].type == BLOCK_AIR) {
                        nextGrid[y+dy][x+dx] = c;
                        nextGrid[y][x].type = BLOCK_AIR;
                    }
                }
            }

            // --- FIRE (Spreading) ---
            else if (c.type == BLOCK_FIRE) {
                // Spread to Wood neighbors in all directions
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
                nextGrid[y][x].life--;
                nextGrid[y][x].color = GetBlockColor(BLOCK_FIRE); // Flicker
                if(nextGrid[y][x].life <= 0) {
                    nextGrid[y][x].type = BLOCK_SMOKE;
                    nextGrid[y][x].life = 60;
                    nextGrid[y][x].color = GetBlockColor(BLOCK_SMOKE);
                }
            }

            // --- SMOKE (Dissipate) ---
            else if (c.type == BLOCK_SMOKE) {
                int dir = GetRandomValue(0, 3); // Waft around
                if (dir == 0) dy = -1; else if (dir == 1) dx = 1; 
                else if (dir == 2) dy = 1; else if (dir == 3) dx = -1;

                if (IsValid(x+dx, y+dy) && grid[y+dy][x+dx].type == BLOCK_AIR) {
                     if (nextGrid[y+dy][x+dx].type == BLOCK_AIR) {
                        nextGrid[y+dy][x+dx] = c;
                        nextGrid[y][x].type = BLOCK_AIR;
                     }
                }
                nextGrid[y+dy][x+dx].life--;
                if(nextGrid[y+dy][x+dx].life <= 0) nextGrid[y+dy][x+dx].type = BLOCK_AIR;
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

// --- RENDER ---
void DrawWorld() {
    for(int y=0; y<GRID_H; y++) {
        for(int x=0; x<GRID_W; x++) {
            if (grid[y][x].type != BLOCK_AIR) {
                DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, grid[y][x].color);
            }
        }
    }
}

// --- PLAYER ---
void InitPlayer(Player* p) {
    p->position = (Vector2){400, 300};
    p->velocity = (Vector2){0,0};
    p->size = 12.0f;
    p->color = MAROON;
}

void UpdatePlayer(Player* p, float dt) {
    Vector2 input = {0,0};
    if (IsKeyDown(KEY_W)) input.y -= 1;
    if (IsKeyDown(KEY_S)) input.y += 1;
    if (IsKeyDown(KEY_A)) input.x -= 1;
    if (IsKeyDown(KEY_D)) input.x += 1;
    
    if (Vector2Length(input) > 0) input = Vector2Normalize(input);
    
    p->velocity = Vector2Scale(input, 200.0f);
    Vector2 nextPos = Vector2Add(p->position, Vector2Scale(p->velocity, dt));
    
    // Collision checking (4 corners)
    int cornersX[4] = { -1, 1, -1, 1 };
    int cornersY[4] = { -1, -1, 1, 1 };
    bool collision = false;
    
    for(int i=0; i<4; i++) {
        int gx = (nextPos.x + cornersX[i]*p->size) / CELL_SIZE;
        int gy = (nextPos.y + cornersY[i]*p->size) / CELL_SIZE;
        
        if (IsValid(gx, gy) && IsSolid(grid[gy][gx].type)) collision = true;
    }
    
    if (!collision) p->position = nextPos;
}

void DrawPlayer(Player* p) {
    DrawRectangle(p->position.x - p->size, p->position.y - p->size, p->size*2, p->size*2, p->color);
    DrawRectangleLines(p->position.x - p->size, p->position.y - p->size, p->size*2, p->size*2, BLACK);
}